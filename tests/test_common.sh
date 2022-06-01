#!/bin/bash

NFD1_ADDR=192.168.56.2
NFD2_ADDR=192.168.56.3
NFD3_ADDR=192.168.56.4
NFD4_ADDR=192.168.56.5

execute() {
    vagrant ssh "$1" -c "$2" $3
}

start_nfd() {
    execute NFD1 "nohup nfd-start"
    execute NFD2 "nohup nfd-start"
    execute NFD3 "nohup nfd-start"
    execute NFD4 "nohup nfd-start"
}

stop_nfd() {
    execute NFD1 "nfd-stop"
    execute NFD2 "nfd-stop"
    execute NFD3 "nfd-stop"
}

KILL_PRODUCER="pkill -9 kiteproducer"
# send SIGINT to kite producer
SEND_PRODUCER_SIGNAL="pkill -2 kiteproducer"
KILL_RV="pkill -9 kiterv"

TEST_FILE="/kite-test/alice/photos/selfie.png"
