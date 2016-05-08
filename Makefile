all: pgenalg.c
	mpicc -I. -Wall -O3 pgenalg.c -lm -o pgenalg
