#!/bin/sh
gcc -Wall -O3 -DAT_DEBUG_FULL -DAT_DEBUG_ENABLE \
    -o Default  default.c atchannel.c at_tok.c \
    -lpthread -ldl -lrt