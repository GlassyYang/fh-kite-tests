#!/bin/bash

source ./test_common.sh

# set -x

NFD1_INIT="nfdc route add /kite-test/rv tcp://${NFD2_ADDR}:6363"
NFD2_INIT="NDN_LOG=kite.*=ALL kiterv /kite-test/rv -m /kite-test/alice -l >nfd2.log 2>&1"
NFD3_INIT="nfdc route add /kite-test/rv tcp://${NFD2_ADDR}:6363"

NFD1_TEST="ndnpeek $TEST_FILE$ -v -w 2000 -r /kite-test/rv"
NFD3_TEST="NDN_LOG=kite.*=ALL kiteproducer -r /kite-test/rv -p /kite-test/alice -l 1000000 -a 1 >nfd3.log 2>&1"

NFD1_CLEAR="
nfdc cs erase / && \
nfdc route remove /kite-test/rv tcp://${NFD2_ADDR}:6363
"

NFD2_CLEAR="
nfdc cs erase / && \
${KILL_RV}
"

NFD3_CLEAR="
nfdc cs erase / && \
nfdc route remove /kite-test/rv tcp://${NFD2_ADDR}:6363 && \
${KILL_PRODUCER}
"

init() {
    execute NFD1 "${NFD1_INIT}"
    execute NFD3 "${NFD3_INIT}"
    
    execute NFD2 "${NFD2_INIT}" &
}

test() {
    execute NFD3 "${NFD3_TEST}" &
    execute NFD3 "${SEND_PRODUCER_SIGNAL}"
    execute NFD1 "${NFD1_TEST}"
    sleep 2
    rvout=`execute NFD2 "cat nfd2.log"`
    producerout=`execute NFD3 "cat nfd3.log"`
}

clear() {
    execute NFD1 "${NFD1_CLEAR}"
    execute NFD2 "${NFD2_CLEAR}"
    execute NFD3 "${NFD3_CLEAR}"
}

start_nfd
init
test
clear
echo
echo '----------------------------'

if [[ "$rvout" =~ "receive mp interest" && "$producerout" =~ "Received consumer Interest" ]]; then
    echo "test succeed"
else
    echo "test failed! rv or producer cannot receive interest!"
    echo "rv output: $rvout"
    echo "producer output: $producerout"
fi