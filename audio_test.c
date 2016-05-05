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
	return 0;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("No command specified\n");
		return 1;
	}
	if (strcmp(argv[1], "note") == 0) {
		return test_note(argc, argv);
	}
	printf("Unrecognized command\n");
	return 1;
}
