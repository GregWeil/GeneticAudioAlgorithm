/// audio_test.c
//A file to store code testing the audio library

#include <stdlib.h>
#include <string.h>

#include "audio.c"

const char* file = "audio.wav";

int test_note(int argc, char** argv) {
	if (argc < 5) {
		printf("Wrong number of parameters\n");
		printf("%s %s frequency volume duration\n", argv[0], argv[1]);
		return 1;
	}
	Note note = note_initialize();
	note.frequency = atof(argv[2]);
	note.volume = atof(argv[3]);
	note.duration = atof(argv[4]);
	Audio audio = note_audio(&note);
	audio_save(&audio, file);
	audio_free(&audio);
	note_free(&note);
	return 0;
}

int test_track(int argc, char** argv) {
	Track track = track_initialize(3);
	Note* note;
	note = &track.notes[0];
	note->time = 0;
	note->duration = 1;
	note = &track.notes[1];
	note->time = 1.25;
	note->duration = 1;
	note = &track.notes[2];
	note->time = 2.5;
	note->duration = 1;
	printf("%d\n", track_samples(&track));
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
