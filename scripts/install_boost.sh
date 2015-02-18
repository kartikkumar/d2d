#!/bin/bash
set -ev

# Fetch and install a newer Boost.
wget -O boost_1_55_0.tar.gz http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.gz/download
tar xzvf boost_1_55_0.tar.gz
cd boost_1_55_0/
./bootstrap.sh --prefix=/usr/local
sudo ./b2 --with=all --target=shared,static install
cd ..