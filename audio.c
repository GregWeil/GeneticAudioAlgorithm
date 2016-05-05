/// audio.c
//A library for audio creation, sort of like midi music but not quite

#include <stdio.h>
#include <limits.h>

const unsigned int SAMPLE_RATE = 48000;

typedef short Sample;


//The representation of a single note
typedef struct {
	double frequency;
	double volume;
	double duration;
} Note;

//Initialize a note with default values
Note note_initialize() {
	Note note;
	note.frequency = 440;
	note.volume = 0.01;
	note.duration = 1;
	return note;
}

//Get the number of samples needed to represent a note
unsigned int note_samples(const Note *note) {
	return (note->duration * SAMPLE_RATE);
}

//Build an audio stream from a note
Sample* note_audio(const Note *note) {
	unsigned int length = note_samples(note);
	Sample* audio = (Sample*)malloc(length * sizeof(Sample));
	unsigned short wavelength = (SAMPLE_RATE / note->frequency);
	for (unsigned int i = 0; i < length; ++i) {
		double wave = (2 * (i % wavelength) / (double)wavelength) - 1;
		audio[i] = (wave * note->volume * SHRT_MAX);
	}
	return audio;
}


//The representation of a track
typedef struct {
	Note* notes;
	unsigned int count;
} Track;

Track track_initialize() {
	Track track;
	track.notes = NULL;
	track.count = 0;
	return track;
}

//Get the length of a track
double track_duration(const Track* track) {
	double duration = 0;
	for (unsigned int i = 0; i < track->count; ++i) {
		if (track->notes[i].duration > duration) {
			duration = track->notes[i].duration;
		}
	}
	return duration;
}

//Get the number of samples needed to represent a track
unsigned int track_samples(const Track* track) {
	return (track_duration(track) * SAMPLE_RATE);
}


//Save an audio stream as a WAV file
void audio_save(const Sample* audio, const unsigned int audiolength, const char* path) {
	FILE* file = fopen(path, "wb");
	
	//Heaader chunk
	fprintf(file, "RIFF");
	unsigned int chunksize = ((audiolength * sizeof(Sample)) + 36);
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
	unsigned int byterate = (samplerate * numchannels * sizeof(Sample));
	fwrite(&byterate, sizeof(byterate), 1, file);
	unsigned short blockalign = (numchannels * sizeof(Sample));
	fwrite(&blockalign, sizeof(blockalign), 1, file);
	unsigned short bitspersample = (sizeof(Sample) * CHAR_BIT);
	fwrite(&bitspersample, sizeof(bitspersample), 1, file);
	
	//Data chunk
	fprintf(file, "data");
	unsigned int datachunksize = (audiolength * sizeof(Sample));
	fwrite(&datachunksize, sizeof(datachunksize), 1, file);
	fwrite(audio, sizeof(Sample), audiolength, file);
	
	//Close the file
	fclose(file);
}
