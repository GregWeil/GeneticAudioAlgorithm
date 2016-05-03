/// audio.c
//A library for audio creation, sort of like midi music but not quite

#include <stdio.h>

typedef short SAMPLE;

const unsigned int SAMPLE_RATE = 48000;

void audio_save(SAMPLE* audio, unsigned int audiolength, char* path) {
	FILE* file = fopen(path, "wb");
	
	//Heaader chunk
	fprintf(file, "RIFF");
	unsigned int chunksize = ((audiolength * sizeof(SAMPLE)) + 36);
	fwrite(&chunksize, sizeof(chunksize), 1, file);
	fprintf(file, "WAVE");
	
	//Format chunk
	fprintf(file, "fmt ");
	unsigned int fmtchunksize = 16;
	fwrite(&fmtchunksize, sizeof(fmtchunksize), 1, file);
	unsigned short audioformat = 1;
	fwrite(&audioformat, sizeof(audioformat), 1, file);
	unsigned short numchannels = 1;
	fwrite(&numchannels, sizeof(numchannels), 1, file);
	unsigned int samplerate = SAMPLE_RATE;
	fwrite(&samplerate, sizeof(samplerate), 1, file);
	unsigned int byterate = (samplerate * numchannels * sizeof(SAMPLE));
	fwrite(&byterate, sizeof(byterate), 1, file);
	unsigned short blockalign = (numchannels * sizeof(SAMPLE));
	fwrite(&blockalign, sizeof(blockalign), 1, file);
	unsigned short bitspersample = (sizeof(SAMPLE) * 8);
	fwrite(&bitspersample, sizeof(bitspersample), 1, file);
	
	//Data chunk
	fprintf(file, "data");
	unsigned int datachunksize = (audiolength * sizeof(SAMPLE));
	fwrite(&datachunksize, sizeof(datachunksize), 1, file);
	fwrite(audio, sizeof(SAMPLE), audiolength, file);
	
	//Close the file
	fclose(file);
}
