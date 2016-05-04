/// audio_test.c
//A file to store code testing the audio library

#include "audio.c"

int main() {
	Note note;
	note.frequency = 220;
	note.volume = 0.05;
	note.length = 1.0;
	Sample* audio = note_audio(&note);
	audio_save(audio, note_samples(&note), "audio.wav");
	free(audio);
	return 0;
}
