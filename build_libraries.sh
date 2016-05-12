#!/bin/sh

unzip ./libraries.zip

cd ./fftw*
autoreconf -f -i
chmod +x ./configure
./configure
make
cd ../

cd ./libsndfile*
autoreconf -f -i
chmod +x ./configure
./configure
make
cd ../
