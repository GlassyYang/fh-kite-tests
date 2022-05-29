# KiteStrategy tests
These are tests for KiteStrategy which is built into NFD for supporting forwarding hint based KITE.
## Requirements
```bash
vagrant>=2.2.19
virtualbox>=6.1.34
```
## How to run tests
### package base box
Use following commands to package base box:
```shell
vagrant box add hashicorp/bionic64
vagrant up
vagrant ssh <<ENDSCRIPT
cd /home/compile
./install.sh
ENDSCRIPT
vagrant package
vagrant box add package.box --name glassyyang/kite-test-env
```
### Run tests
Go into `tests` folder and run `test.sh` to run all tests. Also can run `test_*.sh` except `test_common.dh` manually to
run single test.
## Test cases.
Following are test cases these tests covered.
### `test_normal.sh`
