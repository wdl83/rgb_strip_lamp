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
    ws2812b_strip_t strip;
    uint8_t raw[WS2812B_STRIP_SIZE(STRIP_LENGTH)];
} ws2812b_mmap_t;

typedef union
{
    map_header_t header;
    uint8_t fire_raw[FIRE_MAP_SIZE(STRIP_LENGTH)];
    uint8_t torch_raw[TORCH_MAP_SIZE(STRIP_LENGTH)];
    uint8_t noise_raw[NOISE_MAP_SIZE(STRIP_LENGTH)];
    uint8_t raw[0];
} fx_mmap_t;

typedef struct
{
    rtu_memory_t rtu_memory;
    uint16_t fw_checksum;
    struct
    {
        uint8_t strip_updated : 1; // LSB
        uint8_t : 2;
        uint8_t reboot : 1;
        uint8_t strip_fx : 4;
    };
    uint16_t tmr1_A;
    union
    {
        struct
        {
            uint8_t heartbeat_low;
            uint8_t heartbeat_high;
        };
        uint16_t heartbeat;
    };
    uint8_t rtu_err_reboot_threashold;
    ws2812b_mmap_t ws2812b_mmap;
    fx_mmap_t fx_mmap;
    char tlog[TLOG_SIZE];
} rtu_memory_fields_t;


STATIC_ASSERT_STRUCT_OFFSET(
    rtu_memory_fields_t, fw_checksum,
    sizeof(rtu_memory_t) + 0);

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
    rtu_memory_fields_t, ws2812b_mmap,
    sizeof(rtu_memory_t) + 8);

STATIC_ASSERT_STRUCT_OFFSET(
    rtu_memory_fields_t, fx_mmap,
    sizeof(rtu_memory_t) + 8 + 17 + STRIP_LENGTH * 3);

STATIC_ASSERT_STRUCT_OFFSET(
    rtu_memory_fields_t, tlog,
    sizeof(rtu_memory_t) + 8 + 17 + STRIP_LENGTH * 3
    /* max(fire, torch, noise) */
    + 12 + STRIP_LENGTH + (STRIP_LENGTH >> 2));

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
