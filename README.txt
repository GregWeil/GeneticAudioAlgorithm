to compile fftw-3.3.4 and libsndfile-1.0.26:
extract libraries.zip, then from inside both library folders run the commands:
./configure
make

On a linux system you can run build_libraries.sh

if you want to recompile the libraries, you must first remove the current configuration files with

make distclean

the main makefile must handle linking the libraries. If the libraries are not compiled, it will throw an error.
I did not want to overwrite the makefile that currently existed for pgenalg, so an exampke makefile to run comparison_example_usage.c is below. It demonstrates how to correctly link the libraries:

all: comparison.c comparison.h comparison_example_usage.c
	gcc -I./libsndfile-1.0.26/src -I./fftw-3.3.4/api -Wall -O3 -c comparison.c -o comparison.o
	gcc -I./libsndfile-1.0.26/src -I./fftw-3.3.4/api -Wall -O3 -c comparison_example_usage.c -o comparison_example_usage.o
	gcc comparison.o comparison_example_usage.o -o TestCompare -L./fftw-3.3.4/.libs -lfftw3 -L./libsndfile-1.0.26/src/.libs -lsndfile -lm
