#pragma once

#include <stddef.h>

#include <modbus-c/rtu.h>
#include <ws2812b/rgb.h>
#include <ws2812b/ws2812b.h>

#ifndef RTU_ADDR
#error "Please define RTU_ADDR"
#endif

#define RTU_CMD_ADDR_BASE UINT16_C(0x1000)

typedef union
{
    fire_heat_t fire_heat[STRIP_SIZE]; // 1 * STRIP_SIZE
    torch_energy_t torch_energy[STRIP_SIZE]; // 1 * STRIP_SIZE
} fx_data_t; // 1 * STRIP_SIZE

STATIC_ASSERT(sizeof(fx_data_t) == 1 * STRIP_SIZE);

typedef union
{
    fire_param_t fire_param; // 1
    union
    {
        torch_param_t torch_param; // 7
        /* extends torch param to account for mode data for each STRIP element
         * (2bits per element) */
        uint8_t torch_param_mode[sizeof(torch_param_t) + (STRIP_SIZE >> 2)];
    }; // 7 + 120/4 = 37
} fx_param_t; // 37

STATIC_ASSERT(sizeof(fx_param_t) == 37);

typedef struct
{
    // 0
    uint16_t size;
    // 2
    struct
    {
        uint8_t strip_updated : 1; // LSB
        uint8_t strip_refresh : 1;
        uint8_t reserved : 2;
        uint8_t strip_fx : 4;
    };
    // 3
    uint16_t tmr1_A;
    // 5
    rgb_t rgb_data[STRIP_SIZE];
    // 365 == (5 + (STRIP_SIZE * 3) = 5 + 360)
    ws2812b_strip_t ws2812b_strip;
    // 389 == (365 + 24 = 389)
    fx_data_t fx_data;
    // 509 == (389 + (STRIP_SIZE * 1) = 509)
    fx_param_t fx_param;
    // 546 = (509 + 37) = 546
    char tlog[TLOG_SIZE];
} rtu_memory_fields_t;


STATIC_ASSERT_STRUCT_OFFSET(rtu_memory_fields_t, tmr1_A, 3);
STATIC_ASSERT_STRUCT_OFFSET(rtu_memory_fields_t, rgb_data, 5);
STATIC_ASSERT_STRUCT_OFFSET(rtu_memory_fields_t, fx_data, 389);
STATIC_ASSERT_STRUCT_OFFSET(rtu_memory_fields_t, ws2812b_strip, 365);
STATIC_ASSERT_STRUCT_OFFSET(rtu_memory_fields_t, tlog, 546);

typedef union
{
    rtu_memory_fields_t fields;
    uint8_t data[sizeof(rtu_memory_fields_t)];
} rtu_memory_t;

void rtu_memory_clear(rtu_memory_t *);
void rtu_memory_init(rtu_memory_t *);

uint8_t *rtu_pdu_cb(
    modbus_rtu_state_t *state,
    modbus_rtu_addr_t addr,
    modbus_rtu_fcode_t fcode,
    const uint8_t *begin, const uint8_t *end,
    const uint8_t *curr,
    uint8_t *dst_begin, const uint8_t *const dst_end,
    uintptr_t user_data);
