include ../Makefile.defs

# HW resources in use:
#
# TMR0 - modbus state machine
# TMR1 - simulation updater
# TMR2 - WS2812B strip update
# SPI0 - WS2812B strip update

# -DUSART_DBG_CNTRS
CFLAGS += \
		  -DEEPROM_ADDR_RTU_ADDR=0x0 \
		  -DRTU_ADDR_BASE=0x1000 \
		  -DSTRIP_HEIGHT=20 \
		  -DSTRIP_STRIDE=6 \
		  -DSTRIP_WIDTH=6 \
		  -DSTRIP_SIZE=120 \
		  -DTLOG_SIZE=256 \
		  -Dmap_size_t=uint8_t

TARGET = sol
CSRCS = \
		../drv/spi0.c \
		../drv/tlog.c \
		../drv/tmr0.c \
		../drv/tmr1.c \
		../drv/tmr2.c \
		../drv/usart0.c \
		../hw.c \
		../modbus-c/atmega328p/rtu_impl.c \
		../modbus-c/crc.c \
		../modbus-c/rtu.c \
		../modbus-c/rtu_memory.c \
		../ws2812b/fire.c \
		../ws2812b/fx.c \
		../ws2812b/palette.c \
		../ws2812b/rgb.c \
		../ws2812b/torch.c \
		../ws2812b/ws2812b.c \
		rtu_cmd.c \
		sol.c 

ifdef RELEASE
	CFLAGS +=  \
		-DASSERT_DISABLE
else
endif

include ../Makefile.rules
