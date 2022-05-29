source ./test_common.sh

set -x

NFD3_INIT="nfdc route add /kite-test/rv tcp://${NFD2_ADDR}:6363"
NFD1_INIT="nfdc route add /kite-test/rv tcp://${NFD2_ADDR}:6363"

NFD2_INIT="NDN_LOG=kite.*=ALL kiterv /kite-test/rv "

NFD1_TEST="NDN_LOG=kite.*=ALL kiteproducer -r /kite-test/rv -p /kite-test/alice -l 1000000"

NFD3_TEST="ndnpeek $TEST_FILE$ -v -w 2000 -r /kite-test/rv"

NFD1_CLEAR="\
nfdc cs erase / &&\
nfdc route remove /kite-test/rv tcp://${NFD2_ADDR}:6363 && \
pkill -9 kiteproducer
"

NFD2_CLEAR="
killall kiterv && \
nfdc cs erase / \
"

NFD3_CLEAR="
nfdc cs erase / && \
nfdc route remove /kite-test/rv tcp://${NFD2_ADDR}:6363 \
"

init() {
    execute NFD3 "${NFD3_INIT}"
    execute NFD1 "${NFD1_INIT}"
    execute NFD2 "${NFD2_INIT}" &
}

test() {
    execute NFD1 "${NFD1_TEST}" &
    execute NFD1 "${SEND_PRODUCER_SIGNAL}"
    res=$(execute NFD3 "${NFD3_TEST}")
}

clear() {
    execute NFD1 "$NFD1_CLEAR"
    execute NFD2 "$NFD2_CLEAR"
    execute NFD3 "$NFD3_CLEAR" 
}

start_nfd
init
test
clear
echo
echo '----------------------------------------------'
if [[ "$res" =~ "DATA" ]]; then
    echo "test succeed"
else
    echo "test failed! consumer doesn't receive packet: $res"
fi