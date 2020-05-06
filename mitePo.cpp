#include "mbed.h"
#include <cmath>
#include "DA7212.h"
#include "uLCD_4DGL.h"
#include "accelerometer_handler.h"
#include "config.h"
#include "magic_wand_model_data.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
#define songlength 20
#define numOfSong 3
#define CircuIncre(a) ((a + 1 >= numOfSong)? 0: a+1)
#define CircuDecre(a) ((a - 1 < 0)? 2: a-1)

int PredictGesture(float* output) {
	// How many times the most recent gesture has been matched in a row
	static int continuous_count = 0;
	// The result of the last prediction
	static int last_predict = -1;

	// Find whichever output has a probability > 0.8 (they sum to 1)
	int this_predict = -1;
	for (int i = 0; i < label_num; i++) {
		if (output[i] > 0.8) this_predict = i;
	}

	// No gesture was detected above the threshold
	if (this_predict == -1) {
		continuous_count = 0;
		last_predict = label_num;
		return label_num;
	}

	if (last_predict == this_predict) {
		continuous_count += 1;
	}
	else {
		continuous_count = 0;
	}
	last_predict = this_predict;

	// If we haven't yet had enough consecutive matches for this gesture,
	// report a negative result
	if (continuous_count < config.consecutiveInferenceThresholds[this_predict]) {
		return label_num;
	}
	// Otherwise, we've seen a positive result, so clear all our variables
	// and report it
	continuous_count = 0;
	last_predict = -1;

	return this_predict;
}
typedef struct note {
	int f;
	int len;
} Note;
DA7212 audio;
uLCD_4DGL uLCD(D1, D0, D2); // serial tx, serial rx, reset pin;
Serial pc(USBTX, USBRX);
Timer deboun1;
Timer deboun2;
InterruptIn sw2(SW2);
InterruptIn sw3(SW3);
EventQueue queue(32 * EVENTS_EVENT_SIZE);
EventQueue queue1(32 * EVENTS_EVENT_SIZE);
EventQueue queue2(32 * EVENTS_EVENT_SIZE);
EventQueue queueM(32 * EVENTS_EVENT_SIZE);
Note song[numOfSong][songlength];	// 3 songs to choose
int16_t waveform[kAudioTxBufferSize];

Thread t(osPriorityLow);
Thread t1(osPriorityNormal);
Thread t2(osPriorityHigh);
Thread tM(osPriorityLow);
int state = 0;
int songI = 0;	// song index
int noteI = -1;  // note index
void sw2_rise();
void sw3_rise();
int idc;
int idb;
void gestureModeSelect();
void gestureSongSelect();
void playNote(int);
void playSong(int, int);
void PlayMode();

constexpr int kTensorArenaSize = 60 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
bool should_clear_buffer = false;
bool got_data = false;
int gesture_index;

int main(int argc, char* argv[]) {
	char list[numOfSong][songlength * 2 + 10];
	deboun1.start();
	deboun2.start();
	uLCD.text_width(2);
	uLCD.text_height(4);
	uLCD.printf("\nAwaiting\nPC input\n");
	// load 3 songs	
	for (int j = 0; j < numOfSong;)
		if (pc.readable()) {
			pc.scanf("%s", list[j++]);
			wait(0.8f);
		}
	for (int j = 0; j < numOfSong; j++)
		for (int i = 0, k = 0; k < 2 * songlength; k++) {
			switch (list[j][k])
			{
			case 'w':
				song[j][i++].len = 1;
				break;
			case 'x':
				song[j][i++].len = 2;
				break;
			case 'y':
				song[j][i++].len = 3;
				break;
			case 'z':
				song[j][i++].len = 4;
				break;
			case 'c':
				song[j][i].f = 261;
				break;
			case 'd':
				song[j][i].f = 294;
				break;
			case 'e':
				song[j][i].f = 330;
				break;
			case 'f':
				song[j][i].f = 349;
				break;
			case 'g':
				song[j][i].f = 392;
				break;
			case 'a':
				song[j][i].f = 440;
				break;
			case 'b':
				song[j][i].f = 494;
				break;
			case 'C':
				song[j][i].f = 523;
				break;
			case 'D':
				song[j][i].f = 587;
				break;
			case 'E':
				song[j][i].f = 659;
				break;
			case 'F':
				song[j][i].f = 698;
				break;
			case 'G':
				song[j][i].f = 784;
				break;
			case 'A':
				song[j][i].f = 880;
				break;
			case 'B':
				song[j][i].f = 988;
				break;
			case 's':
				song[j][i].f = 0;
			}
		}
	t.start(callback(&queue, &EventQueue::dispatch_forever));
	t1.start(callback(&queue1, &EventQueue::dispatch_forever));
	t2.start(callback(&queue2, &EventQueue::dispatch_forever));
	tM.start(callback(&queueM, &EventQueue::dispatch_forever));
	sw2.rise(queue1.event(sw2_rise));
	sw3.rise(queue2.event(sw3_rise));
	queueM.call(PlayMode);
}

void gestureModeSelect()
{
	static tflite::MicroErrorReporter micro_error_reporter;
	tflite::ErrorReporter* error_reporter = &micro_error_reporter;
	const tflite::Model* model = tflite::GetModel(g_magic_wand_model_data);

	static tflite::MicroOpResolver<6> micro_op_resolver;
	micro_op_resolver.AddBuiltin(
		tflite::BuiltinOperator_DEPTHWISE_CONV_2D,
		tflite::ops::micro::Register_DEPTHWISE_CONV_2D());
	micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_MAX_POOL_2D,
		tflite::ops::micro::Register_MAX_POOL_2D());
	micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_CONV_2D,
		tflite::ops::micro::Register_CONV_2D());
	micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_FULLY_CONNECTED,
		tflite::ops::micro::Register_FULLY_CONNECTED());
	micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_SOFTMAX,
		tflite::ops::micro::Register_SOFTMAX());
	micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_RESHAPE,
		tflite::ops::micro::Register_RESHAPE(), 1);


	static tflite::MicroInterpreter static_interpreter(
		model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
	tflite::MicroInterpreter* interpreter = &static_interpreter;
	interpreter->AllocateTensors();
	TfLiteTensor* model_input = interpreter->input(0);

	int input_length = model_input->bytes / sizeof(float);
	TfLiteStatus setup_status = SetupAccelerometer(error_reporter);
	if (setup_status != kTfLiteOk) {
		error_reporter->Report("Set up failed\n");
		return;
	}
	error_reporter->Report("Set up successful...\n");
	uLCD.cls();
	uLCD.text_width(2);
	uLCD.text_height(3);
	uLCD.printf("\nMode\nselection\n");
	wait(0.2);
	while (state == 1) {
		// Attempt to read new data from the accelerometer
		got_data = ReadAccelerometer(error_reporter, model_input->data.f,
			input_length, should_clear_buffer);
		// If there was no new data,
		// don't try to clear the buffer again and wait until next time
		if (!got_data) {
			should_clear_buffer = false;
			continue;
		}
		uLCD.cls();
		uLCD.text_width(2);
		uLCD.text_height(3);
		uLCD.printf("\n...\n");
		// Run inference, and report any error
		TfLiteStatus invoke_status = interpreter->Invoke();
		if (invoke_status != kTfLiteOk) {
			error_reporter->Report("Invoke failed on index: %d\n", begin_index);
			continue;
		}
		// Analyze the results to obtain a prediction
		gesture_index = PredictGesture(interpreter->output(0)->data.f);
		// Clear the buffer next time we read data
		should_clear_buffer = gesture_index < label_num;
		// Produce an output

		if (gesture_index < label_num && state == 1) {
			switch (gesture_index)
			{
			case 0:	// ring
				state = 4;
				uLCD.cls();
				uLCD.text_width(2);
				uLCD.text_height(3);
				uLCD.printf("\nGo to\nsong selection?\n");
				return;
			case 1:	// slope
				state = 2;
				uLCD.cls();
				uLCD.text_width(2);
				uLCD.text_height(3);
				uLCD.printf("\nGo to\nLast song?\n");
				return;
			case 2:	// sprint
				state = 3;
				uLCD.cls();
				uLCD.text_width(2);
				uLCD.text_height(3);
				uLCD.printf("\nGo to\nNext song?\n");
				return;
			}
		}
	}
}
void gestureSongSelect()
{
	static tflite::MicroErrorReporter micro_error_reporter;
	tflite::ErrorReporter* error_reporter = &micro_error_reporter;
	const tflite::Model* model = tflite::GetModel(g_magic_wand_model_data);

	static tflite::MicroOpResolver<6> micro_op_resolver;
	micro_op_resolver.AddBuiltin(
		tflite::BuiltinOperator_DEPTHWISE_CONV_2D,
		tflite::ops::micro::Register_DEPTHWISE_CONV_2D());
	micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_MAX_POOL_2D,
		tflite::ops::micro::Register_MAX_POOL_2D());
	micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_CONV_2D,
		tflite::ops::micro::Register_CONV_2D());
	micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_FULLY_CONNECTED,
		tflite::ops::micro::Register_FULLY_CONNECTED());
	micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_SOFTMAX,
		tflite::ops::micro::Register_SOFTMAX());
	micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_RESHAPE,
		tflite::ops::micro::Register_RESHAPE(), 1);


	static tflite::MicroInterpreter static_interpreter(
		model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
	tflite::MicroInterpreter* interpreter = &static_interpreter;
	interpreter->AllocateTensors();
	TfLiteTensor* model_input = interpreter->input(0);

	int input_length = model_input->bytes / sizeof(float);
	TfLiteStatus setup_status = SetupAccelerometer(error_reporter);
	if (setup_status != kTfLiteOk) {
		error_reporter->Report("Set up failed\n");
		return;
	}
	error_reporter->Report("Set up successful...\n");
	uLCD.cls();
	uLCD.text_width(2);
	uLCD.text_height(3);
	uLCD.printf("\nSong\nselection\n");
	wait(0.2);
	while (state == 1) {
		// Attempt to read new data from the accelerometer
		got_data = ReadAccelerometer(error_reporter, model_input->data.f,
			input_length, should_clear_buffer);
		// If there was no new data,
		// don't try to clear the buffer again and wait until next time
		if (!got_data) {
			should_clear_buffer = false;
			continue;
		}
		uLCD.cls();
		uLCD.text_width(2);
		uLCD.text_height(3);
		uLCD.printf("\n...\n");
		// Run inference, and report any error
		TfLiteStatus invoke_status = interpreter->Invoke();
		if (invoke_status != kTfLiteOk) {
			error_reporter->Report("Invoke failed on index: %d\n", begin_index);
			continue;
		}
		// Analyze the results to obtain a prediction
		gesture_index = PredictGesture(interpreter->output(0)->data.f);
		// Clear the buffer next time we read data
		should_clear_buffer = gesture_index < label_num;

		// Produce an output
		if (gesture_index < label_num && state == 5) {
			error_reporter->Report(config.output_message[gesture_index]);
			switch (gesture_index)
			{
			case 0:	// ring
				state = 6;
				uLCD.cls();
				uLCD.text_width(2);
				uLCD.text_height(3);
				uLCD.printf("\nGo to Song.%d?\n", songI);
				return;
			case 1:	// slope
				songI = CircuDecre(songI);
				uLCD.cls();
				uLCD.text_width(2);
				uLCD.text_height(2);
				uLCD.printf("\n<- <-\nNow at:\nSong.%d\n", songI);
				continue;
			case 2:	// sprint
				songI = CircuIncre(songI);
				uLCD.cls();
				uLCD.text_width(2);
				uLCD.text_height(2);
				uLCD.printf("\n-> ->\nNow at:\nSong.%d\n", songI);
				continue;
			}
		}
	}
}

void playNote(int freq)
{
	for (int i = 0; i < kAudioTxBufferSize; i++)
	{
		waveform[i] = (int16_t)(sin((double)i * 2. * M_PI
		/ (double)(kAudioSampleFrequency / freq)) * ((1 << 16) - 1));
	}
	audio.spk.play(waveform, kAudioTxBufferSize);
}
void playSong(int j, int sp)
{
	int i, length;
	for (i = sp; state == 0 && i < songlength; i++) {
		noteI = i;
		length = song[j][i].len;
		while ((length--) && (state == 0))
		{
			// the loop below will play the note for  0.5s
			if (song[j][i].f != 0) {
				for (int k = 0; k < kAudioSampleFrequency / kAudioTxBufferSize / 2; k++)
					queue.call(playNote, song[j][i].f);
			}
		}
		wait(0.5);
	}
}
void PlayMode()
{
	for (int j = 0; j < numOfSong;)
	{
		if (state == 0) {
			uLCD.cls();
			uLCD.text_width(2);
			uLCD.text_height(3);
			uLCD.printf("\nCurrent\nPlaying:\nSong %d\n", songI);

			j = songI;
			playSong(j, noteI + 1);
			// so that pause won't change songI
			if (state == 0) {	// afte completely play a song
				j = CircuIncre(j);
				songI = j;
				noteI = -1;
			}
		}
	}
}

void sw2_rise()
{
	if (deboun1.read_ms() > 1000) {
		switch (state) {
		case 0:	// while playing song
			state = 1;
			idc = queue1.call(gestureModeSelect);
			break;
		case 2: //confim last
			songI = CircuDecre(songI);
			noteI = -1;
			state = 0;
			break;
		case 3:	// confim next
			songI = CircuIncre(songI);
			noteI = -1;
			state = 0;
			break;
		case 4: // confim into song select
			state = 5;
			idb = queue1.call(gestureSongSelect);
			break;
		case 6:	// confim song selecte
			noteI = -1;
			state = 0;
		}
		deboun1.reset();
	}
}
void sw3_rise()
{
	if (deboun2.read_ms() > 1000) {
		switch (state) {
		case 1: // back to playMode
			queue1.cancel(idc);
			state = 0;
			uLCD.cls();
			uLCD.text_width(2);
			uLCD.text_height(3);
			uLCD.printf("\nCurrent\nPlaying:\nSong %d\n", songI);
			break;
		case 2:
			queue1.cancel(idc);
			state = 1;
			idc = queue1.call(gestureModeSelect);
			break;
		case 3:
			queue1.cancel(idc);
			state = 1;
			idc = queue1.call(gestureModeSelect);
			break;
		case 4:
			queue1.cancel(idc);
			state = 1;
			idc = queue1.call(gestureModeSelect);
			break;
		case 6:	// confim song selecte
			queue1.cancel(idb);
			state = 5;
			idb = queue1.call(gestureSongSelect);
		}
		deboun2.reset();
	}
}