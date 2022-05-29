#!/bin/bash

source ./test_common.sh

NFD1_INIT="\
nfdc route add /kite-test/rv tcp://${NFD2_ADDR}:6363
"
NFD2_INIT="\
nfdc route add /kite-test/rv tcp://${NFD3_ADDR}:6363
"

NFD3_INIT="\
nohup sudo nfd-start \
"

NFD1_TEST="\
ndnpeek -p /kite-test/rv/test
"

NFD3_TEST="\
echo \"TEST\" | ndnpoke /kite-test/rv/test
"

NFD1_CLEAR="\
nfdc cs erase / && \
nfdc route remove /kite-test/rv tcp://${NFD2_ADDR}:6363\
"

NFD2_CLEAR="\
nfdc cs erase / && \
nfdc route remove /kite-test/rv tcp://${NFD3_ADDR}:6363\
"

NFD3_CLEAR="\
nfdc cs erase /
killall ndnpoke
"

init() {
    execute NFD3  "${NFD3_INIT}"
    execute NFD2  "${NFD2_INIT}" 
    execute NFD1  "${NFD1_INIT}" 
}

clear() {
    execute NFD1  "${NFD1_CLEAR}" 
    execute NFD2  "${NFD2_CLEAR}"
    execute NFD3  "${NFD3_CLEAR}" 
}

test_normal() {
    execute NFD3  "${NFD3_TEST}" &
    sleep 1
    res=$(execute NFD1  "${NFD1_TEST}")
    echo $res
}

start_nfd
init
test_normal
clear
if [[ "$res" =~ "TEST" ]]; then
    echo "test succeed"
else    
    echo "test failed! with reason: ${res}" >&2
fi