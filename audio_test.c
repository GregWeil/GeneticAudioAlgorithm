/// audio_test.c
//A file to store code testing the audio library

#include <stdlib.h>
#include <string.h>

#include "audio.c"

const char* file = "audio.wav";

int test_note(int argc, char** argv) {
	if (argc < 6) {
		printf("Wrong number of parameters\n");
		printf("%s %s waveform frequency volume duration\n", argv[0], argv[1]);
		return 1;
	}
	Note note = note_initialize();
	if (strcmp(argv[2], "sin") == 0) {
		note.waveform = SIN;
	} else if (strcmp(argv[2], "square") == 0) {
		note.waveform = SQUARE;
	} else if (strcmp(argv[2], "saw") == 0) {
		note.waveform = SAWTOOTH;
	} else {
		printf("Unrecognized waveform\n");
		return 1;
	}
	note.frequency = atof(argv[3]);
	note.volume = atof(argv[4]);
	note.duration = atof(argv[5]);
	Audio audio = note_audio(&note);
	audio_save(&audio, file);
	audio_free(&audio);
	note_free(&note);
	return 0;
}

int test_track(int argc, char** argv) {
	Track track = track_initialize(4);
	Note* note;
	
	note = &track.notes[0];
	note->time = 0;
	note->waveform = SIN;
	note->frequency = 660;
	note->volume = 1;
	note->duration = 1;
	
	note = &track.notes[1];
	note->time = 1.25;
	note->waveform = SQUARE;
	note->frequency = 440;
	note->duration = 1;
	
	note = &track.notes[2];
	note->time = 2.5;
	note->waveform = SAWTOOTH;
	note->frequency = 220;
	note->duration = 1;
	
	note = &track.notes[3];
	note->time = 0;
	note->waveform = SAWTOOTH;
	note->frequency = 110;
	note->volume = 0.25;
	note->duration = 3.5;
	
	Audio audio = track_audio(&track);
	audio_save(&audio, file);
	audio_free(&audio);
	track_free(&track);
	return 0;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("No command specified\n");
		return 1;
	}
	if (strcmp(argv[1], "note") == 0) {
		return test_note(argc, argv);
	} else if (strcmp(argv[1], "track") == 0) {
		return test_track(argc, argv);
	}
	printf("Unrecognized command\n");
	return 1;
}
