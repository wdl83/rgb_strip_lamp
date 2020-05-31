include ../Makefile.defs

# HW resources in use:
#
# TMR0 - modbus state machine
# TMR1 - simulation updater
# TMR2 - WS2812B strip update
# SPI0 - WS2812B strip update

# -DUSART_DBG_CNTRS
CFLAGS += \
		  -DRTU_ADDR=128 \
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
		../modbus-c/crc.c \
		../modbus-c/rtu.c \
		../ws2812b/fire.c \
		../ws2812b/fx.c \
		../ws2812b/palette.c \
		../ws2812b/rgb.c \
		../ws2812b/torch.c \
		../ws2812b/ws2812b.c \
		rtu_cmd.c \
		rtu_impl.c \
		sol.c 

include ../Makefile.rules
