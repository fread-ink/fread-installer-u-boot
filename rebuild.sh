#!/bin/bash

export TYPE=prod # see u-boot-2009.08/board/imx50_yoshi/config.mk
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf- # change this if you want to use a different cross-compiler

make clean
make distclean
rm -rf u-boot.*

./build.sh

