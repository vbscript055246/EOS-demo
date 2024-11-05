#!/bin/sh

set -x
# set -e

rmmod -f mydev
insmod mydev.ko

./writer Dirac &
./reader 192.168.0.87 8888 /dev/mydev
