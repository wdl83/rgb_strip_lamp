BOOTLOADER=../bootloader
DRV_DIR=../atmega328p_drv
MODBUS_C=../modbus_c
WS2812B_STRIP=../ws2812b_strip

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
		  -DHEARTBEAT_PERIOD=4096 \
		  -DRTU_ADDR_BASE=0x1000 \
		  -DRTU_ERR_REBOOT_THREASHOLD=16 \
		  -DSPI0_ISR_ENABLE \
		  -DSTRIP_HEIGHT=20 \
		  -DSTRIP_SIZE=120 \
		  -DSTRIP_STRIDE=6 \
		  -DSTRIP_WIDTH=6 \
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
		$(BOOTLOADER)/fixed.c \
		$(MODBUS_C)/atmega328p/rtu_impl.c \
		$(MODBUS_C)/crc.c \
		$(MODBUS_C)/rtu.c \
		$(MODBUS_C)/rtu_memory.c \
		$(WS2812B_STRIP)/energy.c \
		$(WS2812B_STRIP)/fire.c \
		$(WS2812B_STRIP)/fx.c \
		$(WS2812B_STRIP)/noise.c \
		$(WS2812B_STRIP)/palette.c \
		$(WS2812B_STRIP)/rgb.c \
		$(WS2812B_STRIP)/torch.c \
		$(WS2812B_STRIP)/util.c \
		$(WS2812B_STRIP)/ws2812b.c \
		panic.c \
		rtu_cmd.c \
		sol.c 

LDFLAGS += \
		   -Wl,-T ../bootloader/atmega328p.ld

ifdef RELEASE
	CFLAGS +=  \
		-DASSERT_DISABLE
endif

include $(DRV_DIR)/Makefile.rules

clean:
	cd $(DRV_DIR) && make clean
	cd $(BOOTLOADER) && make clean
	cd $(WS2812B_STRIP) && make clean
	rm *.bin *.elf *.hex *.lst *.map *.o *.su *.stack_usage -f
