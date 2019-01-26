#!/bin/sh

gcc -o poc poc.c --static
cd lkm/
make
cd ../
