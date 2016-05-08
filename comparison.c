#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "sndfile.h"
#include "fftw3.h"
#include "comparison.h"

sf_count_t blockSize = 256; //some code suggests its the size of each sample. Other code suggests its the number of samples. 256 is default.

void PrintAudioMetadata(SF_INFO * file)
{
	//this is only for printing information for debugging purposes
    printf("Frames:\t%ld\n", file->frames); 
    printf("Sample rate:\t%d\n", file->samplerate);
    printf("Channels: \t%d\n", file->channels); //comparison wont be accurate if channels is not 0! 
    printf("Format: \t%d\n", file->format);
    printf("Sections: \t%d\n", file->sections);
    printf("Seekable: \t%d\n", file->seekable);
}

int ReadAudioFile(char* filename, double*** dft_data)
{
	//reads in .wav, returns FFT by reference through dft_data, returns size of dft_data
	sf_count_t i;
	sf_count_t j;
	
	SF_INFO info;
    SNDFILE * f = sf_open(filename, SFM_READ, &info);
    if( !f ){
    	printf("error: could not open %s for processing\n", filename );
    	return 0;
    }
    //PrintAudioMetadata(&info);
    if(info.channels != 1){
    	printf("error: .wav file must be Mono. Code can't handle multi-channel audio.\n");
    	sf_close( f );
    	return 0;
    }

    double* fftw_in = fftw_malloc( sizeof(double) * blockSize * info.channels );
    if ( !fftw_in ) {
		printf("error: fftw_malloc 1 failed\n");
		sf_close( f );
		return 0;
	}

	fftw_complex* fftw_out = fftw_malloc( sizeof(fftw_complex) * blockSize );
    if ( !fftw_out ) {
		printf("error: fftw_malloc 2 failed\n");
		fftw_free( fftw_in );
		sf_close( f );
		return 0;
    }

	fftw_plan plan = fftw_plan_dft_r2c_1d( blockSize, fftw_in, fftw_out, FFTW_MEASURE );
	if ( !plan ) {
		printf("error: Could not create plan\n");
		fftw_free( fftw_in );
		fftw_free( fftw_out );
		sf_close( f );
		return 0;
	}

	//allocate space for array to return
	int numBlocks = (int)(ceil(info.frames / (double)blockSize));
    (*dft_data) = malloc( sizeof(double*) * numBlocks * blockSize/2 );
	for(i = 0; i < (numBlocks * blockSize / 2.0); i++){
		(*dft_data)[i] = malloc(sizeof(double) * 2);
	}
    //printf("transform splits file into %d slices\n", numBlocks);

	sf_count_t numRead;
	for(i = 0; i < numBlocks; i++){

		numRead = sf_readf_double( f, fftw_in, blockSize );

		if ( numRead < blockSize ) {
			//reached last block, which has less than blockSize frames
			//Pad with 0 so fft is same size as other blocks
			for( j = numRead; j < blockSize; j++ ) {
				fftw_in[i] = 0.0;
			}
		}

		fftw_execute( plan );

		for(j = 0; j < blockSize/2; j++){
			//printf("%ld\n", i*numBlocks + j);
			(*dft_data)[i*(blockSize/2) + j][0] = fftw_out[j][0];
			(*dft_data)[i*(blockSize/2) + j][1] = fftw_out[j][1];
		}
	}

	fftw_destroy_plan( plan );
	fftw_free( fftw_in );
	fftw_free( fftw_out);
	sf_close( f );

    return numBlocks * blockSize/2;
}

int PassAudioData(Samples* samples, int numSamples, double*** dft_data)
{

	//samples is an array of shorts of size numSamples
	sf_count_t i;
	sf_count_t j;
	
    if( !samples ){
    	printf("error: no data in samples!\n");
    	return 0;
    }

    double* fftw_in = fftw_malloc( sizeof(double) * blockSize * info.channels );
    if ( !fftw_in ) {
		printf("error: fftw_malloc 1 failed\n");
		sf_close( f );
		return 0;
	}

	fftw_complex* fftw_out = fftw_malloc( sizeof(fftw_complex) * blockSize );
    if ( !fftw_out ) {
		printf("error: fftw_malloc 2 failed\n");
		fftw_free( fftw_in );
		sf_close( f );
		return 0;
    }

	fftw_plan plan = fftw_plan_dft_r2c_1d( blockSize, fftw_in, fftw_out, FFTW_MEASURE );
	if ( !plan ) {
		printf("error: Could not create plan\n");
		fftw_free( fftw_in );
		fftw_free( fftw_out );
		sf_close( f );
		return 0;
	}

	//allocate space for array to return
	int numBlocks = (int)(ceil(info.frames / (double)blockSize));
    (*dft_data) = malloc( sizeof(double*) * numBlocks * blockSize/2 );
	for(i = 0; i < (numBlocks * blockSize / 2.0); i++){
		(*dft_data)[i] = malloc(sizeof(double) * 2);
	}
    //printf("transform splits file into %d slices\n", numBlocks);

	sf_count_t numRead;
	for(i = 0; i < numBlocks; i++){

		numRead = sf_readf_double( f, fftw_in, blockSize );

		if ( numRead < blockSize ) {
			//reached last block, which has less than blockSize frames
			//Pad with 0 so fft is same size as other blocks
			for( j = numRead; j < blockSize; j++ ) {
				fftw_in[i] = 0.0;
			}
		}

		fftw_execute( plan );

		for(j = 0; j < blockSize/2; j++){
			//printf("%ld\n", i*numBlocks + j);
			(*dft_data)[i*(blockSize/2) + j][0] = fftw_out[j][0];
			(*dft_data)[i*(blockSize/2) + j][1] = fftw_out[j][1];
		}
	}

	fftw_destroy_plan( plan );
	fftw_free( fftw_in );
	fftw_free( fftw_out);
	sf_close( f );

    return numBlocks * blockSize/2;
}

double GetFitnessHelper(double** goal, double** test, int size){
	double fitness = 0.0;
	int i;
	for(i = 0; i < size; i++){
		fitness += ((abs(goal[i][0]) - abs(test[i][0])) * (abs(goal[i][0]) - abs(test[i][0])));
		fitness += ((abs(goal[i][1]) - abs(test[i][1])) * (abs(goal[i][1]) - abs(test[i][1])));
	}
	return fitness;
}

double AudioComparison(Samples* samples, int numSamples, double** goal, int goalsize){
	//testfile is the name of the .wav file to get the fitness of
	//goal is the FFT of the original audio file we are trying to emulate
	//size is the size of the first dimension of goal. the second dimension is always size 2.

	double** test = NULL;
	int testsize = ReadAudioFile(samples, numSamples, &test);
	//printf("size: %d\n", testsize);

	double fitness;
	int i;
	if(testsize >= goalsize){
		fitness = GetFitnessHelper(goal, test, goalsize);
		for(i = goalsize; i < testsize; i++){
			fitness += ((0 - abs(test[i][0])) * (0 - abs(test[i][0])));
			fitness += ((0 - abs(test[i][1])) * (0 - abs(test[i][1])));
		}
	}
	else if(testsize < goalsize){
		fitness = GetFitnessHelper(goal, test, testsize);
		for(i = testsize; i < goalsize; i++){
			fitness += ((abs(goal[i][0]) - 0) * (abs(goal[i][0]) - 0));
			fitness += ((abs(goal[i][1]) - 0) * (abs(goal[i][1]) - 0));
		}
	}

	for(i = 0; i < testsize; i++){
		free(test[i]);
	}
	free(test);

	return fitness;
}