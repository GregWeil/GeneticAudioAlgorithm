/// audio.c
//A library for audio creation, sort of like midi music but not quite
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>

//The value of PI, since I need it sometimes
const double PI = 3.14159265358979323846;
//Samples per second
unsigned int SAMPLE_RATE = 48000;
//When mixing a track, compress audio to this volume
double VOLUME_MAX = 1;

//A single audio sample
typedef double Sample;

//The type definition for an audio stream
typedef struct {
	Sample* samples;
	unsigned int count;
} Audio;

//The possible waveforms for a note
typedef enum {
	SIN,
	SQUARE,
	TRIANGLE,
	SAWTOOTH
} Waveform;

//The representation of a single note
typedef struct {
	double time; //Start time in seconds
	Waveform waveform; //The type of note
	double frequency; //Frequency in hertz
	double volume; //Volume from 0 to 1
	double duration; //Duration in seconds
} Note;

//The representation of a track
typedef struct {
	Note* notes;
	unsigned int count;
} Track;


//Initialize an audio stream with a number of samples
Audio audio_initialize(const unsigned int length) {
	Audio audio;
	audio.count = length;
	audio.samples = (Sample*)malloc(audio.count * sizeof(Sample));
	unsigned int i;
	for (i = 0; i < audio.count; ++i) {
		audio.samples[i] = 0;
	}
	return audio;
}

//Free data used by an audio stream
void audio_free(const Audio* audio) {
	free(audio->samples);
}

//Get the duration of an audio clip
double audio_duration(const Audio* audio) {
	return (audio->count * 1.0 / SAMPLE_RATE);
}


//Sample a sin wave
double wave_sample_sin(double time, double frequency) {
	return sin(2.0 * PI * time * frequency);
}

//Sample a square wave
double wave_sample_square(double time, double frequency) {
	double wave = (fmod(time, (1 / frequency)) * frequency);
	return ((wave > 0.5) - (wave < 0.5));
}

//Sample a triangle wave
double wave_sample_triangle(double time, double frequency) {
	double wave = fmod(time, (1 / frequency));
	return (fabs((4.0 * wave * frequency) - 2.0) - 1.0);
}

//Sample a sawtooth wave
double wave_sample_sawtooth(double time, double frequency) {
	double wave = fmod(time, (1 / frequency));
	return ((2.0 * wave * frequency) - 1.0);
}


//Initialize a note with default values
Note note_initialize() {
	Note note;
	note.time = 0;
	note.waveform = SIN;
	note.frequency = 440;
	note.volume = 0.5;
	note.duration = 1;
	return note;
}

//Free data used by a note
void note_free(const Note* note) {
	//Nothing to be done
}

//Get the number of samples needed to represent a note
unsigned int note_samples(const Note* note) {
	return (note->duration * SAMPLE_RATE);
}

//Build an audio stream from a note, adding the samples into an allocated stream
void note_audio_preallocated(const Note* note, Audio* audio, const unsigned int start) {
	double (*wavesample)(double, double) = &wave_sample_sin;
	if (note->waveform == SIN) wavesample = &wave_sample_sin;
	else if (note->waveform == SQUARE) wavesample = &wave_sample_square;
	else if (note->waveform == TRIANGLE) wavesample = &wave_sample_triangle;
	else if (note->waveform == SAWTOOTH) wavesample = &wave_sample_sawtooth;
	unsigned int i, count = note_samples(note);
	if ((audio->count - start) < count) count = (audio->count - start);
	for (i = 0; i < count; ++i) {
		double wave = (*wavesample)((i * 1.0 / SAMPLE_RATE), note->frequency);
		audio->samples[i + start] += (wave * note->volume);
	}
}

//Allocate and build the audio stream for a note in one go
Audio note_audio(const Note* note) {
	Audio audio = audio_initialize(note_samples(note));
	note_audio_preallocated(note, &audio, 0);
	return audio;
}


//Initialize a track with default values
Track track_initialize(unsigned int length) {
	Track track;
	track.count = length;
	track.notes = (Note*)malloc(track.count * sizeof(Note));
	unsigned int i;
	for (i = 0; i < track.count; ++i) {
		track.notes[i] = note_initialize();
	}
	return track;
}

//Free data used by a track
void track_free(const Track* track) {
	unsigned int i;
	for (i = 0; i < track->count; ++i) {
		note_free(&track->notes[i]);
	}
	free(track->notes);
}

//Get the length of a track
double track_duration(const Track* track) {
	double duration = 0;
	unsigned int i;
	for (i = 0; i < track->count; ++i) {
		double time = (track->notes[i].time + track->notes[i].duration);
		if (time > duration) duration = time;
	}
	return duration;
}

//Get the number of samples needed to represent a track
unsigned int track_samples(const Track* track) {
	return (track_duration(track) * SAMPLE_RATE);
}

//Generate the audio stream for a track with a fixed length
Audio track_audio_fixed_samples(const Track* track, const unsigned int count) {
	Audio audio = audio_initialize(count);
	double loudestsample = 1;
	unsigned int i;
	
	//Add together all of the notes
	for (i = 0; i < track->count; ++i) {
		Note* note = &track->notes[i];
		unsigned int notetime = (note->time * SAMPLE_RATE);
		note_audio_preallocated(note, &audio, notetime);
	}
	
	//Find the maximum loudness in the track
	for (i = 0; i < audio.count; ++i) {
		loudestsample = fmax(loudestsample, fabs(audio.samples[i]));
	}
	
	//Compress the high range samples
	for (i = 0; i < audio.count; ++i) {
		audio.samples[i] = (audio.samples[i] * VOLUME_MAX / loudestsample);
	}
	
	return audio;
}

//Generate the audio stream for a track of notes
Audio track_audio(const Track* track) {
	return track_audio_fixed_samples(track, track_samples(track));
}


//Generate a track based on an array of chars
Track track_initialize_from_binary(const char* data, const unsigned int size,
	const double timemax, const double durationmax, const double frequencymax)
{
	//The size of different parts of the data
	const unsigned int startsize = sizeof(unsigned int);
	const unsigned int wavesize = sizeof(unsigned char);
	const unsigned int frequencysize = sizeof(unsigned int);
	const unsigned int volumesize = sizeof(unsigned char);
	const unsigned int durationsize = sizeof(unsigned short);
	
	//The total size of a note
	const unsigned int notesize = (startsize + wavesize
		+ frequencysize + volumesize + durationsize);
	
	//Create the track
	Track track = track_initialize(size / notesize);
	
	//Loop through the data
	unsigned int i;
	for (i = 0; i < track.count; ++i) {
		unsigned int index = (i * notesize);
		Note* note = &track.notes[i];
		note->time = (*(unsigned int*)&data[index] * timemax / UINT_MAX);
		index += startsize;
		unsigned char wave = (*(unsigned char*)&data[index] % 4);
		if (wave == 0) note->waveform = SIN;
		else if (wave == 1) note->waveform = SQUARE;
		else if (wave == 2) note->waveform = TRIANGLE;
		else if (wave == 3) note->waveform = SAWTOOTH;
		index += wavesize;
		note->frequency = (*(unsigned int*)&data[index] * frequencymax / UINT_MAX);
		index += frequencysize;
		note->volume = (*(unsigned char*)&data[index] * 1.0 / UCHAR_MAX);
		index += volumesize;
		note->duration = (*(unsigned short*)&data[index] * durationmax / USHRT_MAX);
		index += durationsize;
	}
	
	return track;
}

//Save an audio stream as a WAV file
void audio_save(const Audio* audio, const char* path) {
	FILE* file = fopen(path, "wb");
	
	//Encode samples to 16 bit
	typedef short EncodedSample;
	EncodedSample* samples = (EncodedSample*)malloc(audio->count * sizeof(EncodedSample));
	unsigned int i;
	for (i = 0; i < audio->count; ++i) {
		samples[i] = (fmin(fmax(audio->samples[i], -1), 1) * SHRT_MAX);
	}
	
	//Heaader chunk
	fprintf(file, "RIFF");
	unsigned int chunksize = ((audio->count * sizeof(EncodedSample)) + 36);
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
	unsigned int byterate = (samplerate * numchannels * sizeof(EncodedSample));
	fwrite(&byterate, sizeof(byterate), 1, file);
	unsigned short blockalign = (numchannels * sizeof(EncodedSample));
	fwrite(&blockalign, sizeof(blockalign), 1, file);
	unsigned short bitspersample = (sizeof(EncodedSample) * CHAR_BIT);
	fwrite(&bitspersample, sizeof(bitspersample), 1, file);
	
	//Data chunk
	fprintf(file, "data");
	unsigned int datachunksize = (audio->count * sizeof(EncodedSample));
	fwrite(&datachunksize, sizeof(datachunksize), 1, file);
	fwrite(samples, sizeof(EncodedSample), audio->count, file);
	
	//Free encoded samples
	free(samples);
	
	//Close the file
	fclose(file);
}
