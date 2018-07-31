//////////////////////////////////////////////////////////////////////////////////////////////////
// NOT FINSISHED
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <string.h>
#include "flexfx.h"
#include "flexfx.i"

const char* product_name_string   = "C99 Reverb";
const char* usb_audio_output_name = "Reverb Audio Out";
const char* usb_audio_input_name  = "Reverb Audio In";
const char* usb_midi_output_name  = "Reverb MIDI Out";
const char* usb_midi_input_name   = "Reverb MIDI In";

const int audio_sample_rate     = 48000;
const int usb_output_chan_count = 2;
const int usb_input_chan_count  = 2;
const int i2s_channel_count     = 2;
const int i2s_is_bus_master     = 1;

const int i2s_sync_word[8] = { 0xFFFFFFFF,0x00000000,0,0,0,0,0,0 };

const char* control_labels[11] = { "C99 Reverb",
                                   "","","","","","","","","",
                                   "Output Volume" };

void flexfx_control( int preset, byte parameters[20], int dsp_prop[6] )
{
	static int state = 1;

    if( state == 1 ) // Volume, Gain
    {
        dsp_prop[0] = state; state = 0x11;
        dsp_prop[1] = FQ( volume_min + param_volume * (volume_max - volume_min) );
        dsp_prop[2] = FQ( exp(10, (-24.0 * (max_gain/100.0-0.5)) ));
    }
}

nline int _comb_filterL( int xx, int ii, int nn ) // yy[k] = xx[k] + g1*xx[k-M1] - g2*yy[k-M2]
{
    ii = (_comb_delays[nn] + ii) & 2047; // Index into sample delay FIFO
    int yy = _comb_bufferL[nn][ii];
    _comb_stateL[nn] = dsp_multiply( yy, FQ(1.0) - _comb_damping )
                     + dsp_multiply( _comb_stateL[nn], _comb_damping );
    _comb_bufferL[nn][ii] = xx + dsp_multiply( _comb_stateL[nn], _comb_feedbk );
    return yy;
}

inline int _comb_filterR( int xx, int ii, int nn ) // yy[k] = xx[k] + g1*xx[k-M1] - g2*yy[k-M2]
{
    ii = (_comb_delays[nn] + ii + _stereo_spread) & 2047; // Index into sample delay FIFO
    int yy = _comb_bufferR[nn][ii];
    _comb_stateR[nn] = dsp_multiply( yy, FQ(1.0) - _comb_damping )
                     + dsp_multiply( _comb_stateR[nn], _comb_damping );
    _comb_bufferR[nn][ii] = xx + dsp_multiply( _comb_stateR[nn], _comb_feedbk );
    return yy;
}

inline int _allpass_filterL( int xx, int ii, int nn ) // yy[k] = xx[k] + g * xx[k-M] - g * xx[k]
{
    ii = (_allpass_delays[nn] + ii) & 1023; // Index into sample delay FIFO
    int yy = _allpass_bufferL[nn][ii] - xx;
    _allpass_bufferL[nn][ii] = xx + dsp_multiply( _allpass_bufferL[nn][ii], _allpass_feedbk );
    return yy;
}

inline int _allpass_filterR( int xx, int ii, int nn ) // yy[k] = xx[k] + g * xx[k-M] - g * xx[k]
{
    ii = (_allpass_delays[nn] + ii + _stereo_spread) & 1023; // Index into sample delay FIFO
    int yy = _allpass_bufferR[nn][ii] - xx;
    _allpass_bufferR[nn][ii] = xx + dsp_multiply( _allpass_bufferR[nn][ii], _allpass_feedbk );
    return yy;
}

void app_initialize( void )
{
    memset( _comb_bufferL, 0, sizeof(_comb_bufferL) );
    memset( _comb_stateL,  0, sizeof(_comb_stateL) );
    memset( _comb_bufferR, 0, sizeof(_comb_bufferR) );
    memset( _comb_stateR,  0, sizeof(_comb_stateR) );
}

int _reverb_volume = 0;

int _comb_buffer   [8][2048]; // Delay lines for comb filters
int _comb_state    [8];       // Comb filter state (previous value)
int _allpass_buffer[4][1024]; // Delay lines for allpass filters

int _allpass_feedbk    = FQ(0.5); // Reflection decay/dispersion
int _stereo_spread     = 23;      // Buffer index spread for stereo separation
int _comb_delays   [8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 }; // From FreeVerb
int _allpass_delays[8] = { 556, 441, 341, 225 }; // From FreeVerb

int _wet_dry_blend  = FQ(0.2); // Parameter: Wet/dry mix setting (0.0=dry)
int _stereo_width   = FQ(0.2); // Parameter: Stereo width setting
int _comb_damping   = FQ(0.2); // Parameter: Reflection damping factor (aka 'reflectivity')
int _comb_feedbk    = FQ(0.2); // Parameter: Reflection feedback ratio (aka 'room size')

void app_thread1( int samples[32], const int property[6] )
{
    static int index = 0; ++index; // Used to index into the sample FIFO delay buffer
    // Eight parallel comb filters ...
    samples[2] = _comb_filterL( samples[0]/8, index, 0 ) + _comb_filterL( samples[0]/8, index, 1 )
               + _comb_filterL( samples[0]/8, index, 2 ) + _comb_filterL( samples[0]/8, index, 3 )
               + _comb_filterL( samples[0]/8, index, 4 ) + _comb_filterL( samples[0]/8, index, 5 )
               + _comb_filterL( samples[0]/8, index, 6 ) + _comb_filterL( samples[0]/8, index, 7 );
    // Four series all-pass filters ...
    samples[2] = _allpass_filterL( samples[2], index, 0 );
    samples[2] = _allpass_filterL( samples[2], index, 1 );
    samples[2] = _allpass_filterL( samples[2], index, 2 );
    samples[2] = _allpass_filterL( samples[2], index, 3 );
}

void app_thread2( int samples[32], const int property[6] )
{
}

void app_thread3( int samples[32], const int property[6] ) {}
void app_thread4( int samples[32], const int property[6] ) {}

void app_thread5( int samples[32], const int property[6] )
{
    samples[0] = dsp_mul( samples[0], _reverb_volume );

    if( property[0] == 1 ) _reverb_volume = property[1];
    if( property[0] == 2 ) {
    }
    if( property[0] == 3 ) {
    }
    if( property[0] == 4 ) {
    }
    
    _delay_fback = 0; _delay_regen = 0;
}
