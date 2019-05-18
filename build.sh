#!/bin/bash

. "./check_bin_size.sh"

set -e

export TYPE=prod # see u-boot-2009.08/board/imx50_yoshi/config.mk
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf- # change this if you want to use a different cross-compiler

make imx50_yoshi_config
make all

checkBinarySize "u-boot.bin"
