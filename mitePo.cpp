#include "mbed.h"
#include <cmath>
#include "DA7212.h"
#include "uLCD_4DGL.h"
#define bufferLength (32)
#define signalLength (1024)

uLCD_4DGL uLCD(D1, D0, D2); // serial tx, serial rx, reset pin;

DA7212 audio;
Serial pc(USBTX, USBRX);
InterruptIn sw2(SW2);
InterruptIn sw3(SW3);
EventQueue queue(32 * EVENTS_EVENT_SIZE);
int idC = 0;
int16_t song[3][42];	// 3 songs to choose
int16_t waveform[kAudioTxBufferSize];
DigitalOut green_led(LED2);

Thread t1;
Thread t2;
int state = 0;
int songI = 0;	// song index
int noteI = -1;  // note index
void sw2_rise();
void sw3_rise();
void getGesture();
void playNote(int);
void playSong(int, int);
void PlayMode();
int main(int argc, char* argv[]) {
	green_led = 1;
	// set up gestue dNN
	{	constexpr int kTensorArenaSize = 60 * 1024;
	uint8_t tensor_arena[kTensorArenaSize];
	bool should_clear_buffer = false;
	bool got_data = false;
	int gesture_index;
	static tflite::MicroErrorReporter micro_error_reporter;
	tflite::ErrorReporter* error_reporter = &micro_error_reporter;
	const tflite::Model* model = tflite::GetModel(g_magic_wand_model_data);
	if (model->version() != TFLITE_SCHEMA_VERSION) {
		error_reporter->Report(
			"Model provided is schema version %d not equal "
			"to supported version %d.",
			model->version(), TFLITE_SCHEMA_VERSION);
		return -1;
	}
	static tflite::MicroOpResolver<5> micro_op_resolver;
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
	static tflite::MicroInterpreter static_interpreter(
		model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
	tflite::MicroInterpreter* interpreter = &static_interpreter;
	interpreter->AllocateTensors();
	TfLiteTensor* model_input = interpreter->input(0);
	if ((model_input->dims->size != 4) || (model_input->dims->data[0] != 1) ||
		(model_input->dims->data[1] != config.seq_length) ||
		(model_input->dims->data[2] != kChannelNumber) ||
		(model_input->type != kTfLiteFloat32)) {
		error_reporter->Report("Bad input tensor parameters in model");
		return -1;
	}
	int input_length = model_input->bytes / sizeof(float);
	TfLiteStatus setup_status = SetupAccelerometer(error_reporter);
	if (setup_status != kTfLiteOk) {
		error_reporter->Report("Set up failed\n");
		return -1;
	}
	error_reporter->Report("Set up successful...\n"); }
	//
	// load 3 songs
	{	
	char buffer;
	audio.spk.pause();
	for (int j = 0; j < 3; j++)
		for (int i = 0; i < 42;)
			if (pc.readable()) {
				buffer = pc.getc();
				switch (buffer)
				{
				case 'c':
					song[j][i] = 261;
					break;
				case 'd':
					song[j][i] = 294;
					break;
				case 'e':
					song[j][i] = 330;
					break;
				case 'f':
					song[j][i] = 349;
					break;
				case 'g':
					song[j][i] = 392;
					break;
				case 'a':
					song[j][i] = 440;
					break;
				case 'b':
					song[j][i] = 494;
					break;
				case 's':
					song[j][i] = song[j][i - 1];
				default:
					break;
				}
				i++;
			}
	}
	//
	green_led = 0;


	
}

void gestureModeSelect()
{
	while (true) {
		// Attempt to read new data from the accelerometer
		got_data = ReadAccelerometer(error_reporter, model_input->data.f,
			input_length, should_clear_buffer);
		// If there was no new data,
		// don't try to clear the buffer again and wait until next time
		if (!got_data) {
			should_clear_buffer = false;
			continue;
		}
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
		if (gesture_index < label_num) {
			error_reporter->Report(config.output_message[gesture_index]);
			switch (gesture_index)
			{
			case 0:	// ring
				state = 4;
				return;
			case 1:	// slope
				state = 3;
				return;
			case 2:	// sprint
				state = 2;
				return;
			}
		}
	}
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
	for (i = sp; state == 0 && i < 42; i++)
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
		if (state == 0) {
			uLCD.reset();
			uLCD.text_width(3); 
			uLCD.text_height(3);
			uLCD.printf("Current Playing:\nSong %d\n", songI); 
			
			j = songI;
			playSong(j, noteI + 1);

			j++;
			if (j > 2)	j -= 3;
			songI = j;
		}
	}
}

void sw2_rise()
{
	switch (state) {
	case 0:	// while playing song
		state = 1;
		uLCD.reset();
		uLCD.text_width(3);
		uLCD.text_height(3);
		uLCD.printf("Mode selection menu");
		break;
	case 2: //confim fowa
		songI = (songI == 2) ? (0) : (songI + 1);
		state = 0;
		break;
	case 3:	// confim backwa
		songI = (songI == 0) ? (2) : (songI - 1);
		state = 0;
		break;
	case 4: // confim into song select
		state = 5;
		uLCD.reset();
		uLCD.text_width(3);
		uLCD.text_height(3);
		uLCD.printf("Song selection menu");
		break;
	case 6:	// confim song selecte
		state = 0;
	}
}
void sw3_rise()
{
	switch (state) {
	case 1: //confim fowa
		state = 0;
		break;
	case 2:	
		state = 1;
		break;
	case 3: 
		state = 1;
		break;
	case 4:	
		state = 1;
		break;
	case 6:	// confim song selecte
		state = 5;
	}
}