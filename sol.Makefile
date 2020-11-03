DRV_DIR=../atmega328p_drv

CPPFLAGS += -I..
CPPFLAGS += -I$(DRV_DIR)

include $(DRV_DIR)/Makefile.defs

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
		$(DRV_DIR)/drv/spi0.c \
		$(DRV_DIR)/drv/tlog.c \
		$(DRV_DIR)/drv/tmr0.c \
		$(DRV_DIR)/drv/tmr1.c \
		$(DRV_DIR)/drv/tmr2.c \
		$(DRV_DIR)/drv/usart0.c \
		$(DRV_DIR)/drv/util.c \
		$(DRV_DIR)/hw.c \
		../modbus_c/atmega328p/rtu_impl.c \
		../modbus_c/crc.c \
		../modbus_c/rtu.c \
		../modbus_c/rtu_memory.c \
		../ws2812b_strip/fire.c \
		../ws2812b_strip/fx.c \
		../ws2812b_strip/palette.c \
		../ws2812b_strip/rgb.c \
		../ws2812b_strip/torch.c \
		../ws2812b_strip/ws2812b.c \
		panic.c \
		rtu_cmd.c \
		sol.c 

ifdef RELEASE
	CFLAGS +=  \
		-DASSERT_DISABLE
else
endif

include $(DRV_DIR)/Makefile.rules
