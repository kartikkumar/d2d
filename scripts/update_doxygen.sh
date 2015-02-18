#!/bin/bash
set -ev

# Uninstall doxygen install. Fetch and build updated version from source.
yes | sudo apt-get --purge remove doxygen;
sudo apt-get install bison;
sudo apt-get install mscgen;
git clone https://github.com/doxygen/doxygen.git;
cd doxygen;
./configure;
make;
sudo make install;
sudo ln -s /usr/local/bin/doxygen /usr/bin/;
