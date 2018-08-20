#!/bin/bash

export ARCH=arm
export SUBARCH=arm
export CROSS_COMPILE=~/kernel-3.10/arm-eabi-4.8/bin/arm-eabi-
make O=out TARGET_ARCH=arm hexing6572_wet_l_defconfig
make O=out TARGET_ARCH=arm | tee build.log
