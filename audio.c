/// audio.c
//A library for audio creation, sort of like midi music but not quite

#include <math.h>
#include <limits.h>
#include <stdio.h>

//Samples per second
const unsigned int SAMPLE_RATE = 48000;
//When mixing a track, compress audio to this volume
const double VOLUME_MAX = 0.25;
//The value of PI, since I need it sometimes
const double PI = 3.14159265358979323846;

//A single audio sample
typedef short Sample;

//The type definition for an audio stream
typedef struct {
	Sample* samples;
	unsigned int count;
} Audio;

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


double wave_sample_sin(double time, double frequency) {
	return sin(2.0 * PI * time * frequency);
}

double wave_sample_square(double time, double frequency) {
	double wave = (fmod(time, (1 / frequency)) * frequency);
	return ((wave > 0.5) - (wave < 0.5));
}

double wave_sample_triangle(double time, double frequency) {
	double wave = fmod(time, (1 / frequency));
	return (fabs((4.0 * wave * frequency) - 2.0) - 1.0);
}

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

//Build an audio stream from a note
Audio note_audio(const Note* note) {
	Audio audio = audio_initialize(note_samples(note));
	double (*wavesample)(double, double) = &wave_sample_sin;
	if (note->waveform == SIN) wavesample = &wave_sample_sin;
	else if (note->waveform == SQUARE) wavesample = &wave_sample_square;
	else if (note->waveform == TRIANGLE) wavesample = &wave_sample_triangle;
	else if (note->waveform == SAWTOOTH) wavesample = &wave_sample_sawtooth;
	unsigned int i;
	for (i = 0; i < audio.count; ++i) {
		double wave = (*wavesample)((i * 1.0 / SAMPLE_RATE), note->frequency);
		audio.samples[i] = (wave * note->volume * SHRT_MAX);
	}
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

//Generate the audio stream for a track of notes
Audio track_audio(const Track* track) {
	Audio audio = audio_initialize(track_samples(track));
	int* samples = (int*)malloc(audio.count * sizeof(int));
	unsigned int i, j;
	
	//Initialize the high range samples
	for (i = 0; i < audio.count; ++i) {
		samples[i] = 0;
	}
	
	//Add together all of the notes
	for (i = 0; i < track->count; ++i) {
		Note* note = &track->notes[i];
		Audio noteaudio = note_audio(note);
		unsigned int notetime = (note->time * SAMPLE_RATE);
		for (j = 0; j < noteaudio.count; ++j) {
			samples[notetime + j] += noteaudio.samples[j];
		}
		audio_free(&noteaudio);
	}
	
	//Find the maximum loudness in the track
	int loudest = SHRT_MAX;
	for (i = 0; i < audio.count; ++i) {
		if (samples[i] > loudest) loudest = samples[i];
		if (-samples[i] > loudest) loudest = -samples[i];
	}
	
	//Compress the high range samples
	for (i = 0; i < audio.count; ++i) {
		audio.samples[i] = samples[i] * (SHRT_MAX * VOLUME_MAX / loudest);
	}
	
	free(samples);
	return audio;
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
	
	Track track = track_initialize(size / notesize);
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
	
	//Heaader chunk
	fprintf(file, "RIFF");
	unsigned int chunksize = ((audio->count * sizeof(Sample)) + 36);
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
	unsigned int datachunksize = (audio->count * sizeof(Sample));
	fwrite(&datachunksize, sizeof(datachunksize), 1, file);
	fwrite(audio->samples, sizeof(Sample), audio->count, file);
	
	//Close the file
	fclose(file);
}