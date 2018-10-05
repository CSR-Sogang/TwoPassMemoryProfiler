#!/bin/bash

LD_PRELOAD=../libdcalltrack.so ./test1 > res1 
LD_PRELOAD=../libdnvm.so ./test1 > res2
