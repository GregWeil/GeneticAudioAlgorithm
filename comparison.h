#include "sndfile.h"

void PrintAudioMetadata(SF_INFO * file);
int ReadAudioFile(char* filename, double*** dft_data);
double GetFitnessHelper(double** goal, double** test, int size);
double AudioComparison(char* testFile, double** goal, int size);