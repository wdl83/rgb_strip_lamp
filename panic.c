#include <avr/interrupt.h>
#include <drv/usart0.h>
#include <drv/watchdog.h>

static
void panic(const char *src)
{
#if 0
    USART0_TX_DISABLE();
    USART0_RX_DISABLE();
    USART0_BR(0);
    USART0_BR(CALC_BR(CPU_CLK, 19200));
    USART0_PARITY_EVEN();
    USART0_TX_ENABLE();

    for(;;)
    {
        usart0_send_str("panic[");
        usart0_send_str(src);
        usart0_send_str("]\n");
    }
#else
    watchdog_enable(WATCHDOG_TIMEOUT_16ms);
    for(;;) {/* wait until reset */}
#endif
}

ISR(INT0_vect) { panic("INT0"); }
ISR(INT1_vect) { panic("INT1"); }
ISR(PCINT0_vect) { panic("PCINT0"); }
ISR(PCINT1_vect) { panic("PCINT1"); }
ISR(PCINT2_vect) { panic("PCINT2"); }
ISR(WDT_vect) { panic("WDT"); }
ISR(ADC_vect) { panic("ADC"); }
ISR(EE_READY_vect) { panic("EE_READY"); }
ISR(ANALOG_COMP_vect) { panic("ANALOG_COMP"); }
ISR(TWI_vect) { panic("TWI"); }
ISR(SPM_READY_vect) { panic("SPM_READY"); }

#if 0
/* Interrupt Vector 0 is the reset vector. */
#define INT0_vect_num     1
#define INT0_vect         _VECTOR(1)   /* External Interrupt Request 0 */
#define INT1_vect_num     2
#define INT1_vect         _VECTOR(2)   /* External Interrupt Request 1 */
#define PCINT0_vect_num   3
#define PCINT0_vect       _VECTOR(3)   /* Pin Change Interrupt Request 0 */
#define PCINT1_vect_num   4
#define PCINT1_vect       _VECTOR(4)   /* Pin Change Interrupt Request 0 */
#define PCINT2_vect_num   5
#define PCINT2_vect       _VECTOR(5)   /* Pin Change Interrupt Request 1 */
#define WDT_vect_num      6
#define WDT_vect          _VECTOR(6)   /* Watchdog Time-out Interrupt */
#define TIMER2_COMPA_vect_num 7
#define TIMER2_COMPA_vect _VECTOR(7)   /* Timer/Counter2 Compare Match A */
#define TIMER2_COMPB_vect_num 8
#define TIMER2_COMPB_vect _VECTOR(8)   /* Timer/Counter2 Compare Match A */
#define TIMER2_OVF_vect_num   9
#define TIMER2_OVF_vect   _VECTOR(9)   /* Timer/Counter2 Overflow */
#define TIMER1_CAPT_vect_num  10
#define TIMER1_CAPT_vect  _VECTOR(10)  /* Timer/Counter1 Capture Event */
#define TIMER1_COMPA_vect_num 11
#define TIMER1_COMPA_vect _VECTOR(11)  /* Timer/Counter1 Compare Match A */
#define TIMER1_COMPB_vect_num 12
#define TIMER1_COMPB_vect _VECTOR(12)  /* Timer/Counter1 Compare Match B */
#define TIMER1_OVF_vect_num   13
#define TIMER1_OVF_vect   _VECTOR(13)  /* Timer/Counter1 Overflow */
#define TIMER0_COMPA_vect_num 14
#define TIMER0_COMPA_vect _VECTOR(14)  /* TimerCounter0 Compare Match A */
#define TIMER0_COMPB_vect_num 15
#define TIMER0_COMPB_vect _VECTOR(15)  /* TimerCounter0 Compare Match B */
#define TIMER0_OVF_vect_num  16
#define TIMER0_OVF_vect   _VECTOR(16)  /* Timer/Couner0 Overflow */
#define SPI_STC_vect_num  17
#define SPI_STC_vect      _VECTOR(17)  /* SPI Serial Transfer Complete */
#define USART_RX_vect_num 18
#define USART_RX_vect     _VECTOR(18)  /* USART Rx Complete */
#define USART_UDRE_vect_num   19
#define USART_UDRE_vect   _VECTOR(19)  /* USART, Data Register Empty */
#define USART_TX_vect_num 20
#define USART_TX_vect     _VECTOR(20)  /* USART Tx Complete */
#define ADC_vect_num      21
#define ADC_vect          _VECTOR(21)  /* ADC Conversion Complete */
#define EE_READY_vect_num 22
#define EE_READY_vect     _VECTOR(22)  /* EEPROM Ready */
#define ANALOG_COMP_vect_num  23
#define ANALOG_COMP_vect  _VECTOR(23)  /* Analog Comparator */
#define TWI_vect_num      24
#define TWI_vect          _VECTOR(24)  /* Two-wire Serial Interface */
#define SPM_READY_vect_num    25
#define SPM_READY_vect    _VECTOR(25)  /* Store Program Memory Read */
#define _VECTORS_SIZE (26 * 4)
#endif
