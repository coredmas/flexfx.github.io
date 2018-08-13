#include <math.h>
#include <string.h>
#include "flexfx.h"
#include "flexfx.i"

const char* product_name_string   = "C99";
const char* usb_audio_output_name = "Audio Out";
const char* usb_audio_input_name  = "Audio In";
const char* usb_midi_output_name  = "MIDI Out";
const char* usb_midi_input_name   = "MIDI In";

const int audio_sample_rate     = 192000; // Default sample rate at boot-up
const int audio_clock_mode      = 0; // 0=internal/master,1=external/master,2=slave
const int usb_output_chan_count = 2; // 2 USB audio class 2.0 output channels
const int usb_input_chan_count  = 2; // 2 USB audio class 2.0 input channels
const int i2s_channel_count     = 2; // Channels per SDIN/SDOUT wire (2,4,or 8)

const int i2s_sync_word[8] = { 0xFFFFFFFF,0x00000000,0,0,0,0,0,0 };

const char* control_labels[21] = { "C99",
                                   "","","","","","","","","","",
                                   "","","","","","","","","","" };

void audio_control( const double parameters[20], int property[6] )
{
}

void audio_mixer( const int usb_output[32], int usb_input[32],
                  const int adc_output[32], int dac_input[32],
                  const int dsp_output[32], int dsp_input[32], const int property[6] )
{
	dac_input[0] = usb_output[0];
	dac_input[1] = usb_output[1];
	usb_input[0] = adc_output[0];
	usb_input[1] = adc_output[1];
}

void dsp_initialize( void )
{
}

void dsp_thread1( int samples[32], const int property[6] ) {}
void dsp_thread2( int samples[32], const int property[6] ) {}
void dsp_thread3( int samples[32], const int property[6] ) {}
void dsp_thread4( int samples[32], const int property[6] ) {}
void dsp_thread5( int samples[32], const int property[6] ) {}
