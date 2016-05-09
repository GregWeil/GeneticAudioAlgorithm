all: comparison.c comparison.h comparison_example_usage.c pgenalg.c
	gcc -I./libsndfile-1.0.26/src -I./fftw-3.3.4/api -Wall -O3 -c comparison.c -o comparison.o
	mpicc -I./libsndfile-1.0.26/src -I./fftw-3.3.4/api -Wall -O3 -c pgenalg.c -o pgenalg.o
	mpicc comparison.o pgenalg.o -o pgenalg -L./fftw-3.3.4/.libs -lfftw3 -L./libsndfile-1.0.26/src/.libs -lsndfile -lm
