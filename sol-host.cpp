#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <string.h>

extern "C"
{
#include <ws2812b/fire.h>
#include <ws2812b/torch.h>
} /* extern "C" */


#define STRIP_SIZE (STRIP_WIDTH * STRIP_HEIGHT)


map_size_t torch_mode_byte_offset(map_size_t stride, map_size_t x, map_size_t y)
{
    return (y * stride  + x) >> 2;
}

map_size_t torch_mode_bit_offset(map_size_t stride, map_size_t x, map_size_t y)
{
    return (y * stride + x) & UINT8_C(0x3);
}

uint8_t torch_mode_get(uint8_t *data, map_size_t stride, map_size_t x, map_size_t y)
{
    const auto byte_offset = TORCH_MODE_BYTE_OFFSET(stride, x, y);
    const auto bit_offset =  TORCH_MODE_BIT_OFFSET(stride, x, y);
    const auto value = data[byte_offset];
    const auto r = (value >> bit_offset);
    const auto rr = r & UINT8_C(0x3);
    return rr;
}

void torch_mode_set(uint8_t *data, map_size_t stride, map_size_t x, map_size_t y, uint8_t mode)
{
    const auto byte_offset = TORCH_MODE_BYTE_OFFSET(stride, x, y);
    const auto bit_offset =  TORCH_MODE_BIT_OFFSET(stride, x, y);
    const auto value = data[byte_offset];
    const uint8_t and_mask = ~(UINT8_C(0x3) << bit_offset);
    const uint8_t or_mask = ((mode) & UINT8_C(0x3)) <<  bit_offset;
    const auto r = data[byte_offset] & and_mask;
    const auto rr = r | or_mask;
    data[byte_offset] = rr;
}


void torch_map_test(torch_energy_map_t *map)
{
    const auto stride = map->header.stride;
    torch_param_t *const param = map->param;

    int i = 0;
    for(map_size_t y = 0; y < map->header.height; ++y)
    {
        for(map_size_t x = 0; x < map->header.width; ++x)
        {
            //if(0 == (y%2))TORCH_MODE_SET(param->mode, stride, x, y, uint8_t(i % 4));
            if(1 == (y%2))torch_mode_set(param->mode, stride, x, y, i % 4);
                ++i;
        }
    }
}

void map_dump(torch_energy_map_t *map)
{
#if 0
    for(map_size_t i = 0; i < map->header.size; ++i)
    {
        std::ostringstream oss;
        oss << "\33[48;2;" << int(map->data[i].value) << ";0;0m ";
        std::cout << oss.str();

        if(0 == (i + 1) % map->header.stride) std::cout << '\n';
    }
#endif
    const auto stride = map->header.stride;
    torch_param_t *const param = map->param;

    for(map_size_t y = 0; y < map->header.height; ++y)
    {
        for(map_size_t x = 0; x < map->header.width; ++x)
        {
            auto mode =
                TORCH_MODE_NONE == TORCH_MODE_GET(param->mode, stride, x, y)
                ? 'N'
                : (
                    TORCH_MODE_PASSIVE == TORCH_MODE_GET(param->mode, stride, x, y)
                    ? 'P'
                    : (
                        TORCH_MODE_SPARK == TORCH_MODE_GET(param->mode, stride, x, y)
                        ? 'S'
                        : 'T'));

            std::ostringstream oss;
            oss << "\33[48;2;" << int(map->data[y * stride + x]) << ";0;0m" << mode;
            std::cout << oss.str();

        }
        std::cout << '\n';
    }
}

void map_dump(rgb_map_t *map)
{
    const map_size_t size =  map->header.height * map->header.width;

    for(map_size_t i = 0; i < size; ++i)
    {
        /*
           ESC[ 38;2;⟨r⟩;⟨g⟩;⟨b⟩ m Select RGB foreground color
           ESC[ 48;2;⟨r⟩;⟨g⟩;⟨b⟩ m Select RGB background color */
        std::ostringstream oss;

        oss
            //<< std::dec << std::setw(3) << std::setfill(' ')
            << "\33[48;2;" << int(map->rgb[i].R)
            << ';' << int(map->rgb[i].G)
            << ';' << int(map->rgb[i].B) << 'm' << ' ';
        std::cout << oss.str();

        if(0 == (i + 1) % map->header.stride) std::cout << '\n';
    }
}

int main(void)
{
    rgb_map_t rgb_map;
    rgb_t rgb[STRIP_SIZE];

    {
        memset(&rgb_map, 0, sizeof(rgb_map));
        memset(rgb, 0, STRIP_SIZE * sizeof(rgb_t));

        rgb_map.header.stride = STRIP_STRIDE;
        rgb_map.header.width = STRIP_WIDTH;
        rgb_map.header.height = STRIP_HEIGHT;
        rgb_map.brightness = 0xFF;
        rgb_map.color_correction.R = VALUE_R(COLOR_CORRECTION_5050);
        rgb_map.color_correction.G = VALUE_G(COLOR_CORRECTION_5050);
        rgb_map.color_correction.B = VALUE_B(COLOR_CORRECTION_5050);
        rgb_map.temp_correction.R = VALUE_R(TEMP_CORRECTION_None);
        rgb_map.temp_correction.G = VALUE_G(TEMP_CORRECTION_None);
        rgb_map.temp_correction.B = VALUE_B(TEMP_CORRECTION_None);
        rgb_map.rgb = rgb;
    }

    torch_energy_map_t torch_energy_map;
    torch_energy_t torch_energy_data[STRIP_SIZE];

    typedef union
    {
        torch_param_t torch_param; // 7
        /* extends torch param to account for mode data for each STRIP element
         * (2bits per element) */
        uint8_t torch_param_mode[sizeof(torch_param_t) + (STRIP_SIZE >> 2)];
    } fx_param_t; // 7 + 120/4 = 37

    fx_param_t fx_param;

    {
        memset(&torch_energy_map, 0, sizeof(torch_energy_map_t));
        memset(torch_energy_data, 0, STRIP_SIZE * sizeof(torch_energy_t));

        torch_energy_map.header.stride = STRIP_STRIDE;
        torch_energy_map.header.width = STRIP_WIDTH;
        torch_energy_map.header.height = STRIP_HEIGHT;
        torch_energy_map.data = torch_energy_data;
        torch_energy_map.param = &fx_param.torch_param;
    }

    torch_init(&torch_energy_map);

    for(;;)
    {
        //++strip.rgb_map.brightness;
        //std::cout << "\33[2J";
        std::cout << "\033[2J"/* reset */ << "\033[1;1H" /* cursor to upper-left */;
        torch_energy_map_update(&torch_energy_map);
        torch_rgb_map_update(&rgb_map, &torch_energy_map);
        std::cout << "\033[0m";
        std::cout << std::string(STRIP_STRIDE, '-') << '\n';
        //torch_map_test(&torch_energy_map);
        map_dump(&torch_energy_map);
        std::cout << "\033[0m";
        std::cout << std::string(STRIP_STRIDE, '-') << '\n';
        map_dump(&rgb_map);
        std::cout << "\033[0m";
        std::cout << std::string(STRIP_STRIDE, '-') << '\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return 0;
}
