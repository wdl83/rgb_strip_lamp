# !/bin/bash

DRV=atmega328p_drv
BOOTLOADER=bootloader
MODBUS_C=modbus_c
WS2812B_STRIP=ws2812b_strip

if [ ! -d ${BOOTLOADER} ]
then
    git clone https://github.com/wdl83/bootloader ${BOOTLOADER}
fi

if [ ! -d ${DRV} ]
then
    git clone https://github.com/wdl83/atmega328p_drv ${DRV}
fi

if [ ! -d ${MODBUS_C} ]
then
    git clone https://github.com/wdl83/modbus_c ${MODBUS_C}
fi

if [ ! -d ${WS2812B_STRIP} ]
then
    git clone https://github.com/wdl83/ws2812b_strip ${WS2812B_STRIP}
fi
