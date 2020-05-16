#pragma once

#include <modbus-c/rtu.h>

#include <ws2812b/rgb.h>
#include <ws2812b/ws2812b.h>

#ifndef RTU_ADDR
#error "Please define RTU_ADDR"
#endif

#define RTU_CMD_ADDR_BASE UINT16_C(0x1000)

typedef struct
{
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
    uint8_t heat_data[STRIP_SIZE];
    // 125 (5 + STRIP_SIZE)
    rgb_t led_data[STRIP_SIZE];
    // 485 (5 + STRIP_SIZE + 3 * STRIP_SIZE)
    ws2812b_strip_t ws2812b_strip;
    // 509
    char tlog[TLOG_SIZE];
} rtu_memory_fields_t;

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
