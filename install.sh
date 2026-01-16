#!/usr/bin/bash

echo "Installing pypbc..." &&
sudo apt update &&
sudo apt-get install -y libgmp-dev && 
sudo apt-get install -y build-essential flex bison &&
wget https://crypto.stanford.edu/pbc/files/pbc-1.0.0.tar.gz &&
tar -xf pbc-1.0.0.tar.gz &&
cd pbc-1.0.0 &&
./configure --prefix=/usr --enable-shared &&
make &&
sudo make install &&
sudo ldconfig &&
cd .. &&
sudo pip3 install . &&
echo "pypbc installed successfully!"
