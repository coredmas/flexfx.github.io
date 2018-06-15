// Stereo multi-voice flanger and chorus with up to three chorus voices per channel (left and
// right) each with their own settings for LFO rate, base delay, modulated delay/depth, high
// and low-pass filters for the feedback signal, regeneration/feedback level, and wet/dry mix.
// Up to nine presets and USB/MIDI control. ​

#include <math.h>
#include <string.h>
#include "flexfx.i"

const char* product_name_string   = "FlexFX Dual Chorus";
const char* usb_audio_output_name = "FlexFX Audio Out";
const char* usb_audio_input_name  = "FlexFX Audio In";
const char* usb_midi_output_name  = "FlexFX MIDI Out";
const char* usb_midi_input_name   = "FlexFX MIDI In";

const int audio_sample_rate     = 48000;
const int usb_output_chan_count = 2;
const int usb_input_chan_count  = 2;
const int i2s_channel_count     = 2;
const int i2s_is_bus_master     = 1;

const int i2s_sync_word[8] = { 0xFFFFFFFF,0x00000000,0,0,0,0,0,0 }; // I2S WCLK values per slot

const char controller_script[] = \
	"" \
	"ui_title( 'FlexFX Dual Chorus'," \
	    "['Blend', 'Rate', 'HiCut','Volume', 'Blend', 'Rate', 'HiCut','Volume']," \
	    "['Feedbk','Depth','LoCut','Balance','Feedbk','Depth','LoCut','Balance'] );" \
    "" \
	"ui_param( 'f', 15, 'Wet/Dry Blend (Left)', [],'Minimum is 100% dry, maximum is 100% wet' );" \
	"ui_param( 'l', 15, 'Feedback Ratio (Left)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'f', 15, 'Modulation Rate (Left)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'l', 15, 'Modulation Depth (Left)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'f', 15, 'Feedback Treble Cut (Left)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'l', 15, 'Feedback Bass Cut (Left)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'f', 99, 'Output Volume (Left)', [],'' );" \
	"ui_param( 'l', 99, 'Output Balance (Left)', [],'' );" \
    "" \
	"ui_param( 'f', 15, 'Wet/Dry Blend (Right)', [],'Minimum is 100% dry, maximum is 100% wet' );" \
	"ui_param( 'l', 15, 'Feedback Ratio (Right)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'f', 15, 'Modulation Rate (Right)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'l', 15, 'Modulation Depth (Right)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'f', 15, 'Feedback Treble Cut (Right)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'l', 15, 'Feedback Bass Cut (Right)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'f', 99, 'Output Volume (Right)', [],'' );" \
	"ui_param( 'l', 99, 'Output Balance (Right)', [],'' );" \
	"";

void property_get_data( const int property[6], byte data[20] )
{
	for( int nn = 0; nn < 5; ++nn ) {
		data[4*nn+0] = (byte)(property[nn+1] >> 24);
		data[4*nn+1] = (byte)(property[nn+1] >> 16);
		data[4*nn+2] = (byte)(property[nn+1] >>  8);
		data[4*nn+3] = (byte)(property[nn+1] >>  0);
	}
}

void property_set_data( int property[6], const byte data[20] )
{
	for( int nn = 0; nn < 5; ++nn ) {
		property[nn+1] = (((unsigned)data[4*nn+0])<<24)
					   + (((unsigned)data[4*nn+1])<<16)
					   + (((unsigned)data[4*nn+2])<< 8)
					   + (((unsigned)data[4*nn+3])<< 0);
	}
}

void app_control( const int rcv_prop[6], int snd_prop[6], int dsp_prop[6] )
{
	static int preset = 0, countdown = 0;
	//static int irdata[240*24/4], iroffs = 0;

	static byte presets[16][20] =
	{
		{ 8,8, 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 8,8, 50,50 },
		{ 8,8, 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 8,8, 50,50 },
		{ 8,8, 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 8,8, 50,50 },
		{ 8,8, 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 8,8, 50,50 },
		{ 8,8, 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 8,8, 50,50 },
		{ 8,8, 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 8,8, 50,50 },
		{ 8,8, 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 8,8, 50,50 },
		{ 8,8, 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 8,8, 50,50 },
		{ 8,8, 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 8,8, 50,50 },
		{ 8,8, 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 8,8, 50,50 },
		{ 8,8, 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 8,8, 50,50 },
		{ 8,8, 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 8,8, 50,50 },
		{ 8,8, 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 8,8, 50,50 },
		{ 8,8, 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 8,8, 50,50 },
		{ 8,8, 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 8,8, 50,50 },
		{ 8,8, 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 8,8, 50,50 },
	};
	static byte params[20] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	
	if( preset == 0 ) { // Initialize
		preset = 1;
		//flash_open();
		//flash_read( 0, (void*)presets, sizeof(presets) );
		//flash_close();
	}

    /*
    static int value, previous = -1;
    double pots[8]; adc_read( pots );
    if(                            pots[2] < (0.0625-0.03) ) value = 1;
    if( pots[2] > (0.0625+0.03) && pots[2] < (0.1875-0.03) ) value = 2;
    if( pots[2] > (0.1875+0.03) && pots[2] < (0.3125-0.03) ) value = 3;
    if( pots[2] > (0.3125+0.03) && pots[2] < (0.4375-0.03) ) value = 4;
    if( pots[2] > (0.4375+0.03) && pots[2] < (0.5625-0.03) ) value = 5;
    if( pots[2] > (0.5625+0.03) && pots[2] < (0.6875-0.03) ) value = 6;
    if( pots[2] > (0.6875+0.03) && pots[2] < (0.8125-0.03) ) value = 7;
    if( pots[2] > (0.8125+0.03) && pots[2] < (0.9375-0.03) ) value = 8;
    if( pots[2] > (0.9375+0.03)                            ) value = 9;
    if( value != previous ) preset = previous = value;
    */
    
	// Properties ...
    // 15n0   Read name of bulk data for preset P (P=0 for active config ...)
    // 15n1   Write name of bulk data and begin data upload for preset P
    // 15n2   Next 32 bytes of bulk data for preset (... or 1 <= P <= 9 ...)
    // 15n3   End bulk data upload (... for loading/storing for preset P)
	// 1700   Read preset data for the active preset, return 17p0 as property ID
	// 17p0   Read preset data for preset P (1 <= P <= 9)
	// 17p2   Notification of preset settings update for preset P
	// 17p3   Notification of preset settings and active preset update for preset P
	// 17p4   Update data for preset P (1 <= P <= 9)
	// 17p5   Update preset data and set the active preset to preset P (1 <= P <= 9)
	
	if( (rcv_prop[0] & 0xFF0F) == 0x1700 || (rcv_prop[0] & 0xFF0F) == 0x1701 )
	{
		int pp = (rcv_prop[0]>>4)&15; if( pp == 0 ) pp = 1;
		snd_prop[0] = (rcv_prop[0] & 0xFF0F) + (pp<<4);
		property_set_data( snd_prop, presets[pp] );
	}
	else if( (rcv_prop[0] & 0xFF0F) == 0x1704 || (rcv_prop[0] & 0xFF0F) == 0x1705 )
	{
		int pp = (rcv_prop[0]>>4)&15;
		property_get_data( rcv_prop, presets[pp] );
		if( (rcv_prop[0] & 0xFF0F) == 0x1705 ) preset = pp;
	}
	else if( (rcv_prop[0] & 0xFF0F) == 0x1501 || (rcv_prop[0] & 0xFF0F) == 0x1502 || (rcv_prop[0] & 0xFF0F) == 0x1503 )
	{
		static int base = 0, page = 0, offset = 0; static byte data[256];
		if( (rcv_prop[0] & 0xFF0F) == 0x1501 ) {
			page = offset = 0; base = (rcv_prop[0] >> 4) & 15;
			flash_open();
		}
		else if( (rcv_prop[0] & 0xFF0F) == 0x1502 || (rcv_prop[0] & 0xFF0F) == 0x1503 ) {
			memcpy( data + offset, rcv_prop + 1, 20 ); offset += 20;
			if( offset == 240 || (rcv_prop[0] & 0xFF0F) == 0x1503 ) {
				if( page < 24 ) flash_write( 1+base+page++, data, sizeof(data) );
				offset = 0;
			}
			if( (rcv_prop[0] & 0xFF0F) == 0x1503 ) flash_close();
		}
	}
}

void app_mixer( const int usb_output[32], int usb_input[32],
                const int i2s_output[32], int i2s_input[32],
                const int dsp_output[32], int dsp_input[32],
                const int property[6] )
{
    int guitar_in = i2s_output[0] - i2s_output[1];
    usb_input[0] = dsp_input[0] = guitar_in;
    usb_input[1] = dsp_input[1] = guitar_in;
    i2s_input[0] = i2s_input[1] = dsp_output[0];
}

void app_initialize( void ) {}

void app_thread1( int samples[32], const int property[6] )
{
    static int delta1 = FQ(3.3/audio_sample_rate);
    static int delta2 = FQ(2.7/audio_sample_rate);
    static int delta3 = FQ(1.5/audio_sample_rate);

    static int time1 = FQ(0.0); time1 += delta1; if(time1 > FQ(1.0)) time1 -= FQ(1.0);
    static int time2 = FQ(0.0); time2 += delta2; if(time2 > FQ(1.0)) time2 -= FQ(1.0);
    static int time3 = FQ(0.0); time3 += delta3; if(time3 > FQ(1.0)) time3 -= FQ(1.0);

    int ii, ff; // // snnniiii,iiiiiiff,ffffffff,ffffffff
    ii = (time1 & 0x0FFFFFFF) >> 18, ff = (time1 & 0x0003FFFF) << 10;
    samples[2] = dsp_lagrange( ff, dsp_sine_10[ii+0], dsp_sine_10[ii+1], dsp_sine_10[ii+2] );
    ii = (time2 & 0x0FFFFFFF) >> 18, ff = (time2 & 0x0003FFFF) << 10;
    samples[3] = dsp_lagrange( ff, dsp_sine_10[ii+0], dsp_sine_10[ii+1], dsp_sine_10[ii+2] );
    ii = (time3 & 0x0FFFFFFF) >> 18, ff = (time3 & 0x0003FFFF) << 10;
    samples[4] = dsp_lagrange( ff, dsp_sine_10[ii+0], dsp_sine_10[ii+1], dsp_sine_10[ii+2] );

    samples[2] = dsp_mul( samples[2], FQ(0.999) );
    samples[3] = dsp_mul( samples[3], FQ(0.999) );
    samples[4] = dsp_mul( samples[4], FQ(0.999) );
}

void app_thread2( int samples[32], const int property[6] )
{
    static int delay_fifo[1024], delay_index = 0;
    static int depth = FQ(+0.10);
    int lfo = (dsp_mul( samples[2], depth ) / 2) + FQ(0.4999); // [-1.0 < lfo < +1.0] to [+0.0 < lfo < +1.0]
    int ii = (lfo & 0x0FFFFFFF) >> 18, ff = (lfo & 0x0003FFFF) << 10; // snnniiii,iiiiiiff,ffffffff,ffffffff
    delay_fifo[delay_index-- & 1023] = samples[0]; // Update the sample delay line.
    int i1 = (delay_index+ii)&1023, i2 = (delay_index+ii+1)&1023, i3 = (delay_index+ii+2)&1023;
    samples[2] = dsp_lagrange( ff, delay_fifo[i1], delay_fifo[i2], delay_fifo[i3] );
}

void app_thread3( int samples[32], const int property[6] )
{
    static int delay_fifo[1024], delay_index = 0;
    static int depth = FQ(+0.10);
    int lfo = (dsp_mul( samples[3], depth ) / 2) + FQ(0.4999); // [-1.0 < lfo < +1.0] to [+0.0 < lfo < +1.0]
    int ii = (lfo & 0x0FFFFFFF) >> 18, ff = (lfo & 0x0003FFFF) << 10; // snnniiii,iiiiiiff,ffffffff,ffffffff
    delay_fifo[delay_index-- & 1023] = samples[0]; // Update the sample delay line.
    int i1 = (delay_index+ii)&1023, i2 = (delay_index+ii+1)&1023, i3 = (delay_index+ii+2)&1023;
    samples[3] = dsp_lagrange( ff, delay_fifo[i1], delay_fifo[i2], delay_fifo[i3] );
}

void app_thread4( int samples[32], const int property[6] )
{
    static int delay_fifo[1024], delay_index = 0;
    static int depth = FQ(+0.10);
    int lfo = (dsp_mul( samples[4], depth ) / 2) + FQ(0.4999); // [-1.0 < lfo < +1.0] to [+0.0 < lfo < +1.0]
    int ii = (lfo & 0x0FFFFFFF) >> 18, ff = (lfo & 0x0003FFFF) << 10; // snnniiii,iiiiiiff,ffffffff,ffffffff
    delay_fifo[delay_index-- & 1023] = samples[0]; // Update the sample delay line.
    int i1 = (delay_index+ii)&1023, i2 = (delay_index+ii+1)&1023, i3 = (delay_index+ii+2)&1023;
    samples[4] = dsp_lagrange( ff, delay_fifo[i1], delay_fifo[i2], delay_fifo[i3] );
}

void app_thread5( int samples[32], const int property[6] )
{
    int blend1 = FQ(+0.5), blend2 = FQ(+0.3), blend3 = FQ(+0.3);
    samples[2] = dsp_blend( samples[0], samples[2], blend1 );
    samples[3] = dsp_blend( samples[0], samples[3], blend2 );
    samples[4] = dsp_blend( samples[0], samples[4], blend3 );
    samples[0] = samples[0]/3 + samples[2]/3 + samples[3]/3;
}
