#!/bin/bash
./stencilgen $@ > /dev/null 2>&1
if [ $? -ne 0 ];
then
    echo "Generation Error!"
    exit 0
fi
make hhz > /dev/null 2>&1
if [ $? -ne 0 ];
then
    echo "Compilation Error!"
    exit 0
fi
nvprof --csv --profile-api-trace none -u ms ./a.out 2>&1 \
    | grep -v "\[CUDA memcpy " | grep -P "([[:digit:].]+,){6}" \
    | cut -d, -f 6 | python ~/python/calc.py;
if [ ${PIPESTATUS[0]} -ne 0 ];
then
    echo "Run Error!"
    exit 0
fi
