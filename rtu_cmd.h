#pragma once

#include <stddef.h>

#include <modbus_c/rtu.h>
#include <modbus_c/rtu_memory.h>
#include <ws2812b_strip/noise.h>
#include <ws2812b_strip/rgb.h>
#include <ws2812b_strip/ws2812b.h>

#ifndef EEPROM_ADDR_RTU_ADDR
#error "Please define EEPROM_ADDR_RTU_ADDR"
#endif

#ifndef RTU_ADDR_BASE
#error "Please define RTU_ADDR_BASE"
#endif

typedef union
{
    fire_heat_t fire_heat[STRIP_SIZE]; // 1 * STRIP_SIZE
    torch_energy_t torch_energy[STRIP_SIZE]; // 1 * STRIP_SIZE
    energy_t energy[STRIP_SIZE]; // 1 * STRIP_SIZE
} fx_data_t; // 1 * STRIP_SIZE

STATIC_ASSERT(sizeof(fx_data_t) == 1 * STRIP_SIZE);

typedef union
{
    fire_param_t fire_param; // 1
    union
    {
        torch_param_t torch_param; // 9
        /* extends torch param to account for mode data for each STRIP element
         * (2bits per element) */
        uint8_t torch_param_mode[sizeof(torch_param_t) + (STRIP_SIZE >> 2)];
    }; // 9 + STRIP_SIZE / 4

    noise_param_t noise_param;
} fx_param_t;

STATIC_ASSERT(sizeof(fx_param_t) == 9 + STRIP_SIZE >> 2);

typedef struct
{
    rtu_memory_t rtu_memory;
    // 0
    uint16_t fw_checksum;
    // 2
    struct
    {
        uint8_t strip_updated : 1; // LSB
        uint8_t : 2;
        uint8_t reboot : 1;
        uint8_t strip_fx : 4;
    };
    // 3
    uint16_t tmr1_A;
    // 5
    union
    {
        struct
        {
            uint8_t heartbeat_low;
            uint8_t heartbeat_high;
        };
        uint16_t heartbeat;
    };
    // 7
    uint8_t rtu_err_reboot_threashold;
    // 8
    ws2812b_strip_t ws2812b_strip;
    // 33 = 8 + 25
    rgb_t rgb_data[STRIP_SIZE];
    // 33 + (STRIP_SIZE * 3)
    fx_data_t fx_data;
    // 33 + (STRIP_SIZE * 3) + (STRIP_SIZE * 1)
    fx_param_t fx_param;
    // 33 + (STRIP_SIZE * 3) + (STRIP_SIZE * 1) + 9 + (STRIP_SIZE >> 2)
    char tlog[TLOG_SIZE];
} rtu_memory_fields_t;


STATIC_ASSERT_STRUCT_OFFSET(
    rtu_memory_fields_t, tmr1_A,
    sizeof(rtu_memory_t) + 3);

STATIC_ASSERT_STRUCT_OFFSET(
    rtu_memory_fields_t, heartbeat,
    sizeof(rtu_memory_t) + 5);

STATIC_ASSERT_STRUCT_OFFSET(
    rtu_memory_fields_t, heartbeat_low,
    sizeof(rtu_memory_t) + 5);

STATIC_ASSERT_STRUCT_OFFSET(
    rtu_memory_fields_t, heartbeat_high,
    sizeof(rtu_memory_t) + 6);

STATIC_ASSERT_STRUCT_OFFSET(
    rtu_memory_fields_t, rtu_err_reboot_threashold,
    sizeof(rtu_memory_t) + 7);

STATIC_ASSERT_STRUCT_OFFSET(
    rtu_memory_fields_t, ws2812b_strip,
    sizeof(rtu_memory_t) + 8);

STATIC_ASSERT_STRUCT_OFFSET(
    rtu_memory_fields_t, rgb_data,
    sizeof(rtu_memory_t) + 33);

STATIC_ASSERT_STRUCT_OFFSET(
    rtu_memory_fields_t, fx_data,
    sizeof(rtu_memory_t) + 33 + (STRIP_SIZE * 3));

STATIC_ASSERT_STRUCT_OFFSET(
    rtu_memory_fields_t, fx_param,
    sizeof(rtu_memory_t) + 33 + (STRIP_SIZE * 3) + (STRIP_SIZE * 1));

STATIC_ASSERT_STRUCT_OFFSET(
    rtu_memory_fields_t, tlog,
    sizeof(rtu_memory_t) + 33 + (STRIP_SIZE * 3) + (STRIP_SIZE * 1) + 9 + (STRIP_SIZE >> 2));

void rtu_memory_fields_clear(rtu_memory_fields_t *);
void rtu_memory_fields_init(rtu_memory_fields_t *);

uint8_t *rtu_pdu_cb(
    modbus_rtu_state_t *state,
    modbus_rtu_addr_t addr,
    modbus_rtu_fcode_t fcode,
    const uint8_t *begin, const uint8_t *end,
    const uint8_t *curr,
    uint8_t *dst_begin, const uint8_t *const dst_end,
    uintptr_t user_data);
