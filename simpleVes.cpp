#include "mbed.h"
#include <cmath>
#include "DA7212.h"
#include "uLCD_4DGL.h"
#define bufferLength (32)
#define signalLength (1024)



DA7212 audio;
uLCD_4DGL uLCD(D1, D0, D2); // serial tx, serial rx, reset pin;
EventQueue queue(32 * EVENTS_EVENT_SIZE);
int16_t song[3][48];	// 3 songs to choose
int16_t waveform[kAudioTxBufferSize];

Thread t;
int songI = 0;	// song index
int noteI = -1;  // note index
void playNote(int);
void playSong(int, int);
void PlayMode();


int main(int argc, char* argv[]) {
	uLCD.text_width(4);
	uLCD.text_height(4);
	uLCD.printf("\nInitializing...\n");

	for (int j = 0; j < 3; j++)
		for (int i = 0; i < 48;)
			song[j][i] = 261 + j * i;
	t.start(callback(&queue, &EventQueue::dispatch_forever));
	PlayMode();
}

void playNote(int freq)
{
	for (int i = 0; i < kAudioTxBufferSize; i++) {
		waveform[i] = (int16_t)(sin((double)i * 2. * M_PI
			/ (double)(kAudioSampleFrequency / freq))
			* ((1 << 16) - 1));
	}
	// the loop below will play the note for the duration of 1s
	for (int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize; ++j)
		audio.spk.play(waveform, kAudioTxBufferSize);
}
void playSong(int j, int sp = 0)
{
	int i;
	for (i = sp; i < 42; i++)
	{	
		noteI = i;
		queue.call(playNote, song[j][i]);
		wait(1.0);
	}
	if (i == 42)	noteI = -1;
}
void PlayMode() 
{
	for (int j = 0; j < 3;)
	{	
			uLCD.cls();
			uLCD.text_width(3); 
			uLCD.text_height(3);
			uLCD.printf("\nCurrent Playing:\nSong %d\n", songI); 
			
			j = songI;
			playSong(j, noteI + 1);

			j++;
			if (j == 3)	j = 0;
			songI = j;
	}
}

