#!/bin/bash
cd build && cmake .. && make clean && make
if [ $? -ne 0 ];then
    echo "compilation failed"
    exit
fi
