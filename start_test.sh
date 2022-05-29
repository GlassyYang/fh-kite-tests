vagrant box add hashicorp/bionic64
vagrant up
vagrant ssh <<ENDSCRIPT
cd /home/compile
./install.sh
ENDSCRIPT
vagrant package
vagrant box add package.box --name glassyyang/kite-test-env
cd tests
./test.sh
