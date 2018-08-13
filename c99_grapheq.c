#include <math.h>
#include <string.h>
#include "flexfx.h"
#include "flexfx.i"

const char* product_name_string   = "C99 Equalizer";
const char* usb_audio_output_name = "GraphEQ Audio Out";
const char* usb_audio_input_name  = "GraphEQ Audio In";
const char* usb_midi_output_name  = "GraphEQ MIDI Out";
const char* usb_midi_input_name   = "GraphEQ MIDI In";

const int audio_sample_rate     = 192000; // Default sample rate at boot-up
const int audio_clock_mode      = 0; // 0=internal/master,1=external/master,2=slave
const int usb_output_chan_count = 2; // 2 USB audio class 2.0 output channels
const int usb_input_chan_count  = 2; // 2 USB audio class 2.0 input channels
const int i2s_channel_count     = 2; // Channels per SDIN/SDOUT wire (2,4,or 8)

const int i2s_sync_word[8] = { 0xFFFFFFFF,0x00000000,0,0,0,0,0,0 };

const char* control_labels[21] = { "C99 Equalizer",
                                   "01 - 56 Hz", "02 - 84 Hz", "03 - 126 Hz",
                                   "04 - 190 Hz", "05 - 284 Hz", "06 - 427 Hz",
                                   "07 - 640 Hz", "08 - 960 Hz", "Output Volume",
                                   "","","","","","","","","","" };

void audio_control( const double parameters[20], int property[6] )
{
	static int state = 1;
	
	double param_band[15], max_gain = 0;
	for( int ii = 0; ii < 15; ++ii ) {
	    param_band[ii] = parameters[ii];
	    if( param_band[ii] > max_gain ) max_gain = param_band[ii];
	}
    double param_volume = (parameters[15];
    double volume_min = 0.100, volume_max = 0.900;
    
    if( state == 1 ) // Volume, Gain
    {
        property[0] = state; state = 0x11;
        property[1] = FQ( volume_min + param_volume * (volume_max - volume_min) );
        property[2] = max_gain <= 0.5 ? FQ(1.0) : FQ( 1.0 / 2*max_gain );
    }
    else if( state >= 0x11 && state <= 0x1F ) // Bands 1 - 15
    {
        static double fb[15] = { 56,84,126,190,284,427,640,960,1440,2160,3240,4860,7290,10935,16402 };
        int ii = state - 0x11;
        double fs = audio_sample_rate, gain = 24.0 * (param_band[ii]-0.5);
        property[0] = state; state = state == 0x1F ? 1 : state+1;
        calc_peaking( property+1, fb[ii]/fs, 2.0, gain );
    }
}

void audio_mixer( const int usb_output[32], int usb_input[32],
                  const int i2s_output[32], int i2s_input[32],
                  const int dsp_output[32], int dsp_input[32], const int property[6] )
{
}

int _grapheq_coeff[15*5], _grapheq_state[15*4];

void dsp_initialize( void )
{
    return;
    memset( _grapheq_coeff, 0, sizeof(_grapheq_coeff) );
    memset( _grapheq_state, 0, sizeof(_grapheq_state) );
    // Initialize all bands to unity gain (b0=1, b1=b2=a1=a2=0)
    for( int ii = 0; ii < 15; ++ii ) _grapheq_coeff[5*ii] = FQ(+1.0);
}

void dsp_thread1( int samples[32], const int property[6] )
{
    return;
    static int volume = 0, gain = 0;
    
    samples[0] = dsp_mul( samples[0], gain );
    samples[0] = dsp_biquad( samples[0], _grapheq_coeff, _grapheq_state, 15 );
    samples[0] = dsp_mul( samples[0], volume );
    
    if( property[0] == 1 ) { volume = property[1], gain = property[2]; }
    if( property[0] >= 0x11 && property[0] <= 0x1F ) {
        memcpy( _grapheq_coeff + 5*(property[0]-0x11), property+1, 5*sizeof(int) );
    }
}

void dsp_thread2( int samples[32], const int property[6] ) {}
void dsp_thread3( int samples[32], const int property[6] ) {}
void dsp_thread4( int samples[32], const int property[6] ) {}
void dsp_thread5( int samples[32], const int property[6] ) {}
