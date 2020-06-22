#include "rtu_cmd.h"

#include <string.h>
#include <stddef.h>

#include <ws2812b/ws2812b.h>
#include <drv/tlog.h>

void rtu_memory_fields_clear(rtu_memory_fields_t *fields)
{
    memset(fields, 0, sizeof(rtu_memory_fields_t));
}

void rtu_memory_fields_init(rtu_memory_fields_t *fields)
{
    fields->rtu_memory.addr_begin = RTU_ADDR_BASE;
    fields->rtu_memory.addr_end = RTU_ADDR_BASE + sizeof(rtu_memory_fields_t) - sizeof(rtu_memory_t);

    fields->size = sizeof(rtu_memory_fields_t);
    fields->strip_fx = FX_NONE;
	/* 16MHz / 64 = 1MHz / 4 = 250kHz === 4us
	 * 16bit x 4us = 262144us = 262ms
	 * 30fps === 33333us / 4us = 8333
	 * 60fps === 16666us / 4us = 4167
     * 120fps === 8333us / 4us = 2083 */
    fields->tmr1_A = 4167;

    fields->ws2812b_strip =
        (ws2812b_strip_t)
        {
            .rgb_map =
            {
                .header =
                    (map_header_t)
                    {
                        .stride = STRIP_STRIDE,
                        .width = STRIP_WIDTH,
                        .height = STRIP_HEIGHT,
                    },
                .rgb = fields->rgb_data,
                .brightness = 0xFF,
                .color_correction =
                    (rgb_t)
                    {
                        .R = VALUE_R(COLOR_CORRECTION_5050),
                        .G = VALUE_G(COLOR_CORRECTION_5050),
                        .B = VALUE_B(COLOR_CORRECTION_5050)
                    },
                .temp_correction =
                    (rgb_t)
                    {
                        .R = VALUE_R(TEMP_CORRECTION_Tungsten100W),
                        .G = VALUE_G(TEMP_CORRECTION_Tungsten100W),
                        .B = VALUE_B(TEMP_CORRECTION_Tungsten100W)
                    },
                .palette_id = (palette_id_t){.value = PALETTE_ID_INVALID}
            },
            .rgb_idx = 0,
            .rgb_size = STRIP_SIZE * sizeof(rgb_t),
            .fx_data_map =
            {
                .data_map =
                    (data_map_t)
                    {
                        .header =
                            (map_header_t)
                            {
                                .stride = STRIP_STRIDE,
                                .width = STRIP_WIDTH,
                                .height = STRIP_HEIGHT,
                            },
                        .data = &fields->fx_data,
                        .param = &fields->fx_param
                    }
            },
            .flags =
                (ws2812b_flags_t)
                {
                    .update = 0,
                    .updated = 0,
                    .abort = 0,
                    .aborted = 0,
                    .fx = FX_NONE
                }
        };
}

uint8_t *rtu_pdu_cb(
    modbus_rtu_state_t *state,
    modbus_rtu_addr_t addr,
    modbus_rtu_fcode_t fcode,
    const uint8_t *begin, const uint8_t *end,
    /* curr == begin + sizeof(addr_t) + sizeof(fcode_t) */
    const uint8_t *curr,
    uint8_t *dst_begin, const uint8_t *const dst_end,
    uintptr_t user_data)
{
    rtu_memory_fields_t *rtu_memory_fields = (rtu_memory_fields_t *)user_data;

    TLOG_PRINTF("S%02" PRIX8 " F%02" PRIX8, addr, fcode);

    if(modbus_rtu_addr(state) != addr) goto exit;

    *dst_begin++ = addr;

    dst_begin =
        rtu_memory_pdu_cb(
            &rtu_memory_fields->rtu_memory,
            fcode,
            begin + sizeof(addr), end,
            curr,
            dst_begin, dst_end);
exit:
    return dst_begin;
}
