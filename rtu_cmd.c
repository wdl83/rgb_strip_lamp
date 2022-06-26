#include <string.h>
#include <stddef.h>

#include <avr/pgmspace.h>
#include <util/crc16.h>

#include <ws2812b_strip/ws2812b.h>
#include <drv/tlog.h>

#include "rtu_cmd.h"

extern uint16_t __text_start;
//extern void *__text_end;
//extern void *__data_load_start;
extern uint16_t __data_load_end;

static
uint16_t calc_fw_checksum(void)
{
    uint16_t crc16 = UINT16_C(0xFFFF);
    { // .text & data sections
        uint8_t *begin = (uint8_t*)((uint16_t)&__text_start);
        const uint8_t *const end = (uint8_t *)((uint16_t)&__data_load_end);

        while(begin != end) crc16 = _crc16_update(crc16, pgm_read_byte(begin++));
    }
    return crc16;
}

void rtu_memory_fields_clear(rtu_memory_fields_t *fields)
{
    memset(fields, 0, sizeof(rtu_memory_fields_t));
}

void rtu_memory_fields_init(rtu_memory_fields_t *fields)
{
    fields->header.addr_begin = RTU_ADDR_BASE;
    fields->header.addr_end =
        RTU_ADDR_BASE + sizeof(rtu_memory_fields_t) - sizeof(rtu_memory_header_t);

    fields->fw_checksum = calc_fw_checksum();
    fields->strip_updated = 1;
    fields->strip_fx = FX_NONE;
	/* 16MHz / 64 = 1MHz / 4 = 250kHz === 4us
	 * 16bit x 4us = 262144us = 262ms
	 * 30fps === 33333us / 4us = 8333
	 * 50fps === 20000us / 4us = 5000
	 * 60fps === 16666us / 4us = 4167
     * 120fps === 8333us / 4us = 2083 */
    fields->tmr1_A = UINT16_C(5000);

    /* cyclic timer (tmr1) is used to decrement heartbeat value
     * default CTC value is 4167. HW clock cycle time of tmr1 is 4us
     * 4167 * 4us = 16668us = ~16ms
     * 0xFFFF x 16668us = 65535 x 16668us = 1,092,337,380us ~18min
     * 0x0400 x 16668us = 1024 x 16668us = 1,708,032 ~17,08s
     * 0x1000 x 16668us = 4096 x 16668us ~ 68,32s */
    fields->heartbeat = UINT16_C(HEARTBEAT_PERIOD);
    fields->rtu_err_reboot_threashold = UINT8_C(RTU_ERR_REBOOT_THREASHOLD);

    fields->ws2812b_mmap = (ws2812b_mmap_t)
    {
        .strip =
        {
            .flags = {.value = 0},
            .rgb_idx = 0,
            .rgb_size = STRIP_LENGTH * 3,
            .rgb_map =
            {
                .brightness = UINT8_C(0x00),
                .target_brightness = UINT8_C(0x7F),
                .color_correction =
                {
                    .R = VALUE_R(COLOR_CORRECTION_None),
                    .G = VALUE_G(COLOR_CORRECTION_None),
                    .B = VALUE_B(COLOR_CORRECTION_None)
                },
                .temp_correction =
                {
                    .R = VALUE_R(TEMP_CORRECTION_None),
                    .G = VALUE_G(TEMP_CORRECTION_None),
                    .B = VALUE_B(TEMP_CORRECTION_None)
                },
                .palette16_id = {.value = PALETTE16_ID_INVALID},
                .header =
                {
                    .stride = STRIP_STRIDE,
                    .width = STRIP_WIDTH,
                    .height = STRIP_HEIGHT,
                }
            }
        }
    };

    fields->fx_mmap = (fx_mmap_t)
    {
        .header =
        {
            .stride = STRIP_STRIDE,
            .width = STRIP_WIDTH,
            .height = STRIP_HEIGHT,
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

    //TLOG_XPRINT16("S|F", ((uint16_t)addr << 8) | fcode);

    /* because crossing rtu_err_reboot_threashold will cause
     * reboot decrese error count if valid PDU received */
    if(state->err_cntr) --state->err_cntr;

    if(modbus_rtu_addr(state) != addr) goto exit;

    *dst_begin++ = addr;

    dst_begin =
        rtu_memory_pdu_cb(
            (rtu_memory_t *)&rtu_memory_fields->header,
            fcode,
            begin + sizeof(addr), end,
            curr,
            dst_begin, dst_end);
exit:
    return dst_begin;
}
