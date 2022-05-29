source ./test_common.sh

vagrant up

start_nfd
./test_normal.sh
./test_kite_base.sh
./test_kite_multi.sh
./test_kite_nack.sh
./test_kite_retransmission.sh
./test_search_route.sh