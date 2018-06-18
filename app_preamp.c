// Three preamp gain stages in series per left and right channel with internal highly
// oversampled signal processing for articulate stereo overdrive/distortion voicing. The second
// and third preamp stages for the left and right channels incorporate adjustable pre-filtering
// and bias settings, midrange emphasis filter frequency and Q settings, and a tube-based gain
// model with slew-rate limiting creating a configurable tube-like multi-stage guitar preamp.
// Up to nine presets and USB/MIDI control. 

#include <math.h>
#include <string.h>
#include "flexfx.i"

const char* product_name_string   = "FlexFX Preamp";
const char* usb_audio_output_name = "FlexFX Audio Out";
const char* usb_audio_input_name  = "FlexFX Audio In";
const char* usb_midi_output_name  = "FlexFX MIDI Out";
const char* usb_midi_input_name   = "FlexFX MIDI In";

const int audio_sample_rate     = 192000;
const int usb_output_chan_count = 2;
const int usb_input_chan_count  = 2;
const int i2s_channel_count     = 2;
const int i2s_is_bus_master     = 1;

const double pi = 3.14159265359;

const int i2s_sync_word[8] = { 0xFFFFFFFF,0x00000000,0,0,0,0,0,0 }; // I2S WCLK values per slot

const char controller_script[] = \
	"" \
	"ui_title( 'FlexFX Preamp'," \
	    "['','Drive','LowCut', 'Voice2','Volume', '','Drive','LowCut', 'Voice2','Volume' ]," \
	    "['','Emph', 'Slewlim','Voice3','Balance','','Emph', 'Slewlim','Voice3','Balance'] );" \
	"ui_separator();" \
	"ui_param( 'f', 15, 'Preamp Drive (Left)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'l', 15, 'Midrange Boost (Left)', ['Min','','','','','','','','','','','','','','Max'],'Midrange boost frequency before stages 2 and 3' );" \
	"ui_param( 'f', 15, 'Bass Cut (Left)', [],'Reduces bass before preamp stages 2 and 3' );" \
	"ui_param( 'l', 15, 'Slew Rate (Left)', [],'Increase slew-rate limiting for preamp stages 2 and 3' );" \
	"ui_param( 'f', 15, 'Stage 2 Voice (Left)', ['Soft/Cold','Soft/Cool','Soft/Norm','Soft/Warm','Soft/Hot','Tube/Cold','Tube/Cool','Tube/Norm','Tube/Warm','Tube/Hot','Hard/Cold','Hard/Cool','Hard/Norm','Hard/Warm','Hard/Hot'],'Bias and clip shape (soft/hard are symmetrical, tube is asym)' );" \
	"ui_param( 'l', 15, 'Stage 3 Voice (Left)', ['Soft/Cold','Soft/Cool','Soft/Norm','Soft/Warm','Soft/Hot','Tube/Cold','Tube/Cool','Tube/Norm','Tube/Warm','Tube/Hot','Hard/Cold','Hard/Cool','Hard/Norm','Hard/Warm','Hard/Hot'],'Bias and clip shape (soft/hard are symmetrical, tube is asym)' );" \
	"ui_param( 'f', 99, 'Output Volume (Left)', [],'' );" \
	"ui_param( 'l', 99, 'Output Balance (Left)', [],'' );" \
	"ui_separator();" \
	"ui_param( 'f', 15, 'Preamp Drive (Left)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'l', 15, 'Midrange Boost (Left)', ['Min','','','','','','','','','','','','','','Max'],'Midrange boost frequency before stages 2 and 3' );" \
	"ui_param( 'f', 15, 'Bass Cut (Left)', [],'Reduces bass before preamp stages 2 and 3' );" \
	"ui_param( 'l', 15, 'Slew Rate (Left)', [],'Increase slew-rate limiting for preamp stages 2 and 3' );" \
	"ui_param( 'f', 15, 'Stage 2 Voice (Left)', ['Soft/Cold','Soft/Cool','Soft/Norm','Soft/Warm','Soft/Hot','Tube/Cold','Tube/Cool','Tube/Norm','Tube/Warm','Tube/Hot','Hard/Cold','Hard/Cool','Hard/Norm','Hard/Warm','Hard/Hot'],'Bias and clip shape (soft/hard are symmetrical, tube is asym)' );" \
	"ui_param( 'l', 15, 'Stage 3 Voice (Left)', ['Soft/Cold','Soft/Cool','Soft/Norm','Soft/Warm','Soft/Hot','Tube/Cold','Tube/Cool','Tube/Norm','Tube/Warm','Tube/Hot','Hard/Cold','Hard/Cool','Hard/Norm','Hard/Warm','Hard/Hot'],'Bias and clip shape (soft/hard are symmetrical, tube is asym)' );" \
	"ui_param( 'f', 99, 'Output Volume (Left)', [],'' );" \
	"ui_param( 'l', 99, 'Output Balance (Left)', [],'' );" \
	"";

int _preamp_antialias_coeff[48] = // util_fir.py 0.0 0.165 1.0 -118
{
    FQ(-0.000000257),FQ(-0.000004062),FQ(-0.000017834),FQ(-0.000044475),FQ(-0.000067942),
    FQ(-0.000035800),FQ(+0.000141136),FQ(+0.000548053),FQ(+0.001174548),FQ(+0.001799398),
    FQ(+0.001924536),FQ(+0.000854644),FQ(-0.002012794),FQ(-0.006719312),FQ(-0.012264136),
    FQ(-0.016373514),FQ(-0.015772830),FQ(-0.007053819),FQ(+0.012039884),FQ(+0.041349045),
    FQ(+0.077612027),FQ(+0.114780889),FQ(+0.145409249),FQ(+0.162733367),FQ(+0.162733367),
    FQ(+0.145409249),FQ(+0.114780889),FQ(+0.077612027),FQ(+0.041349045),FQ(+0.012039884),
    FQ(-0.007053819),FQ(-0.015772830),FQ(-0.016373514),FQ(-0.012264136),FQ(-0.006719312),
    FQ(-0.002012794),FQ(+0.000854644),FQ(+0.001924536),FQ(+0.001799398),FQ(+0.001174548),
    FQ(+0.000548053),FQ(+0.000141136),FQ(-0.000035800),FQ(-0.000067942),FQ(-0.000044475),
    FQ(-0.000017834),FQ(-0.000004062),FQ(-0.000000257)
};

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

static int master_volume = 0, master_tone = 0;
int tone_coeffs[4] = {FQ(1.0),0,0,0}, tone_stateL[2] = {0,0}, tone_stateR[2] = {0,0};

void app_control( const int rcv_prop[6], int snd_prop[6], int dsp_prop[6] )
{
	static int preset = 0, countdown = 0;

	static byte presets[16][20] =
	{
		{ 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 50,50, 0,0,0,0 },
		{ 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 50,50, 0,0,0,0 },
		{ 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 50,50, 0,0,0,0 },
		{ 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 50,50, 0,0,0,0 },
		{ 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 50,50, 0,0,0,0 },
		{ 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 50,50, 0,0,0,0 },
		{ 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 50,50, 0,0,0,0 },
		{ 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 50,50, 0,0,0,0 },
		{ 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 50,50, 0,0,0,0 },
		{ 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 50,50, 0,0,0,0 },
		{ 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 50,50, 0,0,0,0 },
		{ 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 50,50, 0,0,0,0 },
		{ 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 50,50, 0,0,0,0 },
		{ 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 50,50, 0,0,0,0 },
		{ 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 50,50, 0,0,0,0 },
		{ 8,8, 8,8, 8,8, 50,50, 8,8, 8,8, 8,8, 50,50, 0,0,0,0 },
	};
	static byte params[20] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	
	if( preset == 0 ) { // Initialize
		preset = 1;
		//flash_open();
		//flash_read( 0, (void*)presets, sizeof(presets) );
		//flash_close();
	}

    static int value, previous = -1;
    double pots[8];
    adc_read( pots ); pots[0] = pots[1] = 0.5; pots[2] = 0.0;
    
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
    
    master_volume = pots[0]; master_tone = pots[1];
    calc_lowpass( tone_coeffs, 2000 + master_tone * 10000, 0.707 );
    
    snd_prop[0] = 0xFFFFFFFF;
    snd_prop[1] = master_volume; snd_prop[2] = master_tone; snd_prop[3] = pots[2];
    snd_prop[4] = snd_prop[5] = 0;
    
    return;
    
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
	
	//if( countdown > 0 ) {
	//	if( --countdown == 0 ) {
	//		flash_open(); flash_write( 0, (void*)presets, sizeof(presets) ); flash_close();
	//	}
	//}

	// Parameters ...
    // 0=drive,1=Emph,2=LoCut,3=Slew,4=voice1,5=voice2,6=volume,7=balance
	
	// Voices ...
	// 1 ='Soft/Cold', 2='Soft/Cool', 3='Soft/Norm', 4='Soft/Warm', 5='Soft/Hot',
	// 6 ='Tube/Cold', 7='Tube/Cool', 8='Tube/Norm', 9='Tube/Warm',10='Tube/Hot',
	// 11='Hard/Cold',12='Hard/Cool',13='Hard/Norm',14='Hard/Warm',15='Hard/Hot',
	
	// DSP Property --> Model Coefficients ...
	// Block, Bias, Type, Slew, Drive --> DCblock, Bias, Type, Slew

    if( params[0] != presets[preset][0] || params[2] != presets[preset][2] || // Block,Gain,Voice
        params[3] != presets[preset][3] )
    {
        params[0] = presets[preset][0]; params[2] = presets[preset][2];
        params[3] = presets[preset][3];
        static int bias[15] = { FQ(+0.02),FQ(+0.01),FQ(0),FQ(-0.01),FQ(-0.02), FQ(+0.02),FQ(+0.01),FQ(0),FQ(-0.01),FQ(-0.02),FQ(+0.02),FQ(+0.01),FQ(0),FQ(-0.01),FQ(-0.02) };
        static int type[15] = { 1,1,1,1,1, 0,0,0,0,0, 2,2,2,2,2 };
		dsp_prop[0] = 1;
		dsp_prop[1] = FQ(0.999);//FQ( exp(-2.0*pi*(5.0*params[2])/576000.0) ); // Block
		dsp_prop[2] = bias[params[4]-1]; // Bias
		dsp_prop[3] = 0; // = type[params[4]-1]; // Type
		dsp_prop[4] = FQ( 0.02 + ((params[3]/15.001)*(0.08-0.02)) ); // Slew
		dsp_prop[5] = FQ( ((double)params[0])/15.001 ); // Drive
    }
    else if( params[1] != presets[preset][1] ) // EmphF,EmphQ
    {
        params[1] = presets[preset][1];
		dsp_prop[0] = 2;
		//calc_peaking( dsp_prop+1, 0.00005+params[2]*0.0001, 0.3+(params[3]*0.06), 6.0 );
		calc_peaking( dsp_prop+1, 0.00005+params[1]*0.0001, 0.707, 6.0 );
    }
    else if( params[6] != presets[preset][6] || params[7] != presets[preset][7] ) // Vol,Bal
    {
        params[6] = presets[preset][6]; params[7] = presets[preset][7];
		dsp_prop[0] = 3;
		dsp_prop[2] = FQ(0.5); //FQ( 0.0666 * params[9] ); // Balance
		dsp_prop[3] = FQ( ((double)params[6]) / 200 );
    }
}

void app_mixer( const int usb_output[32], int usb_input[32],
                const int i2s_output[32], int i2s_input[32],
                const int dsp_output[32], int dsp_input[32], const int property[6] )
{
    // Send pseudo-differential ADC input signal to the DSP threads and to USB audio in.
    dsp_input[0] = usb_input[0] = i2s_output[0] / 2 - i2s_output[1] / 2; // Left
    dsp_input[1] = usb_input[1] = i2s_output[0] / 2 - i2s_output[1] / 2; // Right
    
    // Convert I2S output format Q31 to DSP input format Q28.
    dsp_input[0] /= 8; dsp_input[1] /= 8;
    
    // Apply master volume and send DSP left/right outputs to left/right DAC channels.
    i2s_input[0] = dsp_mul( dsp_output[0], master_volume ); // Left
    i2s_input[1] = dsp_mul( dsp_output[1], master_volume ); // Right
    
    // Apply master tone control to left/right channels before sending to the DAC and
    // convert from Q28 to Q31.
    i2s_input[0] = dsp_iir1( i2s_input[0], tone_coeffs, tone_stateL ) * 8;
    i2s_input[1] = dsp_iir1( i2s_input[1], tone_coeffs, tone_stateR ) * 8;
    
    // Adjust preamp outputs to be instrument level rather than line level.
    i2s_input[0] /= 8; i2s_input[1] /= 8;
}

int _preamp_amp1_coeff[18] = { 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0 };
int _preamp_amp2_coeff[18] = { 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0 };
int _preamp_amp3_coeff[18] = { 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0 };
int _preamp_amp4_coeff[18] = { 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0 };
int _preamp_amp5_coeff[18] = { 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0 };

int _preamp_amp1_state[18] = { 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0 };
int _preamp_amp2_state[18] = { 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0 };
int _preamp_amp3_state[18] = { 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0 };
int _preamp_amp4_state[18] = { 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0 };
int _preamp_amp5_state[18] = { 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,0,0,0,0 };

int _preamp_upsample_state[48];
int _preamp_dnsample_state[48];

void copy_prop( int dst[6], const int src[5] )
{
	dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
	dst[3] = src[3]; dst[4] = src[4];
}

void app_initialize( void )
{
}

void app_thread1( int samples[32], const int property[6] )
{
    _dsp_upsample( samples, _preamp_antialias_coeff,_preamp_upsample_state, 48, 3 );

	samples[2] = efx_preamp1( samples[2], _preamp_amp1_coeff, _preamp_amp1_state );
	samples[1] = efx_preamp1( samples[1], _preamp_amp1_coeff, _preamp_amp1_state );
	samples[0] = efx_preamp1( samples[0], _preamp_amp1_coeff, _preamp_amp1_state );

	samples[5] = dsp_mul( samples[2], _preamp_amp4_coeff[4] );
	samples[4] = dsp_mul( samples[1], _preamp_amp4_coeff[4] );
	samples[3] = dsp_mul( samples[0], _preamp_amp4_coeff[4] );

	samples[2] = dsp_mul( samples[2], _preamp_amp2_coeff[4] );
	samples[1] = dsp_mul( samples[1], _preamp_amp2_coeff[4] );
	samples[0] = dsp_mul( samples[0], _preamp_amp2_coeff[4] );
}

void app_thread2( int samples[32], const int property[6] )
{
	samples[2] = efx_preamp2( samples[2], _preamp_amp2_coeff, _preamp_amp2_state );
	samples[1] = efx_preamp2( samples[1], _preamp_amp2_coeff, _preamp_amp2_state );
	samples[0] = efx_preamp2( samples[0], _preamp_amp2_coeff, _preamp_amp2_state );

	samples[2] = efx_preamp2( samples[2], _preamp_amp3_coeff, _preamp_amp3_state );
}

void app_thread3( int samples[32], const int property[6] )
{
	samples[1] = efx_preamp2( samples[1], _preamp_amp3_coeff, _preamp_amp3_state );
	samples[0] = efx_preamp2( samples[0], _preamp_amp3_coeff, _preamp_amp3_state );

	samples[5] = efx_preamp2( samples[5], _preamp_amp4_coeff, _preamp_amp4_state );
	samples[4] = efx_preamp2( samples[4], _preamp_amp4_coeff, _preamp_amp4_state );
}

void app_thread4( int samples[32], const int property[6] )
{	
	samples[3] = efx_preamp2( samples[3], _preamp_amp4_coeff, _preamp_amp4_state );
	
	samples[5] = efx_preamp2( samples[5], _preamp_amp5_coeff, _preamp_amp5_state );
	samples[4] = efx_preamp2( samples[4], _preamp_amp5_coeff, _preamp_amp5_state );
	samples[3] = efx_preamp2( samples[3], _preamp_amp5_coeff, _preamp_amp5_state );
}

void app_thread5( int samples[32], const int property[6] )
{
	samples[2] = dsp_mul( samples[2], _preamp_amp2_coeff[14] );
	samples[1] = dsp_mul( samples[1], _preamp_amp2_coeff[14] );
	samples[0] = dsp_mul( samples[0], _preamp_amp2_coeff[14] );

	samples[5] = dsp_mul( samples[5], _preamp_amp4_coeff[14] );
	samples[4] = dsp_mul( samples[4], _preamp_amp4_coeff[14] );
	samples[3] = dsp_mul( samples[3], _preamp_amp4_coeff[14] );
	
	//samples[2] += samples[5]; samples[1] += samples[4]; samples[0] += samples[3];

    _dsp_dnsample( samples, _preamp_antialias_coeff, _preamp_dnsample_state, 48, 3 ) ;
    
    if( property[0] ==  1 ) copy_prop( _preamp_amp2_coeff+ 0, property+1 );
    if( property[0] ==  1 ) copy_prop( _preamp_amp3_coeff+ 0, property+1 );
    if( property[0] ==  2 ) copy_prop( _preamp_amp2_coeff+ 6, property+1 );
    if( property[0] ==  2 ) copy_prop( _preamp_amp3_coeff+ 6, property+1 );
    if( property[0] ==  3 ) copy_prop( _preamp_amp2_coeff+12, property+1 );
    if( property[0] ==  3 ) copy_prop( _preamp_amp3_coeff+12, property+1 );
    if( property[0] ==  4 ) copy_prop( _preamp_amp4_coeff+ 0, property+1 );
    if( property[0] ==  4 ) copy_prop( _preamp_amp5_coeff+ 0, property+1 );
    if( property[0] ==  5 ) copy_prop( _preamp_amp4_coeff+ 6, property+1 );
    if( property[0] ==  5 ) copy_prop( _preamp_amp5_coeff+ 6, property+1 );
    if( property[0] ==  6 ) copy_prop( _preamp_amp4_coeff+12, property+1 );
    if( property[0] ==  6 ) copy_prop( _preamp_amp5_coeff+12, property+1 );
    
    if( (property[0] & 0x80000000) == 0x80000000 )
    {
    	//irdata[ (property[0] & 0xFFFF) + 0 ] = property[1];
    	//irdata[ (property[0] & 0xFFFF) + 1 ] = property[2];
    	//irdata[ (property[0] & 0xFFFF) + 2 ] = property[3];
    	//irdata[ (property[0] & 0xFFFF) + 3 ] = property[4];
    	//irdata[ (property[0] & 0xFFFF) + 4 ] = property[5];
    }
}
