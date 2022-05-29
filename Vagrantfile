# -*- mode: ruby -*-
# vi: set ft=ruby :

$setup_ssh = <<SCRIPT
mkdir -p ~/.ssh
cp /vagrant/temp/sshkey ~/.ssh/id_rsa
cp /vagrant/temp/sshkey.pub ~/.ssh/id_rsa.pub
cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys
chown -R vagrant:vagrant ~/.ssh
chmod -R 700 ~/.ssh
SCRIPT

Vagrant.configure("2") do |config|
  
  config.vm.box = "hashicorp/bionic64"
  config.vm.synced_folder ".", "/home/compile"
  config.vm.synced_folder "conf", "/usr/local/etc/ndn"
  
  config.vm.provider "virtualbox" do |vb|
    vb.memory = "4096"
    vb.cpus = "6"
  end
  
  config.vm.box_check_update = false
end
