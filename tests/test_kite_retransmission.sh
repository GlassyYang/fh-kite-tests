
source ./test_common.sh

NFD3_INIT="nfdc route add /kite-test/rv tcp://${NFD2_ADDR}:6363"
NFD1_INIT="nfdc route add /kite-test/rv tcp://${NFD2_ADDR}:6363"
NFD4_INIT="nfdc route add /kite-test/rv tcp://${NFD2_ADDR}:6363"

# start rv
NFD2_INIT="NDN_LOG=kite.*=ALL kiterv /kite-test/rv"

NFD1_TEST="ndnpeek $TEST_FILE$ -v -w 4000 -r /kite-test/rv"

NFD3_TEST="NDN_LOG=kite.*=ALL kiteproducer -r /kite-test/rv -p /kite-test/alice -l 1000000 -a 1 >nfd3.log 2>&1"
NFD4_TEST="NDN_LOG=kite.*=ALL kiteproducer -r /kite-test/rv -p /kite-test/alice -l 1000000 >nfd4.log 2>&1"


NFD3_CLEAR="\
nfdc cs erase / &&\
nfdc route remove /kite-test/rv tcp://${NFD2_ADDR}:6363 && \
${KILL_PRODUCER}
"

NFD4_CLEAR="\
nfdc cs erase / &&\
nfdc route remove /kite-test/rv tcp://${NFD2_ADDR}:6363 && \
${KILL_PRODUCER}
"

NFD2_CLEAR="
killall kiterv && \
nfdc cs erase / \
"

NFD1_CLEAR="
nfdc cs erase / && \
nfdc route remove /kite-test/rv tcp://${NFD2_ADDR}:6363 \
"

init() {
    execute NFD1 "${NFD1_INIT}"
    execute NFD3 "${NFD3_INIT}"
    execute NFD4 "${NFD4_INIT}"

    execute NFD2 "${NFD2_INIT}" &
}

test() {
    execute NFD3 "${NFD3_TEST}" &
    execute NFD4 "${NFD4_TEST}" &

    execute NFD3 "${SEND_PRODUCER_SIGNAL}"

    res=$(execute NFD1 "${NFD1_TEST}")
    
    
    execute NFD4 "${SEND_PRODUCER_SIGNAL}"
    
    route_b=`execute NFD2 "nfdc route" | grep "/kite-test/alice"`

    sleep 2

    res3=`execute NFD3 "cat nfd3.log"`
    res4=`execute NFD4 "cat nfd4.log"`
    
    # check route on NFD2
    route_a=`execute NFD2 "nfdc route" | grep "/kite-test/alice"`
}

clear() {
    execute NFD1 "${NFD1_CLEAR}"
    execute NFD2 "${NFD2_CLEAR}"
    execute NFD3 "${NFD3_CLEAR}"
    execute NFD4 "${NFD4_CLEAR}"
}

start_nfd
init
test
clear
echo
echo '-------------------------------------------------------'
if [[ "$res3" =~ "Received consumer Interest" && "$res4" =~ "Received consumer Interest" ]]; then
    if [[ "$res" =~ "DATA" ]]; then
        echo "test succeed"
    else
        echo "test failed! consumer doesn't receive packet: $res"
    fi
else  
    echo "test failed! kite producer doesn't receive packet:" >&2
    echo "kiteproducer on NFD3: $res3"
    echo "kiteproducer on NFD4: $res4"
fi
echo "route before send Interest: ${route_b}"
echo "route after send Interest: ${route_a}"