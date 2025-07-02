#!/bin/bash
cd /home/arno/Code/Pheno/CheckoutPlugins/
tput setaf 1
echo "Updating checkout plugins"
tput sgr0
git pull
cd /home/arno/Code/Pheno/PhenoLink
tput setaf 1
echo "building"
tput sgr0
cmake --build build/ -j16
tput setaf 1
echo "copying plugins"
tput sgr0
cp build/bin/internalplugins/Process/*.so build/bin/plugins/
tput setaf 1
echo "running on 192.168.2.128:13555"
tput sgr0
build/bin/PhenoLinkZMQWorker -proxy 192.168.2.128:13555 -m /mnt/shares/
