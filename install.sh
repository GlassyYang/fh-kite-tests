#!/bin/bash
sudo apt update
sudo apt install -y build-essential pkg-config python3-minimal libboost-all-dev libssl-dev libsqlite3-dev libpcap-dev libsystemd-dev
cd kite-ndn-cxx
./waf configure
./waf
sudo ./waf install
./waf distclean
sudo ldconfig
cd ../kite-NFD
./waf configure
./waf
sudo ./waf install
./waf distclean
cd ../kite-ndn-tools
./waf configure
./waf
sudo ./waf install
./waf distclean
ndnsec delete /kite-test/rv
ndnsec delete /kite-test/alice
ndnsec import -i /usr/local/etc/ndn/rv-test.safebag -P 1
ndnsec import -i /usr/local/etc/ndn/alice-test.safebag -P 1
ndnsec cert-install /usr/local/etc/ndn/alice-test.cert
ndnsec cert-install /usr/local/etc/ndn/rv-test.cert

