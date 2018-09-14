#include <math.h>
#include <string.h>
#include <xccompat.h>
#include <xs1.h>

#include "xio.h"
#include "dsp.h"
#include "c99.h"

extern const int audio_sample_rate;

extern const char* control_labels[21];

static void _property_get_data( const int property[6], byte data[20] );
static void _property_set_data( int property[6], const byte data[20] );
static void _property_set_text( int property[6], const char data[20] );
static void _read_adc( double values[4] );

static byte _preset_data[4096];

int _master_volume = 0, _master_preset = 0, _master_sync = 0;
int _master_tone_coeff[8] = {FQ(1.0),0,0,0,0,0}, _master_tone_state[4] = {0,0,0,0};
int _footswitch_short_press = 0, _footswitch_long_press = 0;

void xio_control( const int rcv_prop[6], int snd_prop[6], int dsp_prop[6] )
{
    static int state = 0;
        
    if( state == 0 )
    {
        state = 1;
        //flash_read( 0, _preset_data );
        if( 1 || _preset_data[0] == 0xFF ) { // FLASH has been erased.
            memset( _preset_data, 50, 5*20 ); // Initialize params to default (50=midpoint).
            //flash_write( 0, _preset_data );
        }
        return;
    }
    
    if( rcv_prop[0] == 0 )
    {
        double tone, pots[8] = {0,0,0,0,0,0,0,0};
        _read_adc( pots );
        
        _master_volume = FQ( 0.25 + (0.75 * pots[2]) );
        tone = pots[1];
        calc_lowpass( _master_tone_coeff, (1000+12000*tone)/audio_sample_rate, 0.707 );

        static int preset = 0;
        if( preset < (int)(400 * pots[0]) - 3 || preset > (int)(400 * pots[0]) + 3 )
        {
            snd_prop[0] = 0x2102;
            snd_prop[1] = preset = (int)(400 * pots[0]);
        }

        //static int pre1 = 1, pre2 = 1;
        //pre1 = (int)(1 + 254 * pots[0]); if( pre1 == 0 ) pre1 = 0; if( pre1 >= 255 ) pre1 = 255;
        //if( pre1 != pre2 ) serial_write( pre2 = pre1 );
        //if( _footswitch_long_press ) { _footswitch_long_press = 0; serial_write( pre2 ); }

        //if( serial_count() > 0 )
        {
            //byte data = serial_read();
            //double knob = (data-1) / 254.0, ratio = 0.0;
            int idx1 = 0, idx2 = 0;
            //if( knob>=0.00 && knob<0.25 ) {idx1=0; idx2=1; ratio=(4*(knob-0.00));}
            //if( knob>=0.25 && knob<0.50 ) {idx1=1; idx2=2; ratio=(4*(knob-0.25));}
            //if( knob>=0.50 && knob<0.75 ) {idx1=2; idx2=3; ratio=(4*(knob-0.50));}
            //if( knob>=0.75 && knob<1.00 ) {idx1=3; idx2=4; ratio=(4*(knob-0.75));}
        
            idx1 = 0; idx2 = 0;
        
            double parameters[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
            //double targets[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
            for( int ii = 0; ii < 20; ++ii )
            {
                double param1 = _preset_data[20*idx1+ii] / 100.0;
                //double param2 = _preset_data[20*idx2+ii] / 100.0;
                parameters[ii] = param1;// * (1-ratio) + param2 * ratio;
                //double diff = targets[ii] - parameters[ii];
                //if( diff > +0.01 ) diff = +0.01; if( diff > -0.01 ) diff = -0.01;
                //parameters[ii] += diff;
            }
            c99_control( parameters, dsp_prop );
        }
    }

    // 20nn - Read parameter label for parameter N
    else if( (rcv_prop[0] & 0xFF00) == 0x2000 )
    {
        int ii = rcv_prop[0] & 0xFF;
        snd_prop[0] = rcv_prop[0];
        _property_set_text( snd_prop, (void*) control_labels[ii] );
    }
    // 2100 - Read the current preset_num,volume,tone,preset,bypass settings
    // 2101 - Write the current preset_num,volume,tone,preset,bypass settings
    // 2102 - Notification of changes to preset_num,volume,tone,preset,bypass settings
    //else if( (rcv_prop[0] & 0xFFFF) == 0x2100 )
    //{
    //    snd_prop[0] = rcv_prop[0]; snd_prop[1] = preset;
    //}
    //else if( (rcv_prop[0] & 0xFFFF) == 0x2101 )
    //{
    //    snd_prop[0] = rcv_prop[0]; _master_sync = rcv_prop[1];
    //}
    // 21p0 - Read preset parameter values for preset P (0 <= P < 16)
    // 21p1 - Write preset parameter values for preset P (0 <= P < 16)
    else if( (rcv_prop[0] & 0xFF0F) == 0x2100)
    {
        int pp = (rcv_prop[0] & 0x00F0) >> 4;
        snd_prop[0] = (rcv_prop[0] & 0xFF0F) + 16*pp;
        _property_set_data( snd_prop, _preset_data + 20*pp );
    }
    else if( (rcv_prop[0] & 0xFF0F) == 0x2101)
    {
        int pp = (rcv_prop[0] & 0x00F0) >> 4;
        snd_prop[0] = (rcv_prop[0] & 0xFF0F) + 16*pp;
        _property_get_data( rcv_prop, _preset_data + 20*pp );
    }
}

void xio_mixer( const int usb_output[32], int usb_input[32],
                const int adc_output[32], int dac_input[32],
                const int dsp_output[32], int dsp_input[32], const int property[6] )
{
    //static int active = 0, footsw = 0, sync_tx = 0, sync_rx = 0;
    //int level, period = audio_sample_rate / 1000;
    
    int input  = adc_output[0]; // Guitar/instrument input Q31
    int result = dsp_output[0]; // DSP Preamp output Q28
    
    static int cc = 0; result = cc++;

    result = dsp_iir2( result, _master_tone_coeff, _master_tone_state ); // Tone knob/control
    result = dsp_mul ( result, _master_volume ); // Volume knob/control

    usb_input[0] = input;      // USB input left Q31 = guitar input (ADC) Q31
    usb_input[1] = result * 8; // USB input right Q31 = DSP result Q28
    dsp_input[0] = input  / 8; // DSP input Q28 = guitar input Q31
    dac_input[0] = result * 8; // DAC left Q31 = DSP result Q28
    dac_input[1] = result * 8; // DAC right Q31 = DSP result Q28

    /*
    if( sync_tx > 0 ) { i2s_input[2] = 0xFFFFFFFF; --sync_tx; } // Transmit sync signal to others
    else
    {
        i2s_input[2] = 0;
        level = i2s_input[2]; // Determine foot-switch hold time.
        if( level == 0xFFFFFFFF && footsw < 999999 ) ++footsw; // Being held, keep counting.    
        if( level == 0x00000000 ) { // Released after being held.
            if( footsw >=  100*period && footsw < 500*period ) { // Toggle active/bypass mode.
                if( active ) active = 0;
                else active = 1;
            }
            if( footsw >=  500*period ) sync_tx = (_master_preset+1) * 20*period;
            footsw = 0;
        }
        level = i2s_input[4]; // Determine sense hold time (e.g. which preset is being indicated).
        if( level == 0xFFFFFFFF ) ++sync_rx; // Being held, keep counting.
        if( level == 0x00000000 ) { // Released after being held, select preset based on hold time.
            if( sync_rx >= 10*period && sync_rx <  30*period ) _master_sync = 0; // Preset 1
            if( sync_rx >= 30*period && sync_rx <  50*period ) _master_sync = 1; // Preset 2
            if( sync_rx >= 50*period && sync_rx <  70*period ) _master_sync = 2; // Preset 3
            if( sync_rx >= 70*period && sync_rx <  90*period ) _master_sync = 3; // Preset 4
            if( sync_rx >= 90*period && sync_rx < 110*period ) _master_sync = 4; // Preset 5
            sync_rx = 0;
        }
    }
    i2s_input[2] = active; // Set LED to 1 if footsw == 1 (active, not bypassed)
    */
    c99_mixer( usb_output, usb_input, adc_output, dac_input, dsp_output, dsp_input, property );
}

static void _property_get_data( const int property[6], byte data[20] )
{
	for( int nn = 0; nn < 5; ++nn ) {
		data[4*nn+0] = (byte)(property[nn+1] >> 24);
		data[4*nn+1] = (byte)(property[nn+1] >> 16);
		data[4*nn+2] = (byte)(property[nn+1] >>  8);
		data[4*nn+3] = (byte)(property[nn+1] >>  0);
	}
}

static void _property_set_data( int property[6], const byte data[20] )
{
	for( int nn = 0; nn < 5; ++nn ) {
		property[nn+1] = (((unsigned)data[4*nn+0])<<24)
					   + (((unsigned)data[4*nn+1])<<16)
					   + (((unsigned)data[4*nn+2])<< 8)
					   + (((unsigned)data[4*nn+3])<< 0);
	}
}

static void _property_set_text( int property[6], const char data[20] )
{
	property[1] = property[2] = property[3] = property[4] = property[5] = 0;
	if( data[0] == 0 ) return;
	for( int nn = 0; nn < 5; ++nn ) {
	    if( data[4*nn+0] == 0 ) break;
	    property[nn+1] += (((unsigned)data[4*nn+0])<<24);
	    if( data[4*nn+1] == 0 ) break;
	    property[nn+1] += (((unsigned)data[4*nn+1])<<16);
	    if( data[4*nn+2] == 0 ) break;
	    property[nn+1] += (((unsigned)data[4*nn+2])<< 8);
	    if( data[4*nn+3] == 0 ) break;
	    property[nn+1] += (((unsigned)data[4*nn+3])<< 0);
	}
}

static void _read_adc( double values[4] )
{
    i2c_start(100000); i2c_write(0xC8); i2c_write(0x60+1);
    i2c_start(100000); i2c_write(0xC9); timer_delay(100);
    values[2] = (double)i2c_read()/256; i2c_ack(0);
    i2c_start(100000); i2c_write(0xC8); i2c_write(0x60+3);
    i2c_start(100000); i2c_write(0xC9); timer_delay(100);
    values[0] = (double)i2c_read()/256; i2c_ack(0);
    i2c_start(100000); i2c_write(0xC8); i2c_write(0x60+5);
    i2c_start(100000); i2c_write(0xC9); timer_delay(100);
    values[1] = (double)i2c_read()/256; i2c_ack(0);
    i2c_start(100000); i2c_write(0xC8); i2c_write(0x60+7);
    i2c_start(100000); i2c_write(0xC9); timer_delay(100);
    values[3] = (double)i2c_read()/256; i2c_ack(0);
    i2c_stop();
}
