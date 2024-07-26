#!/bin/sh
gcc -DAT_DEBUG_FULL -DAT_DEBUG_ENABLE \
    -o app main.c  request_proc.c http_server.c util/log.c \
    atc/cellMgtFrame.c atc/atchannel.c atc/at_tok.c \
    -lpthread -ldl -lrt -luv -ljansson -lhttp_parser

echo "root" | sudo -S ./app /dev/ttyUSB2