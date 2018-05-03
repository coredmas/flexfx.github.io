#include "flexfx.h" // Defines config variables, I2C and GPIO functions, etc.
#include <math.h>   // Floating point for filter coeff calculations in the background process.
#include <string.h> // Memory and string functions

const char* product_name_string   = "FlexFX Example";   // Your product name
const char* usb_audio_output_name = "FlexFX Audio Out"; // USB audio output name
const char* usb_audio_input_name  = "FlexFX Audio In";  // USB audio input name
const char* usb_midi_output_name  = "FlexFX MIDI Out";  // USB MIDI output name
const char* usb_midi_input_name   = "FlexFX MIDI In";   // USB MIDI input name

const int audio_sample_rate     = 48000; // Audio sampling frequency
const int usb_output_chan_count = 2;     // 2 USB audio class 2.0 output channels
const int usb_input_chan_count  = 2;     // 2 USB audio class 2.0 input channels
const int i2s_channel_count     = 2;     // ADC/DAC channels per SDIN/SDOUT wire
const int i2s_is_bus_master     = 1;     // Set to 1 if FlexFX creates I2S clocks

const int i2s_sync_word[8] = { 0xFFFFFFFF,0x00000000,0,0,0,0,0,0 }; // I2S WCLK values per slot

const char controller_script[] = \
	"" \
	"ui_header   ( ID:0x01, 'GUI Example', ['','Volume(L/R)','','Voices','Amplitude','Frequency','','File','Name','Action'] );" \
	"ui_separator();" \
	/* The output volume slider controls range from 0.00 to 0.95 in steps of 0.05 */
	"ui_vslider  ( ID:0x02, 'f', 30, 0.0,0.95,0.05 );" /* Vertical slider in "Volume" column */ \
	"ui_vslider  ( ID:0x03, 'l', 30, 0.0,0.95,0.05 );" /* Vertical slider in "Volume" column */ \
	"ui_separator();" \
	"ui_label    ( ID:0x04, 'F', 'LFO 1' );"           /* First label in "Voices" column */ \
	"ui_label    ( ID:0x05, 'N', 'LFO 2' );"           /* Next  label in "Voices" column */ \
	"ui_label    ( ID:0x06, 'L', 'LFO 3' );"           /* Last  label in "Voices" column */ \
	/* The LFO amplitude slider controls range from 0.00 to 0.95 in steps of 0.05 */
	"ui_hslider  ( ID:0x07, 'F', 96, 0.0,0.95,0.05 );" /* First slider in "Amplitude" column */ \
	"ui_hslider  ( ID:0x08, 'N', 96, 0.0,0.95,0.05 );" /* Next  slider in "Amplitude" column */ \
	"ui_hslider  ( ID:0x09, 'L', 96, 0.0,0.95,0.05 );" /* Last  slider in "Amplitude" column */ \
    /* The LFO frequency slider controls range from 1.00 Hz to 10.0 Hz in steps of 0.1 Hz. */
	"ui_hslider  ( ID:0x0A, 'F', 96, 1.0,10.0,0.10 );" /* First slider in "Frequency" column */ \
	"ui_hslider  ( ID:0x0B, 'N', 96, 1.0,10.0,0.10 );" /* Next  slider in "Frequency" column */ \
	"ui_hslider  ( ID:0x0C, 'L', 96, 1.0,10.0,0.10 );" /* Last  slider in "Frequency" column */ \
	"ui_separator();" \
	"ui_file     ( ID:0x0D, 'S', 2, 'Select', ID:0x0E );" \
	"ui_label    ( ID:0x0E, 'S', 'clip.wav' );" \
	"ui_button   ( ID:0x0F, 'S', 2, 'Play' );" \
	"";

static int current_preset = -1;

static int volumeL[9]    = {0,0,0,0,0,0,0,0,0};
static int volumeR[9]    = {0,0,0,0,0,0,0,0,0};
static int osc1_delta[9] = {0,0,0,0,0,0,0,0,0}; // For generating successive samples for OSC 1
static int osc2_delta[9] = {0,0,0,0,0,0,0,0,0}; // For generating successive samples for OSC 2
static int osc3_delta[9] = {0,0,0,0,0,0,0,0,0}; // For generating successive samples for OSC 3
static int osc1_level[9] = {0,0,0,0,0,0,0,0,0}; // Oscillator amplitude for OSC 1
static int osc2_level[9] = {0,0,0,0,0,0,0,0,0}; // Oscillator amplitude for OSC 2
static int osc3_level[9] = {0,0,0,0,0,0,0,0,0}; // Oscillator amplitude for OSC 3

static int wave_data[5*4096] = {0};

void app_control( const int rcv_prop[6], int snd_prop[6], int dsp_prop[6] )
{
    static int state = 0, offset = 0;
    
    if( state == 0 ) {
        state = 1; memset( wave_data, 0, sizeof(wave_data) );
    }
    
    if( rcv_prop[0] == 0x1501 ) offset = 0; // Start upload property
    else if( rcv_prop[0] == 0x1502 && offset < 4096 ) // Continue upload property
    {
        wave_data[5*offset+0] = rcv_prop[1]; wave_data[5*offset+1] = rcv_prop[2];
        wave_data[5*offset+2] = rcv_prop[3]; wave_data[5*offset+3] = rcv_prop[4];
        wave_data[5*offset+4] = rcv_prop[5]; ++offset;
    }
    else if( (rcv_prop[0] & 0xF000) == 0x2000 ) // HTML GUI properties
    {
        int preset = (rcv_prop[0] >> 8) & 0x0F, propid = rcv_prop[0] & 0xFF;
        
        // Change of preset? Load FLASH data if ...
        if( preset != current_preset && state == 0 ) {
            current_preset = preset;
        }
        // Get output volumes and store as FQ values ...
        if( propid == 0x01 ) volumeL[preset] = FQ( rcv_prop[1] );
        if( propid == 0x02 ) volumeR[preset] = FQ( rcv_prop[1] );
        // Get oscillator frequencies (float), convert to per-cycle deltas, store as Q28 ...
        if( propid == 0x06 ) osc1_delta[preset] = FQ( rcv_prop[1] / audio_sample_rate );
        if( propid == 0x07 ) osc2_delta[preset] = FQ( rcv_prop[1] / audio_sample_rate );
        if( propid == 0x08 ) osc3_delta[preset] = FQ( rcv_prop[1] / audio_sample_rate );
        // Get oscillator volume levels (float), store as FQ values ...
        if( propid == 0x09 ) osc1_level[preset] = FQ( rcv_prop[1] );
        if( propid == 0x0A ) osc2_level[preset] = FQ( rcv_prop[1] );
        if( propid == 0x0B ) osc3_level[preset] = FQ( rcv_prop[1] );
        
        // Forward relevant preset changes to DSP threads indefinitely ...
        if( state == 1 ) { // Update DSP threads with oscillator frequencies.
            state = 2;
            dsp_prop[0] = 1; dsp_prop[1] = osc1_delta[preset];
            dsp_prop[2] = osc2_delta[preset]; dsp_prop[3] = osc3_delta[preset];
        }
        else if( state == 2 ) { // Update DSP threads with oscillator amplitudes.
            state = 1;
            dsp_prop[0] = 2; dsp_prop[1] = osc1_level[preset];
            dsp_prop[2] = osc2_level[preset]; dsp_prop[3] = osc3_level[preset];
        }
        else if( propid == 0x0F ) dsp_prop[0] = 3; // Start playback.
    }
}

void app_mixer( const int usb_output[32], int usb_input[32],
                const int i2s_output[32], int i2s_input[32],
                const int dsp_output[32], int dsp_input[32], const int property[6] )
{
    int pp = current_preset;
    static int wave_index = 4096; if( property[0] == 3 ) wave_index = 0; // Start playback.
    
    // Route DSP result to the USB inputs ...
    usb_input[0] = usb_input[1] = dsp_output[0] * 8; // FQ (DSP) to Q31 (USB/I2S)
    
    // Convert oscillator signals from FQ to Q31, divide by 3 (total number of signals to sum)
    // to prevent overflow, then sum.
    usb_input[0] = ((dsp_output[2] * 8) / 3) + ((dsp_output[3] * 8) / 3); // Add #1 and #2.
    usb_input[1] = ((dsp_output[2] * 8) / 3) + ((dsp_output[4] * 8) / 3); // Add #1 and #3.
    
    // Add wave file playback signal to both left and right, convert from Q28 to Q31 first.
    if( wave_index < 4096 ) {
        int sample = wave_data[wave_index++] * 8;
        usb_input[0] += sample / 3; // Add 4th signal (wave data) to left.
        usb_input[1] += sample / 3; // Add 4th signal (wave data) to right.
    }
    
    // Apply volume levels ...
    usb_input[0] = dsp_multiply( usb_input[0], volumeL[pp] ); // Scale output level (left)
    usb_input[1] = dsp_multiply( usb_input[1], volumeR[pp] ); // Scale output level (right)
}

void app_initialize( void )
{
}

void app_thread1( int samples[32], const int property[6] ) // Oscillators 1, 2, and 3.
{
    // Oscillator time increments. Update them upon receiving the appropriate property.
    static int delta1 = 0, delta2 = 0, delta3 = 0;
    if( property[0] == 1 ) { delta1 = property[1]; delta2 = property[2]; delta3 = property[3]; }
    // Oscillator amplitudes. Update them upon receiving the appropriate property.
    static int level1 = 0, level2 = 0, level3 = 0;
    if( property[0] == 2 ) { level1 = property[1]; level2 = property[2]; level3 = property[3]; }

    // Update oscillator time: Increment each and limit to 1.0 -- wrap as needed.
    static int time1 = FQ(0.0); time1 += delta1; if(time1 > FQ(1.0)) time1 -= FQ(1.0);
    static int time2 = FQ(0.0); time2 += delta2; if(time2 > FQ(1.0)) time2 -= FQ(1.0);
    static int time3 = FQ(0.0); time3 += delta3; if(time3 > FQ(1.0)) time3 -= FQ(1.0);

    // Look up the sine value, interpolate to smooth out the look-ups.
    // - II is index into sine table (0.0 < II < 1.0), FF is the fractional remainder.
    // - Use 2nd order interpolation to smooth out lookup values.
    // - Index and fraction portion of FQ sample value: snnniiii,iiiiiiff,ffffffff,ffffffff
    int ii, ff;
    ii = (time1 & 0x0FFFFFFF) >> 18, ff = (time1 & 0x0003FFFF) << 10;
    samples[2] = dsp_lagrange( ff, dsp_sine_10[ii+0], dsp_sine_10[ii+1], dsp_sine_10[ii+2] );
    ii = (time2 & 0x0FFFFFFF) >> 18, ff = (time2 & 0x0003FFFF) << 10;
    samples[3] = dsp_lagrange( ff, dsp_sine_10[ii+0], dsp_sine_10[ii+1], dsp_sine_10[ii+2] );
    ii = (time3 & 0x0FFFFFFF) >> 18, ff = (time3 & 0x0003FFFF) << 10;
    samples[4] = dsp_lagrange( ff, dsp_sine_10[ii+0], dsp_sine_10[ii+1], dsp_sine_10[ii+2] );

    // Send OSC values downstream for use by other threads, ensure that they're less than +1.0.
    samples[2] = dsp_multiply( samples[2], level1 );
    samples[3] = dsp_multiply( samples[3], level2 );
    samples[4] = dsp_multiply( samples[4], level3 );
}

void app_thread2( int samples[32], const int property[6] ) // Play back wave data.
{
    samples[5] = 0;
}

void app_thread3( int samples[32], const int property[6] )
{
}

void app_thread4( int samples[32], const int property[6] )
{
}

void app_thread5( int samples[32], const int property[6] )
{
}
