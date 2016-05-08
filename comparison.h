#include "sndfile.h"

void PrintAudioMetadata(SF_INFO * file);
int ReadAudioFile(char* filename, double*** dft_data, unsigned int* samplerate, unsigned int* frames);
int PassAudioData(double* samples, int numSamples, double*** dft_data);
double GetFitnessHelper(double** goal, double** test, int size);
double AudioComparison(double* samples, int numSamples, double** goal, int goalsize);