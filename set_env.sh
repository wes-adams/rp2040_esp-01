#!/bin/bash

export PATH=$PATH:/home/wadams/Downloads/arm/gcc-arm-11.2-2022.02-x86_64-arm-none-eabi/bin
export PICO_SDK_PATH=/home/wadams/Code/rp2040/pico-sdk/
echo "PATH = $PATH"
echo

cd build && cmake .. && make
if [ $? -ne 0 ];then
    echo "compilation failed"
    exit
fi

exit
