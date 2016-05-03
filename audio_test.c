/// audio_test.c
//A file to store code testing the audio library

#include "audio.c"

int main() {
	int size = SAMPLE_RATE * 1;
	short* audio = (short*)malloc(size * sizeof(short));
	for (int i = 0; i < size; ++i) {
		audio[i] = 0;
		audio[i] += ((i % 80) - 40) * 40;
		audio[i] += ((i % 320) - 160) * 10;
	}
	audio_save(audio, size, "audio.wav");
	free(audio);
	return 0;
}
