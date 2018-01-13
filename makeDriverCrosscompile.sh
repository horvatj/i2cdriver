#!/bin/bash

make clean

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- KDIR=../kernel/linux-ti_3.12.30-phy2-ro/

