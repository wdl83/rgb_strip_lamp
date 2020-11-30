DRV_DIR=../atmega328p_drv

CPPFLAGS += -I.. -I$(DRV_DIR)

include Makefile-host.defs

DEFS = \
		  -DSTRIP_HEIGHT=20 \
		  -DSTRIP_WIDTH=20 \
		  -DSTRIP_STRIDE=20 \
		  -Dmap_size_t=uint16_t

CFLAGS += $(DEFS)
CXXFLAGS += $(DEFS)

TARGET = sol-host

CSRCS = \
		../ws2812b_strip/fire.c \
		../ws2812b_strip/fx.c \
		../ws2812b_strip/palette.c \
		../ws2812b_strip/rgb.c \
		../ws2812b_strip/torch.c

CXXSRCS = \
		sol-host.cpp

include Makefile-host.rules
