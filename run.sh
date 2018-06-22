#!/bin/sh

sudo rmmod hellomodule.ko
rm test
gcc -o test test.c

make

sudo insmod hellomodule.ko
