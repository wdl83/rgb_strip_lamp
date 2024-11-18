#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <hw.h>

#include <drv/assert.h>
#include <drv/spi0.h>
#include <drv/tlog.h>
#include <drv/tmr1.h>
#include <drv/tmr2.h>
#include <drv/watchdog.h>

#include <bootloader/fixed.h>

#include <modbus_c/atmega328p/rtu_impl.h>
#include <modbus_c/rtu.h>

#include <ws2812b_strip/ws2812b.h>

#include "rtu_cmd.h"

/*-----------------------------------------------------------------------------*/
static
void cyclic_tmr_cb(uintptr_t user_data)
{
    rtu_memory_fields_t *rtu_memory_fields = (rtu_memory_fields_t *)user_data;
    ws2812b_strip_t *strip = &rtu_memory_fields->ws2812b_mmap.strip;

    /* prevent heartbeat underflow */
    if(rtu_memory_fields->heartbeat) --rtu_memory_fields->heartbeat;

    if(!strip->flags.abort && strip->flags.updated)
    {
        strip->flags.updated = 0;
        strip->flags.update = 1;
    }
}

static
void cyclic_tmr_start(rtu_memory_fields_t *rtu_memory_fields, timer_cb_t cb)
{
    timer1_cb(cb, (uintptr_t)rtu_memory_fields);
    /* Clear Timer on Compare */
    TMR1_MODE_CTC();
    TMR1_WR16_A(rtu_memory_fields->tmr1_A);
    TMR1_WR16_CNTR(0);
    TMR1_A_INT_ENABLE();
    TMR1_CLK_DIV_64();
}

static
void cyclic_tmr_stop(void)
{
    TMR1_CLK_DISABLE();
    TMR1_A_INT_DISABLE();
    TMR1_A_INT_CLEAR();
    timer1_cb(NULL, 0);
}

static
void cyclic_tmr_update(rtu_memory_fields_t *rtu_memory_fields)
{
    if(rtu_memory_fields->tmr1_A == TMR1_RD16_A()) return;

    cyclic_tmr_stop();
    cyclic_tmr_start(rtu_memory_fields, cyclic_tmr_cb);
}
/*-----------------------------------------------------------------------------*/
static
void fx_none(rtu_memory_fields_t *rtu_memory_fields)
{
    ws2812b_strip_t *strip = &rtu_memory_fields->ws2812b_mmap.strip;

    ws2812b_update(strip);
}

static
void fx_static(rtu_memory_fields_t *rtu_memory_fields)
{
    ws2812b_strip_t *strip = &rtu_memory_fields->ws2812b_mmap.strip;

    ws2812b_update(strip);
}

static
void fx_fire(rtu_memory_fields_t *rtu_memory_fields)
{
    ws2812b_strip_t *strip = &rtu_memory_fields->ws2812b_mmap.strip;

    fx_calc_fire(&strip->rgb_map, rtu_memory_fields->fx_mmap.raw);
    ws2812b_update(strip);
}

static
void fx_torch(rtu_memory_fields_t *rtu_memory_fields)
{
    ws2812b_strip_t *strip = &rtu_memory_fields->ws2812b_mmap.strip;

    fx_calc_torch(&strip->rgb_map, rtu_memory_fields->fx_mmap.raw);
    ws2812b_update(strip);
}

static
void fx_noise(rtu_memory_fields_t *rtu_memory_fields)
{
    ws2812b_strip_t *strip = &rtu_memory_fields->ws2812b_mmap.strip;

    fx_calc_noise(&strip->rgb_map, rtu_memory_fields->fx_mmap.raw);
    ws2812b_update(strip);
}
/*-----------------------------------------------------------------------------*/
static
void suspend(uintptr_t user_data)
{
    /* suspend ws2812b strip update - as this is the only heavy duty task
     * cyclic time can keep running */
    rtu_memory_fields_t *rtu_memory_fields = (rtu_memory_fields_t *)user_data;
    ws2812b_strip_t *strip = &rtu_memory_fields->ws2812b_mmap.strip;

    strip->flags.abort = 1;
}

static
void resume(uintptr_t user_data)
{
    rtu_memory_fields_t *rtu_memory_fields = (rtu_memory_fields_t *)user_data;
    ws2812b_strip_t *strip = &rtu_memory_fields->ws2812b_mmap.strip;

    if(strip->flags.aborted) ws2812b_update(strip);
    else if(strip->flags.abort) strip->flags.abort = 0;
}
/*-----------------------------------------------------------------------------*/
static
void handle_heartbeat(rtu_memory_fields_t *rtu_memory_fields)
{
    if(rtu_memory_fields->heartbeat) return;
    rtu_memory_fields->heartbeat = HEARTBEAT_PERIOD;
    //TLOG_TP();

    //watchdog_enable(WATCHDOG_TIMEOUT_16ms);
    //for(;;) {/* wait until reset */}
}

static
void handle_rtu_state(
    modbus_rtu_state_t *state,
    rtu_memory_fields_t *rtu_memory_fields)
{
    if(rtu_memory_fields->rtu_err_reboot_threashold > state->stats.err_cntr) return;
    fixed__.app_reset_code.curr = RESET_CODE_RTU_ERROR;
    watchdog_disable();
    watchdog_enable(WATCHDOG_TIMEOUT_16ms);
    for(;;) {/* wait until reset */}
}

static
void handle_reboot(rtu_memory_fields_t *rtu_memory_fields)
{
    if(!rtu_memory_fields->reboot) return;

    rtu_memory_fields->reboot = 0;
    fixed__.app_reset_code.curr = RESET_CODE_REBOOT;
    watchdog_disable();
    watchdog_enable(WATCHDOG_TIMEOUT_250ms);
    sei(); /* USART0 async transmission in progress - Modbus reply */
    for(;;) {/* wait until reset */}
}

static
void handle_strip(rtu_memory_fields_t *rtu_memory_fields)
{
    if(!rtu_memory_fields->strip_updated) return;
    rtu_memory_fields->strip_updated = 0;
#if 0
    cyclic_tmr_update(rtu_memory_fields);
#endif
    ws2812b_strip_t *strip = &rtu_memory_fields->ws2812b_mmap.strip;

    strip->flags.fx = rtu_memory_fields->strip_fx;

    if(FX_NONE == strip->flags.fx)
    {
        ws2812b_clear(strip);
        ws2812b_power_off(strip);
    }
    else
    {
        if(FX_STATIC == strip->flags.fx)
        {
            /* WARNING: HW limit (power supply) */
            strip->rgb_map.brightness =
                UINT8_C(0x80) > strip->rgb_map.target_brightness
                ? strip->rgb_map.target_brightness
                : 0x7F;

            ws2812b_apply_correction(strip);
        }
        else ws2812b_clear(strip);

        if(FX_FIRE == strip->flags.fx)
        {
            fx_init_fire(rtu_memory_fields->fx_mmap.raw);
        }
        else if(FX_TORCH == strip->flags.fx)
        {
            fx_init_torch(rtu_memory_fields->fx_mmap.raw);
        }
        else if(FX_NOISE == strip->flags.fx)
        {
            fx_init_noise(rtu_memory_fields->fx_mmap.raw);
        }

        ws2812b_power_on(strip);
    }

    strip->flags.updated = 1;
}

static
void dispatch_uninterruptible(
    modbus_rtu_state_t *state,
    rtu_memory_fields_t *rtu_memory_fields)
{
    handle_rtu_state(state, rtu_memory_fields);
    handle_heartbeat(rtu_memory_fields);
    handle_reboot(rtu_memory_fields);
    handle_strip(rtu_memory_fields);
}

static
void dispatch_interruptible(rtu_memory_fields_t *rtu_memory_fields)
{
    ws2812b_strip_t *strip = &rtu_memory_fields->ws2812b_mmap.strip;

    if(!strip->flags.update) return;
    else strip->flags.update = 0;

    sei();

    if(FX_NONE == strip->flags.fx) fx_none(rtu_memory_fields);
    else if(FX_STATIC == strip->flags.fx) fx_static(rtu_memory_fields);
    else if(FX_FIRE == strip->flags.fx) fx_fire(rtu_memory_fields);
    else if(FX_TORCH == strip->flags.fx) fx_torch(rtu_memory_fields);
    else if(FX_NOISE == strip->flags.fx) fx_noise(rtu_memory_fields);
}
/*-----------------------------------------------------------------------------*/
__attribute__((noreturn))
void main(void)
{
    /* watchdog is enabled by bootloader whenever it "jumps" to app code */
    fixed__.app_reset_code.curr = RESET_CODE_APP_IDLE;
    watchdog_disable();
    watchdog_enable(WATCHDOG_TIMEOUT_1000ms);

    hw_init(eeprom_read_byte((const uint8_t *)EEPROM_ADDR_HW_INFO));

    rtu_memory_fields_t rtu_memory_fields;
    modbus_rtu_state_t state;

    rtu_memory_fields_clear(&rtu_memory_fields);
    rtu_memory_fields_init(&rtu_memory_fields);
    tlog_init(rtu_memory_fields.tlog);

    TLOG_XPRINT2x8("MCUSR|RSTCNT", fixed__.mcusr, fixed__.reset_counter);
    TLOG_XPRINT2x8("B|ARSTC", fixed__.bootloader_reset_code.value, fixed__.app_reset_code.value);
    TLOG_XPRINT2x8("PNC|APPCNT", fixed__.panic_counter, fixed__.app_counter);
    TLOG_XPRINT16(
        "TLOG",
        STRUCT_OFFSET(rtu_memory_fields_t, tlog)
        - STRUCT_OFFSET(rtu_memory_fields_t, header));

    ws2812b_init(&rtu_memory_fields.ws2812b_mmap.strip);
    modbus_rtu_impl(
        &state,
        eeprom_read_byte((const uint8_t *)EEPROM_ADDR_RTU_ADDR),
        suspend, resume,
        rtu_pdu_cb,
        (uintptr_t)&rtu_memory_fields);
    cyclic_tmr_start(&rtu_memory_fields, cyclic_tmr_cb);

    /* set SMCR SE (Sleep Enable bit) */
    sleep_enable();

    for(;;)
    {
        cli(); // disable interrupts
        watchdog_reset();
        modbus_rtu_event(&state);

        if(modbus_rtu_idle(&state))
        {
            dispatch_uninterruptible(&state, &rtu_memory_fields);
            dispatch_interruptible(&rtu_memory_fields);
        }
        sei();
        sleep_cpu();
    }
}
/*-----------------------------------------------------------------------------*/
