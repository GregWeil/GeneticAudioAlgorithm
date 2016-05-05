/// audio.c
//A library for audio creation, sort of like midi music but not quite

#include <stdio.h>
#include <limits.h>

const unsigned int SAMPLE_RATE = 48000;
const double VOLUME_MAX = 0.2;

//A single audio sample
typedef short Sample;

//The type definition for an audio stream
typedef struct {
	Sample* samples;
	unsigned int count;
} Audio;

//The representation of a single note
typedef struct {
	double time;
	double frequency;
	double volume;
	double duration;
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


//Initialize a note with default values
Note note_initialize() {
	Note note;
	note.time = 0;
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
	unsigned short wavelength = (SAMPLE_RATE / note->frequency);
	for (unsigned int i = 0; i < audio.count; ++i) {
		double wave = (2 * (i % wavelength) / (double)wavelength) - 1;
		audio.samples[i] = (wave * note->volume * SHRT_MAX);
	}
	return audio;
}


//Initialize a track with default values
Track track_initialize(unsigned int length) {
	Track track;
	track.count = length;
	track.notes = (Note*)malloc(track.count * sizeof(Note));
	for (unsigned int i = 0; i < track.count; ++i) {
		track.notes[i] = note_initialize();
	}
	return track;
}

//Free data used by a track
void track_free(const Track* track) {
	for (unsigned int i = 0; i < track->count; ++i) {
		note_free(&track->notes[i]);
	}
	free(track->notes);
}

//Get the length of a track
double track_duration(const Track* track) {
	double duration = 0;
	for (unsigned int i = 0; i < track->count; ++i) {
		double time = (track->notes[i].time + track->notes[i].duration);
		if (time > duration) duration = time;
	}
	return duration;
}

//Get the number of samples needed to represent a track
unsigned int track_samples(const Track* track) {
	return (track_duration(track) * SAMPLE_RATE);
}

Audio track_audio(const Track* track) {
	Audio audio = audio_initialize(track_samples(track));
	
	//Initialize the high range samples
	int* samples = (int*)malloc(audio.count * sizeof(int));
	for (unsigned int i = 0; i < audio.count; ++i) {
		samples[i] = 0;
	}
	
	//Add together all of the notes
	for (unsigned int i = 0; i < track->count; ++i) {
		Note* note = &track->notes[i];
		Audio noteaudio = note_audio(note);
		unsigned int notetime = (note->time * SAMPLE_RATE);
		for (unsigned int j = 0; j < noteaudio.count; ++j) {
			samples[notetime + j] += noteaudio.samples[j];
		}
		audio_free(&noteaudio);
	}
	
	//Find the maximum loudness in the track
	int loudest = SHRT_MAX;
	for (unsigned int i = 0; i < audio.count; ++i) {
		if (samples[i] > loudest) loudest = samples[i];
		if (-samples[i] > loudest) loudest = -samples[i];
	}
	
	//Compress the high range samples
	for (unsigned int i = 0; i < audio.count; ++i) {
		audio.samples[i] = samples[i] * (SHRT_MAX * VOLUME_MAX / loudest);
	}
	
	free(samples);
	return audio;
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
