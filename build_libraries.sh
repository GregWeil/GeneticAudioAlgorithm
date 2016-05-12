#!/bin/sh

unzip ./libraries.zip

cd ./fftw*
autoreconf -f -i
chmod +x ./configure
./configure
cd ../

cd ./libsndfile*
autoreconf -f -i
chmod +x ./configure
./configure
cd ../
