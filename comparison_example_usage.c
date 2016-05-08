#include <stdio.h>
#include <stdlib.h>
#include "comparison.h"

int main(int argc, char ** argv)
{
	//this function provides an example of how this code should be utilized.
	//goalFile is our original audio file.
	//the tests are all hypothetical individuals to evaluate the fitness of.
	char* goalFile = "./audio/test.wav";
	char* test1 = "./audio/test.wav";
	char* test2 = "./audio/test_normalize.wav";
	char* test3 = "./audio/test_noise1.wav";
	char* test4 = "./audio/test_noise2.wav";

	//Code to read in the original audio file that the gen alg is trying to replicate. should only be read in once, and FFT data passed to where it is needed
    double** Goal = NULL;
    int size = ReadAudioFile(goalFile, &Goal);
	if( !size ) {
		printf("error: ReadAudioFile failed for %s\n", goalFile);
	}

	//Code to test fitness of individual by comparing individuals FFT to the Original FFT.
	printf("Fitnesses (lower is better):\n");

	double fitness = AudioComparison(test1, Goal, size);
	printf("%s has a fitness of %f\n", test1, fitness);

	fitness = AudioComparison(test2, Goal, size);
	printf("%s has a fitness of %f\n", test2, fitness);

	fitness = AudioComparison(test3, Goal, size);
	printf("%s has a fitness of %f\n", test3, fitness);

	fitness = AudioComparison(test4, Goal, size);
	printf("%s has a fitness of %f\n", test4, fitness);

	//free data
	int i;
	for(i = 0; i < size; i++){
		free(Goal[i]);
	}
	free(Goal);

	return 0;
}