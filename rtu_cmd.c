#include "rtu_cmd.h"

#include <string.h>
#include <stddef.h>

#include <ws2812b/ws2812b.h>
#include <drv/tlog.h>

void rtu_memory_clear(rtu_memory_t *rtu_memory)
{
    memset(rtu_memory->data, 0, sizeof(rtu_memory_t));
}

void rtu_memory_init(rtu_memory_t *rtu_memory)
{
    rtu_memory->fields.size = sizeof(rtu_memory_t);
    rtu_memory->fields.reserved = UINT8_C(0x3);
    rtu_memory->fields.strip_fx = FX_NONE;
	/* 16MHz / 64 = 1MHz / 4 = 250kHz === 4us
	 * 16bit x 4us = 262144us = 262ms
	 * 30fps === 33ms / 4us = 8333
	 * 60fps === 16,7ms / 4us = 4167 */
    rtu_memory->fields.tmr1_A = 4167;

    rtu_memory->fields.ws2812b_strip =
        (ws2812b_strip_t)
        {
            .rgb_map =
            {
                .stride = STRIP_STRIDE,
                .width = STRIP_WIDTH,
                .height = STRIP_HEIGHT,
                .size = STRIP_SIZE,
                .led = rtu_memory->fields.led_data,
                .brightness = 0xFF,
                .color_correction =
                    (rgb_t)
                    {
                        .R = VALUE_R(COLOR_CORRECTION_None),
                        .G = VALUE_G(COLOR_CORRECTION_None),
                        .B = VALUE_B(COLOR_CORRECTION_None)
                    },
                .temp_correction =
                    (rgb_t)
                    {
                        .R = VALUE_R(TEMP_CORRECTION_None),
                        .G = VALUE_G(TEMP_CORRECTION_None),
                        .B = VALUE_B(TEMP_CORRECTION_None)
                    },
            },
            .heat_map =
            {
                .stride = STRIP_STRIDE,
                .width = STRIP_WIDTH,
                .height = STRIP_HEIGHT,
                .size = STRIP_SIZE,
                .data = rtu_memory->fields.heat_data
            },
            .idx = 0,
            .size = STRIP_SIZE * 3,
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


typedef modbus_rtu_addr_t addr_t;
typedef modbus_rtu_crc_t crc_t;
typedef modbus_rtu_ecode_t ecode_t;
typedef modbus_rtu_fcode_t fcode_t;
typedef modbus_rtu_state_t state_t;


static inline
uint16_t rd16(const uint8_t *src)
{
    const uint8_t high = *src++;
    const uint8_t low = *src++;
    return high << 8 | low;
}

static inline
uint8_t *wr16(uint8_t *dst, uint16_t data)
{
    *dst++ = data >> 8;
    *dst++ = data & UINT16_C(0xFF);
    return dst;
}

#define ILLEGAL_FUNCTION 0x1
#define ILLEGAL_DATA_ADDRESS 0x2
#define ILLEGAL_DATA_VALUE 0x3
#define SERVER_DEVICE_FAILURE 0x4

static inline
uint8_t *except(fcode_t fcode, ecode_t ecode, uint8_t *reply)
{
    TLOG_TP();
    /* exception */
    *reply++ = fcode + UINT8_C(0x80);
    /* ILLEGAL FUNCTION */
    *reply++ = ecode;
    return reply;
}

static
uint8_t *read_n16(
    const uint8_t *begin,  const uint8_t *end,
    const uint8_t *curr,
    uint8_t *reply,
    uintptr_t user_data)
{
    (void)begin;

    if(2 > end - curr)
    {
        TLOG_TP();
        return except(FCODE_RD_HOLDING_REGISTERS, ILLEGAL_DATA_ADDRESS, reply);
    }

    /* starting address */
    const uint16_t addr = rd16(curr);
    curr += sizeof(addr);

    if(!(
            RTU_CMD_ADDR_BASE + sizeof(rtu_memory_t) > addr
            && RTU_CMD_ADDR_BASE <= addr))
    {
        TLOG_TP();
        return except(FCODE_RD_HOLDING_REGISTERS, ILLEGAL_DATA_ADDRESS, reply);
    }

    if(2 > end - curr)
    {
        TLOG_TP();
        return except(FCODE_RD_HOLDING_REGISTERS, ILLEGAL_DATA_VALUE, reply);
    }

    const uint16_t num = rd16(curr);
    curr += sizeof(num);

    if(!( 0 < num  && 0x7E > num))
    {
        TLOG_TP();
        return except(FCODE_RD_HOLDING_REGISTERS, ILLEGAL_DATA_VALUE, reply);
    }

    if(!(RTU_CMD_ADDR_BASE + sizeof(rtu_memory_t) > addr + num))
    {
        TLOG_TP();
        return except(FCODE_RD_HOLDING_REGISTERS, ILLEGAL_DATA_ADDRESS, reply);
    }

    TLOG_PRINTF("RA%04" PRIX16 " N%02" PRIX8, addr, (uint8_t)num);

    rtu_memory_t *rtu_memory = (rtu_memory_t *)user_data;

    /* fcode */
    *reply++ = UINT8_C(FCODE_RD_HOLDING_REGISTERS);
    /* byte count */
    *reply++ = (uint8_t)(num << 1);

    uint16_t addr_begin = addr - RTU_CMD_ADDR_BASE;
    const uint16_t  addr_end = addr_begin + num;

    for(; addr_begin != addr_end; ++addr_begin)
    {
        uint16_t data = rtu_memory->data[addr_begin];
        reply = wr16(reply, data);
    }

    return reply;
}

static
uint8_t *write_16(
    const uint8_t *begin,  const uint8_t *end,
    const uint8_t *curr,
    uint8_t *reply,
    uintptr_t user_data)
{
    if(2 > end - curr)
    {
        TLOG_TP();
        return except(FCODE_WR_REGISTER, ILLEGAL_DATA_ADDRESS, reply);
    }

    uint16_t addr = rd16(curr);
    curr += sizeof(addr);

    if(!(RTU_CMD_ADDR_BASE + sizeof(rtu_memory_t) > addr))
    {
        TLOG_TP();
        return except(FCODE_WR_REGISTER, ILLEGAL_DATA_ADDRESS, reply);
    }

    if(2 > end - curr)
    {
        TLOG_TP();
        return except(FCODE_WR_REGISTER, ILLEGAL_DATA_VALUE, reply);
    }

    uint16_t data = rd16(curr);
    curr += sizeof(data);

    if((UINT16_C(0xFF00) & data))
    {
        TLOG_TP();
        return except(FCODE_WR_REGISTER, ILLEGAL_DATA_VALUE, reply);
    }

    TLOG_PRINTF("WA%04" PRIX16 " D%02" PRIX8, addr, (uint8_t)data);

    rtu_memory_t *rtu_memory = (rtu_memory_t *)user_data;
    rtu_memory->data[addr - RTU_CMD_ADDR_BASE] = (uint8_t)data;

    /* echo request on success  */
    const uint16_t size =  1 /* fcode */ + 2 /* addr */ + 2 /* data */;

    return memcpy(reply, begin, size) + size;
}

static
uint8_t *write_n16(
    const uint8_t *begin,  const uint8_t *end,
    const uint8_t *curr,
    uint8_t *reply,
    uintptr_t user_data)
{
    if(2 > end - curr)
    {
        TLOG_TP();
        return except(FCODE_WR_REGISTERS, ILLEGAL_DATA_ADDRESS, reply);
    }

    const uint16_t addr = rd16(curr);
    curr += sizeof(addr);

    if(!(RTU_CMD_ADDR_BASE + sizeof(rtu_memory_t) > addr))
    {
        TLOG_TP();
        return except(FCODE_WR_REGISTERS, ILLEGAL_DATA_ADDRESS, reply);
    }

    if(2 > end - curr)
    {
        TLOG_TP();
        return except(FCODE_WR_REGISTERS, ILLEGAL_DATA_VALUE, reply);
    }

    const uint16_t num = rd16(curr);
    curr += sizeof(num);

    if(2 > end - curr)
    {
        TLOG_TP();
        return except(FCODE_WR_REGISTERS, ILLEGAL_DATA_VALUE, reply);
    }

    if(!( 0 < num  && 0x7C > num))
    {
        TLOG_TP();
        return except(FCODE_WR_REGISTERS, ILLEGAL_DATA_VALUE, reply);
    }

    const uint8_t byte_count = *curr;
    curr += sizeof(byte_count);

    if(byte_count != (num << 1))
    {
        TLOG_TP();
        return except(FCODE_WR_REGISTERS, ILLEGAL_DATA_VALUE, reply);
    }

    rtu_memory_t *rtu_memory = (rtu_memory_t *)user_data;

    TLOG_PRINTF("nWA%04" PRIX16 " N%02" PRIX8, addr, (uint8_t)num);

    uint16_t addr_begin = addr - RTU_CMD_ADDR_BASE;
    const uint16_t addr_end = addr_begin + num;

    for(; addr_begin != addr_end; ++addr_begin)
    {
        if(2 > end - curr)
        {
            TLOG_TP();
            return except(FCODE_WR_REGISTERS, ILLEGAL_DATA_VALUE, reply);
        }

        uint16_t data = rd16(curr);
        curr += sizeof(data);

        if((UINT16_C(0xFF00) & data))
        {
            TLOG_TP();
            TLOG_PRINTF("ERR %" PRIX16, data);
            return except(FCODE_WR_REGISTERS, ILLEGAL_DATA_VALUE, reply);
        }

        rtu_memory->data[addr_begin] = data;
    }

    /* echo request on success  */
    const uint16_t size =  1 /* fcode */ + 2 /* addr */ + 2 /* num */;

    return memcpy(reply, begin, size) + size;
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
    TLOG_PRINTF("S%02" PRIX8 " F%02" PRIX8, addr, fcode);

    if(RTU_ADDR != addr) goto exit;

    *dst_begin++ = addr;

    if(FCODE_RD_HOLDING_REGISTERS == fcode)
    {
        dst_begin = read_n16(begin + sizeof(addr), end, curr, dst_begin, user_data);
    }
    else if(FCODE_WR_REGISTER == fcode)
    {
        dst_begin = write_16(begin + sizeof(addr), end, curr, dst_begin, user_data);
    }
    else if(FCODE_WR_REGISTERS == fcode)
    {
        dst_begin = write_n16(begin + sizeof(addr), end, curr, dst_begin, user_data);
    }
    else
    {
        dst_begin = except(fcode, ILLEGAL_FUNCTION, dst_begin);
    }
exit:
    return dst_begin;
}
