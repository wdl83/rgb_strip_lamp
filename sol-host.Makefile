include ../Makefile-host.defs

DEFS = \
		  -DSTRIP_HEIGHT=20 \
		  -DSTRIP_WIDTH=20 \
		  -DSTRIP_STRIDE=20 \
		  -Dmap_size_t=uint16_t

CFLAGS += $(DEFS)
CXXFLAGS += $(DEFS)

TARGET = sol-host

CSRCS = \
		../ws2812b/fire.c \
		../ws2812b/fx.c \
		../ws2812b/palette.c \
		../ws2812b/rgb.c \
		../ws2812b/torch.c

CXXSRCS = \
		sol-host.cpp

include ../Makefile-host.rules
