#include <math.h>
#include <string.h>
#include "flexfx.i"

const char* product_name_string   = "FlexFX Dual Ampsim";
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
	"ui_title( 'FlexFX Dual Ampsim'," \
	    "['','Drive',  'Stack','Middle','Volume', '','Drive',  'Stack','Middle','Volume' ]," \
	    "['','Cabinet','Bass', 'Treble','Balance','','Cabinet','Bass', 'treble','Balance'] );" \
	"ui_separator();" \
	"ui_param( 'f', 15, 'Input Drive (Left)', ['Min','','','','','','','','','','','','','','Max'], '' );" \
	"ui_param( 'l', 15, 'IR File Name (Left)', [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15], 'Selects the impulse response' );" \
	"ui_param( 'f', 15, 'Tone Stack Type (Left)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'l', 15, 'Bass Level (Left)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'f', 15, 'Midrange Level (Left)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'l', 15, 'Treble Level (Left)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'f', 99, 'Output Volume (Left)', [],'' );" \
	"ui_param( 'l', 99, 'Output Balance (Left)', [],'' );" \
	"ui_separator();" \
	"ui_param( 'f', 15, 'Input Drive (Right)', ['Min','','','','','','','','','','','','','','Max'], '' );" \
	"ui_param( 'l', 15, 'IR File Name (Right)', [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15], 'Selects the impulse response' );" \
	"ui_param( 'f', 15, 'Tone Stack Type (Left)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'l', 15, 'Bass Level (Left)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'f', 15, 'Midrange Level (Left)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'l', 15, 'Treble Level (Left)', ['Min','','','','','','','','','','','','','','Max'],'' );" \
	"ui_param( 'f', 99, 'Output Volume (Left)', [],'' );" \
	"ui_param( 'l', 99, 'Output Balance (Left)', [],'' );" \
	"";

#define _CONVOLVE_00(cc,ss) \
    asm("ldd   %0,%1,%2[0]":"=r"(b1),"=r"(b0):"r"(cc)); \
    asm("ldd   %0,%1,%2[0]":"=r"(s2),"=r"(s1):"r"(ss)); \
    asm("std   %0,%1,%2[0]"::"r"(s1), "r"(s0),"r"(ss)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b0),"r"(s0), "0"(ah),"1"(al)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b1),"r"(s1), "0"(ah),"1"(al));

#define _CONVOLVE_01(cc,ss) \
    asm("ldd   %0,%1,%2[1]":"=r"(b1),"=r"(b0):"r"(cc)); \
    asm("ldd   %0,%1,%2[1]":"=r"(s0),"=r"(s3):"r"(ss)); \
    asm("std   %0,%1,%2[1]"::"r"(s3), "r"(s2),"r"(ss)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b0),"r"(s2), "0"(ah),"1"(al)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b1),"r"(s3), "0"(ah),"1"(al));

#define _CONVOLVE_02(cc,ss) \
    asm("ldd   %0,%1,%2[2]":"=r"(b1),"=r"(b0):"r"(cc)); \
    asm("ldd   %0,%1,%2[2]":"=r"(s2),"=r"(s1):"r"(ss)); \
    asm("std   %0,%1,%2[2]"::"r"(s1), "r"(s0),"r"(ss)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b0),"r"(s0), "0"(ah),"1"(al)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b1),"r"(s1), "0"(ah),"1"(al));

#define _CONVOLVE_03(cc,ss) \
    asm("ldd   %0,%1,%2[3]":"=r"(b1),"=r"(b0):"r"(cc)); \
    asm("ldd   %0,%1,%2[3]":"=r"(s0),"=r"(s3):"r"(ss)); \
    asm("std   %0,%1,%2[3]"::"r"(s3), "r"(s2),"r"(ss)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b0),"r"(s2), "0"(ah),"1"(al)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b1),"r"(s3), "0"(ah),"1"(al));

#define _CONVOLVE_04(cc,ss) \
    asm("ldd   %0,%1,%2[4]":"=r"(b1),"=r"(b0):"r"(cc)); \
    asm("ldd   %0,%1,%2[4]":"=r"(s2),"=r"(s1):"r"(ss)); \
    asm("std   %0,%1,%2[4]"::"r"(s1), "r"(s0),"r"(ss)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b0),"r"(s0), "0"(ah),"1"(al)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b1),"r"(s1), "0"(ah),"1"(al));

#define _CONVOLVE_05(cc,ss) \
    asm("ldd   %0,%1,%2[5]":"=r"(b1),"=r"(b0):"r"(cc)); \
    asm("ldd   %0,%1,%2[5]":"=r"(s0),"=r"(s3):"r"(ss)); \
    asm("std   %0,%1,%2[5]"::"r"(s3), "r"(s2),"r"(ss)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b0),"r"(s2), "0"(ah),"1"(al)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b1),"r"(s3), "0"(ah),"1"(al));

#define _CONVOLVE_06(cc,ss) \
    asm("ldd   %0,%1,%2[6]":"=r"(b1),"=r"(b0):"r"(cc)); \
    asm("ldd   %0,%1,%2[6]":"=r"(s2),"=r"(s1):"r"(ss)); \
    asm("std   %0,%1,%2[6]"::"r"(s1), "r"(s0),"r"(ss)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b0),"r"(s0), "0"(ah),"1"(al)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b1),"r"(s1), "0"(ah),"1"(al));

#define _CONVOLVE_07(cc,ss) \
    asm("ldd   %0,%1,%2[7]":"=r"(b1),"=r"(b0):"r"(cc)); \
    asm("ldd   %0,%1,%2[7]":"=r"(s0),"=r"(s3):"r"(ss)); \
    asm("std   %0,%1,%2[7]"::"r"(s3), "r"(s2),"r"(ss)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b0),"r"(s2), "0"(ah),"1"(al)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b1),"r"(s3), "0"(ah),"1"(al));

#define _CONVOLVE_08(cc,ss) \
    asm("ldd   %0,%1,%2[8]":"=r"(b1),"=r"(b0):"r"(cc)); \
    asm("ldd   %0,%1,%2[8]":"=r"(s2),"=r"(s1):"r"(ss)); \
    asm("std   %0,%1,%2[8]"::"r"(s1), "r"(s0),"r"(ss)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b0),"r"(s0), "0"(ah),"1"(al)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b1),"r"(s1), "0"(ah),"1"(al));

#define _CONVOLVE_09(cc,ss) \
    asm("ldd   %0,%1,%2[9]":"=r"(b1),"=r"(b0):"r"(cc)); \
    asm("ldd   %0,%1,%2[9]":"=r"(s0),"=r"(s3):"r"(ss)); \
    asm("std   %0,%1,%2[9]"::"r"(s3), "r"(s2),"r"(ss)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b0),"r"(s2), "0"(ah),"1"(al)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b1),"r"(s3), "0"(ah),"1"(al));

#define _CONVOLVE_10(cc,ss) \
    asm("ldd   %0,%1,%2[10]":"=r"(b1),"=r"(b0):"r"(cc)); \
    asm("ldd   %0,%1,%2[10]":"=r"(s2),"=r"(s1):"r"(ss)); \
    asm("std   %0,%1,%2[10]"::"r"(s1), "r"(s0),"r"(ss)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b0),"r"(s0), "0"(ah),"1"(al)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b1),"r"(s1), "0"(ah),"1"(al));

#define _CONVOLVE_11(cc,ss) \
    asm("ldd   %0,%1,%2[11]":"=r"(b1),"=r"(b0):"r"(cc)); \
    asm("ldd   %0,%1,%2[11]":"=r"(s0),"=r"(s3):"r"(ss)); \
    asm("std   %0,%1,%2[11]"::"r"(s3), "r"(s2),"r"(ss)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b0),"r"(s2), "0"(ah),"1"(al)); \
    asm("maccs %0,%1,%2,%3" :"=r"(ah),"=r"(al):"r"(b1),"r"(s3), "0"(ah),"1"(al));

#define _CONVOLVE_24p(cc,ss) \
{ \
    _CONVOLVE_00(cc,ss); _CONVOLVE_01(cc,ss); _CONVOLVE_02(cc,ss); _CONVOLVE_03(cc,ss); \
    _CONVOLVE_04(cc,ss); _CONVOLVE_05(cc,ss); _CONVOLVE_06(cc,ss); _CONVOLVE_07(cc,ss); \
    _CONVOLVE_08(cc,ss); _CONVOLVE_09(cc,ss); _CONVOLVE_10(cc,ss); _CONVOLVE_11(cc,ss); \
}

/*
int _tone_coeff[16][6] = // util_iir.py lowpass (3000 * n^1.333) Hz) 0.707 0.0
{
    { FQ(+1.000921453),FQ(-1.998141473),FQ(+0.997226867),FQ(+1.998141473),FQ(-0.998148320),0 },
    { FQ(+1.001151550),FQ(-1.997675241),FQ(+0.996534388),FQ(+1.997675241),FQ(-0.997685938),0 },
    { FQ(+1.001381539),FQ(-1.997208370),FQ(+0.995842231),FQ(+1.997208370),FQ(-0.997223770),0 },
    { FQ(+1.001611421),FQ(-1.996740861),FQ(+0.995150396),FQ(+1.996740861),FQ(-0.996761817),0 },
    { FQ(+1.001841196),FQ(-1.996272714),FQ(+0.994458883),FQ(+1.996272714),FQ(-0.996300079),0 },
    { FQ(+1.002070864),FQ(-1.995803931),FQ(+0.993767692),FQ(+1.995803931),FQ(-0.995838556),0 },
    { FQ(+1.002300425),FQ(-1.995334511),FQ(+0.993076823),FQ(+1.995334511),FQ(-0.995377248),0 },
    { FQ(+1.002529879),FQ(-1.994864455),FQ(+0.992386277),FQ(+1.994864455),FQ(-0.994916156),0 },
    { FQ(+1.002759226),FQ(-1.994393765),FQ(+0.991696053),FQ(+1.994393765),FQ(-0.994455279),0 },
    { 0,0,0,0,0,0 }, { 0,0,0,0,0,0 }, { 0,0,0,0,0,0 }, { 0,0,0,0,0,0 },
    { 0,0,0,0,0,0 }, { 0,0,0,0,0,0 }, { 0,0,0,0,0,0 },
};
*/

static void _tonestack( int* cc, double l, double m, double t, double v1, double v2, double v3 )
{
    double C1 = 0.25e-9, C2 = 20.0e-9, C3 = 20.0e-9; C1 *= v1; C2 *= v2; C3 *= v3;
    double R1 = 250e3, R2 = 1e6, R3 = 25e3, R4 = 56e3;
    double Fs = 48000.0, k = 2 * Fs;
    double m2 = m * m;
    
    double b1=t*C1*R1+m*C3*R3+l*(C1*R2+C2*R2)+(C1*R3+C2*R3);
    double b2=t*(C1*C2*R1*R4+C1*C3*R1*R4)-m2*(C1*C3*R3*R3+C2*C3*R3*R3)+m*(C1*C3*R1*R3+C1*C3*R3*R3+C2*C3*R3*R3)+l*(C1*C2*R1*R2+C1*C2*R2*R4+C1*C3*R2*R4)+l*m*(C1*C3*R2*R3+C2*C3*R2*R3)+(C1*C2*R1*R3+C1*C2*R3*R4+C1*C3*R3*R4);
    double b3=l*m*(C1*C2*C3*R1*R2*R3+C1*C2*C3*R2*R3*R4)-m2*(C1*C2*C3*R1*R3*R3+C1*C2*C3*R3*R3*R4)+m*(C1*C2*C3*R1*R3*R3+C1*C2*C3*R3*R3*R4)+t*C1*C2*C3*R1*R3*R4-t*m*C1*C2*C3*R3*R3*R4+t*l*C1*C2*C3*R1*R2*R4;
    double a0=1;
    double a1=(C1*R1+C1*R3+C2*R3+C2*R4+C3*R4)+m*C3*R3+l*(C1*R2+C2*R2);
    double a2=m*(C1*C3*R1*R3-C2*C3*R3*R4+C1*C3*R3*R3+C2*C3*R3*R3)+l*m*(C1*C3*R2*R3+C2*C3*R2*R3)-m2*(C1*C3*R3*R3+C2*C3*R3*R3)+l*(C1*C2*R2*R4+C1*C2*R1*R2+C1*C3*R2*R4+C2*C3*R2*R4)+(C1*C2*R1*R4+C1*C3*R1*R4+C1*C2*R3*R4+C1*C2*R1*R3+C1*C3*R3*R4+C2*C3*R3*R4);
    double a3=l*m*(C1*C2*C3*R1*R2*R3+C1*C2*C3*R2*R3*R4)-m2*(C1*C2*C3*R1*R3*R3+C1*C2*C3*R3*R3*R4)+m*(C1*C2*C3*R3*R3*R4+C1*C2*C3*R1*R3*R3-C1*C2*C3*R1*R3*R4)+l*C1*C2*C3*R1*R2*R4+C1*C2*C3*R1*R3*R4;

    double B0 =       -b1*k -b2*k*k   -b3*k*k*k;
    double B1 =       -b1*k +b2*k*k +3*b3*k*k*k;
    double B2 =       +b1*k +b2*k*k -3*b3*k*k*k;
    double B3 =       +b1*k -b2*k*k   +b3*k*k*k;
    double A0 = -a0   -a1*k -a2*k*k   -a3*k*k*k;
    double A1 = -3*a0 -a1*k +a2*k*k +3*a3*k*k*k;
    double A2 = -3*a0 +a1*k +a2*k*k -3*a3*k*k*k;
    double A3 = -a0   +a1*k -a2*k*k   +a3*k*k*k;
    
    cc[0] = B0; cc[1] = B1; cc[2] = B2; cc[3] = B3;
    cc[4] = -A1/A0; cc[5] = -A2/A0; cc[6] = -A3/A0;
}

static int _volume_level = 0;
static int _tone_coeff[5] = {0,0,0,0,0}, _tone_state[2][4] = {{0,0,0,0},{0,0,0,0}};
static byte _wave_data[29*256];

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
    
    return;


/*
	else if( state == 99 )
	{
		//flash_open(); flash_read(0,(void*)irdata,sizeof(irdata)); flash_close();
		iroffs = 0;
		dsp_prop[0] = 0x80000000 + iroffs;
		dsp_prop[1] = irdata[iroffs++]; dsp_prop[2] = irdata[iroffs++];
		dsp_prop[3] = irdata[iroffs++]; dsp_prop[4] = irdata[iroffs++];
		dsp_prop[5] = irdata[iroffs++];
		if( iroffs == 240*24/4 ) state = 0;
	}
*/
}

void app_mixer( const int usb_output[32], int usb_input[32],
                const int i2s_output[32], int i2s_input[32],
                const int dsp_output[32], int dsp_input[32], const int property[6] )
{
    int guitar_in = i2s_output[0] - i2s_output[1];
    usb_input[0] = dsp_input[0] = guitar_in;
    usb_input[1] = dsp_input[1] = guitar_in;
    i2s_input[0] = i2s_input[1] = dsp_output[0];
    return;

    //int sampleL = dsp_iir( dsp_output[0], _tone_coeff, _tone_state[0], 1 );
    //int sampleR = dsp_iir( dsp_output[1], _tone_coeff, _tone_state[1], 1 );

    usb_input[0] = dsp_input[0] = i2s_output[0] / 8;
    usb_input[1] = dsp_input[3] = i2s_output[1] / 8;
    i2s_input[0] = dsp_output[0]*8/4 + usb_output[0]/8;
    i2s_input[1] = dsp_output[3]*8/4 + usb_output[1]/8;
}

int ir_coeff[3600], ir_state[3600];

void app_initialize( void )
{
    memset( ir_coeff, 0, sizeof(ir_coeff) );
    memset( ir_state, 0, sizeof(ir_state) );
    ir_coeff[0] = ir_coeff[312*5] = FQ(+1.0);
}

int _antialias_coeff[72] = // util_fir.py 0.1 0.21 1 -120
{
    FQ(-0.000000006),FQ(+0.000001381),FQ(+0.000004326),FQ(+0.000002405),FQ(-0.000013689),
    FQ(-0.000036610),FQ(-0.000027816),FQ(+0.000051040),FQ(+0.000160857),FQ(+0.000152874),
    FQ(-0.000105892),FQ(-0.000494599),FQ(-0.000567774),FQ(+0.000078702),FQ(+0.001180593),
    FQ(+0.001627896),FQ(+0.000335831),FQ(-0.002288016),FQ(-0.003858406),FQ(-0.001780131),
    FQ(+0.003640321),FQ(+0.007890688),FQ(+0.005361270),FQ(-0.004595658),FQ(-0.014391661),
    FQ(-0.012873566),FQ(+0.003753649),FQ(+0.024252322),FQ(+0.027758163),FQ(+0.001891265),
    FQ(-0.040159287),FQ(-0.060869171),FQ(-0.022662597),FQ(+0.080312227),FQ(+0.208722649),
    FQ(+0.297546418),FQ(+0.297546418),FQ(+0.208722649),FQ(+0.080312227),FQ(-0.022662597),
    FQ(-0.060869171),FQ(-0.040159287),FQ(+0.001891265),FQ(+0.027758163),FQ(+0.024252322),
    FQ(+0.003753649),FQ(-0.012873566),FQ(-0.014391661),FQ(-0.004595658),FQ(+0.005361270),
    FQ(+0.007890688),FQ(+0.003640321),FQ(-0.001780131),FQ(-0.003858406),FQ(-0.002288016),
    FQ(+0.000335831),FQ(+0.001627896),FQ(+0.001180593),FQ(+0.000078702),FQ(-0.000567774),
    FQ(-0.000494599),FQ(-0.000105892),FQ(+0.000152874),FQ(+0.000160857),FQ(+0.000051040),
    FQ(-0.000027816),FQ(-0.000036610),FQ(-0.000013689),FQ(+0.000002405),FQ(+0.000004326),
    FQ(+0.000001381),FQ(-0.000000006)
};

int _amp_lut[32768];

int _amp1_coeff[8] = { 0,0,0,0,0,0,0,0 }, _amp1_state[10] = { 0,0,0,0,0,0,0,0,0,0 };
int _amp2_coeff[8] = { 0,0,0,0,0,0,0,0 }, _amp2_state[10] = { 0,0,0,0,0,0,0,0,0,0 };

int _upsample_state1[72], _upsample_state2[72];
int _dnsample_state1[72], _dnsample_state2[72];

void app_thread1( int samples[32], const int property[6] )
{
    static int offset = 0, muted = 0;
    if( property[0] == 0x1501 ) { offset = 0; muted = 1; }
    if( property[0] == 0x1502 && offset < 1560-5 ) {
		ir_coeff[offset+0] = ir_coeff[offset+312*5+0] = property[1] / 32;
		ir_coeff[offset+1] = ir_coeff[offset+312*5+1] = property[2] / 32;
		ir_coeff[offset+2] = ir_coeff[offset+312*5+2] = property[3] / 32;
		ir_coeff[offset+3] = ir_coeff[offset+312*5+3] = property[4] / 32;
		ir_coeff[offset+4] = ir_coeff[offset+312*5+4] = property[5] / 32;
		offset += 5;
    }
    if( property[0] == 0x1501 ) { offset = 0; muted = 0; }

    //samples[4] = dsp_mul( samples[0], gain ); samples[0] = dsp_mul( samples[0], gain );
    //samples[5] = dsp_mul( samples[1], gain ); samples[1] = dsp_mul( samples[1], gain );
    //samples[6] = dsp_mul( samples[2], gain ); samples[2] = dsp_mul( samples[2], gain );
    //samples[7] = dsp_mul( samples[3], gain ); samples[3] = dsp_mul( samples[3], gain );
    
    _dsp_upsample( samples+0, _antialias_coeff,_upsample_state1, 72, 4 );
    _dsp_upsample( samples+4, _antialias_coeff,_upsample_state2, 72, 4 );

    samples[3] = efx_pwramp( samples[3], _amp1_coeff, _amp1_state );
    samples[2] = efx_pwramp( samples[2], _amp1_coeff, _amp1_state );
    samples[1] = efx_pwramp( samples[1], _amp1_coeff, _amp1_state );
    samples[0] = efx_pwramp( samples[0], _amp1_coeff, _amp1_state );

    samples[7] = efx_pwramp( samples[7], _amp2_coeff, _amp2_state );
    samples[6] = efx_pwramp( samples[6], _amp2_coeff, _amp2_state );
    samples[5] = efx_pwramp( samples[5], _amp2_coeff, _amp2_state );
    samples[4] = efx_pwramp( samples[4], _amp2_coeff, _amp2_state );
    
    _dsp_dnsample( samples+0, _antialias_coeff, _dnsample_state1, 72, 4 );
    _dsp_dnsample( samples+4, _antialias_coeff, _dnsample_state2, 72, 4 );
    
    //samples[0] = dsp_iir3( samples[0], _amp1_coeff, _amp1_state+4 );
    //samples[4] = dsp_iir3( samples[4], _amp2_coeff, _amp2_state+4 );
    samples[3] = samples[4];
}

void app_thread2( int samples[32], const int property[6] )
{
    int ah; unsigned al = 0, s0, b0, b1, s1, s2, s3;

    s0 = samples[0]; ah = 0, al = 1<<(QQ-1);
    _CONVOLVE_24p( ir_coeff+312*0+  0, ir_state+312*0+  0 );
    _CONVOLVE_24p( ir_coeff+312*0+ 24, ir_state+312*0+ 24 );
    _CONVOLVE_24p( ir_coeff+312*0+ 48, ir_state+312*0+ 48 );
    _CONVOLVE_24p( ir_coeff+312*0+ 72, ir_state+312*0+ 72 );
    _CONVOLVE_24p( ir_coeff+312*0+ 96, ir_state+312*0+ 96 );
    _CONVOLVE_24p( ir_coeff+312*0+120, ir_state+312*0+120 );
    _CONVOLVE_24p( ir_coeff+312*0+144, ir_state+312*0+144 );
    _CONVOLVE_24p( ir_coeff+312*0+168, ir_state+312*0+168 );
    _CONVOLVE_24p( ir_coeff+312*0+192, ir_state+312*0+192 );
    _CONVOLVE_24p( ir_coeff+312*0+216, ir_state+312*0+216 );
    _CONVOLVE_24p( ir_coeff+312*0+240, ir_state+312*0+240 );
    _CONVOLVE_24p( ir_coeff+312*0+264, ir_state+312*0+264 );
    _CONVOLVE_24p( ir_coeff+312*0+312, ir_state+312*0+312 );
    //_CONVOLVE_24p( ir_coeff+312*0+336, ir_state+312*0+336 );
    //_CONVOLVE_24p( ir_coeff+312*0+360, ir_state+312*0+360 );
    samples[1] = ah; samples[2] = al;

    s0 = samples[4]; ah = 0, al = 1<<(QQ-1);
    _CONVOLVE_24p( ir_coeff+312*5+  0, ir_state+312*5+  0 );
    _CONVOLVE_24p( ir_coeff+312*5+ 24, ir_state+312*5+ 24 );
    _CONVOLVE_24p( ir_coeff+312*5+ 48, ir_state+312*5+ 48 );
    _CONVOLVE_24p( ir_coeff+312*5+ 72, ir_state+312*5+ 72 );
    _CONVOLVE_24p( ir_coeff+312*5+ 96, ir_state+312*5+ 96 );
    _CONVOLVE_24p( ir_coeff+312*5+120, ir_state+312*5+120 );
    _CONVOLVE_24p( ir_coeff+312*5+144, ir_state+312*5+144 );
    _CONVOLVE_24p( ir_coeff+312*5+168, ir_state+312*5+168 );
    _CONVOLVE_24p( ir_coeff+312*5+192, ir_state+312*5+192 );
    _CONVOLVE_24p( ir_coeff+312*5+216, ir_state+312*5+216 );
    _CONVOLVE_24p( ir_coeff+312*5+240, ir_state+312*5+240 );
    _CONVOLVE_24p( ir_coeff+312*5+264, ir_state+312*5+264 );
    _CONVOLVE_24p( ir_coeff+312*5+312, ir_state+312*5+312 );
    //_CONVOLVE_24p( ir_coeff+312*5+336, ir_state+312*5+336 );
    //_CONVOLVE_24p( ir_coeff+312*5+360, ir_state+312*5+360 );
    samples[4] = ah; samples[5] = al;
}

void app_thread3( int samples[32], const int property[6] )
{
    int ah; unsigned al = 0, s0, b0, b1, s1, s2, s3;

    s0 = samples[0]; ah = samples[1]; al = samples[2];
    _CONVOLVE_24p( ir_coeff+312*1+  0, ir_state+312*1+  0 );
    _CONVOLVE_24p( ir_coeff+312*1+ 24, ir_state+312*1+ 24 );
    _CONVOLVE_24p( ir_coeff+312*1+ 48, ir_state+312*1+ 48 );
    _CONVOLVE_24p( ir_coeff+312*1+ 72, ir_state+312*1+ 72 );
    _CONVOLVE_24p( ir_coeff+312*1+ 96, ir_state+312*1+ 96 );
    _CONVOLVE_24p( ir_coeff+312*1+120, ir_state+312*1+120 );
    _CONVOLVE_24p( ir_coeff+312*1+144, ir_state+312*1+144 );
    _CONVOLVE_24p( ir_coeff+312*1+168, ir_state+312*1+168 );
    _CONVOLVE_24p( ir_coeff+312*1+192, ir_state+312*1+192 );
    _CONVOLVE_24p( ir_coeff+312*1+216, ir_state+312*1+216 );
    _CONVOLVE_24p( ir_coeff+312*1+240, ir_state+312*1+240 );
    _CONVOLVE_24p( ir_coeff+312*1+264, ir_state+312*1+264 );
    _CONVOLVE_24p( ir_coeff+312*1+312, ir_state+312*1+312 );
    //_CONVOLVE_24p( ir_coeff+312*1+336, ir_state+312*1+336 );
    //_CONVOLVE_24p( ir_coeff+312*1+360, ir_state+312*1+360 );
    samples[1] = ah; samples[2] = al;

    s0 = samples[4]; ah = samples[5]; al = samples[6];
    _CONVOLVE_24p( ir_coeff+312*6+  0, ir_state+312*6+  0 );
    _CONVOLVE_24p( ir_coeff+312*6+ 24, ir_state+312*6+ 24 );
    _CONVOLVE_24p( ir_coeff+312*6+ 48, ir_state+312*6+ 48 );
    _CONVOLVE_24p( ir_coeff+312*6+ 72, ir_state+312*6+ 72 );
    _CONVOLVE_24p( ir_coeff+312*6+ 96, ir_state+312*6+ 96 );
    _CONVOLVE_24p( ir_coeff+312*6+120, ir_state+312*6+120 );
    _CONVOLVE_24p( ir_coeff+312*6+144, ir_state+312*6+144 );
    _CONVOLVE_24p( ir_coeff+312*6+168, ir_state+312*6+168 );
    _CONVOLVE_24p( ir_coeff+312*6+192, ir_state+312*6+192 );
    _CONVOLVE_24p( ir_coeff+312*6+216, ir_state+312*6+216 );
    _CONVOLVE_24p( ir_coeff+312*6+240, ir_state+312*6+240 );
    _CONVOLVE_24p( ir_coeff+312*6+264, ir_state+312*6+264 );
    _CONVOLVE_24p( ir_coeff+312*6+312, ir_state+312*6+312 );
    //_CONVOLVE_24p( ir_coeff+312*6+336, ir_state+312*6+336 );
    //_CONVOLVE_24p( ir_coeff+312*6+360, ir_state+312*6+360 );
    samples[5] = ah; samples[6] = al;
}

void app_thread4( int samples[32], const int property[6] )
{
    int ah; unsigned al = 0, s0, b0, b1, s1, s2, s3;

    s0 = samples[0]; ah = samples[1]; al = samples[2];
    _CONVOLVE_24p( ir_coeff+312*2+  0, ir_state+312*2+  0 );
    _CONVOLVE_24p( ir_coeff+312*2+ 24, ir_state+312*2+ 24 );
    _CONVOLVE_24p( ir_coeff+312*2+ 48, ir_state+312*2+ 48 );
    _CONVOLVE_24p( ir_coeff+312*2+ 72, ir_state+312*2+ 72 );
    _CONVOLVE_24p( ir_coeff+312*2+ 96, ir_state+312*2+ 96 );
    _CONVOLVE_24p( ir_coeff+312*2+120, ir_state+312*2+120 );
    _CONVOLVE_24p( ir_coeff+312*2+144, ir_state+312*2+144 );
    _CONVOLVE_24p( ir_coeff+312*2+168, ir_state+312*2+168 );
    _CONVOLVE_24p( ir_coeff+312*2+192, ir_state+312*2+192 );
    _CONVOLVE_24p( ir_coeff+312*2+216, ir_state+312*2+216 );
    _CONVOLVE_24p( ir_coeff+312*2+240, ir_state+312*2+240 );
    _CONVOLVE_24p( ir_coeff+312*2+264, ir_state+312*2+264 );
    _CONVOLVE_24p( ir_coeff+312*2+312, ir_state+312*2+312 );
    //_CONVOLVE_24p( ir_coeff+312*2+336, ir_state+312*2+336 );
    //_CONVOLVE_24p( ir_coeff+312*2+360, ir_state+312*2+360 );
    samples[1] = ah; samples[2] = al;

    s0 = samples[4]; ah = samples[5]; al = samples[6];
    _CONVOLVE_24p( ir_coeff+312*7+  0, ir_state+312*7+  0 );
    _CONVOLVE_24p( ir_coeff+312*7+ 24, ir_state+312*7+ 24 );
    _CONVOLVE_24p( ir_coeff+312*7+ 48, ir_state+312*7+ 48 );
    _CONVOLVE_24p( ir_coeff+312*7+ 72, ir_state+312*7+ 72 );
    _CONVOLVE_24p( ir_coeff+312*7+ 96, ir_state+312*7+ 96 );
    _CONVOLVE_24p( ir_coeff+312*7+120, ir_state+312*7+120 );
    _CONVOLVE_24p( ir_coeff+312*7+144, ir_state+312*7+144 );
    _CONVOLVE_24p( ir_coeff+312*7+168, ir_state+312*7+168 );
    _CONVOLVE_24p( ir_coeff+312*7+192, ir_state+312*7+192 );
    _CONVOLVE_24p( ir_coeff+312*7+216, ir_state+312*7+216 );
    _CONVOLVE_24p( ir_coeff+312*7+240, ir_state+312*7+240 );
    _CONVOLVE_24p( ir_coeff+312*7+264, ir_state+312*7+264 );
    _CONVOLVE_24p( ir_coeff+312*7+312, ir_state+312*7+312 );
    //_CONVOLVE_24p( ir_coeff+312*7+336, ir_state+312*7+336 );
    //_CONVOLVE_24p( ir_coeff+312*7+360, ir_state+312*7+360 );
    samples[5] = ah; samples[6] = al;
}

void app_thread5( int samples[32], const int property[6] )
{
    int ah; unsigned al = 0, s0, b0, b1, s1, s2, s3;

    s0 = samples[0]; ah = samples[1]; al = samples[2];
    _CONVOLVE_24p( ir_coeff+312*3+  0, ir_state+312*3+  0 );
    _CONVOLVE_24p( ir_coeff+312*3+ 24, ir_state+312*3+ 24 );
    _CONVOLVE_24p( ir_coeff+312*3+ 48, ir_state+312*3+ 48 );
    _CONVOLVE_24p( ir_coeff+312*3+ 72, ir_state+312*3+ 72 );
    _CONVOLVE_24p( ir_coeff+312*3+ 96, ir_state+312*3+ 96 );
    _CONVOLVE_24p( ir_coeff+312*3+120, ir_state+312*3+120 );
    _CONVOLVE_24p( ir_coeff+312*3+144, ir_state+312*3+144 );
    _CONVOLVE_24p( ir_coeff+312*3+168, ir_state+312*3+168 );
    _CONVOLVE_24p( ir_coeff+312*3+192, ir_state+312*3+192 );
    _CONVOLVE_24p( ir_coeff+312*3+216, ir_state+312*3+216 );
    _CONVOLVE_24p( ir_coeff+312*3+240, ir_state+312*3+240 );
    _CONVOLVE_24p( ir_coeff+312*3+264, ir_state+312*3+264 );
    _CONVOLVE_24p( ir_coeff+312*3+312, ir_state+312*3+312 );
    //_CONVOLVE_24p( ir_coeff+312*3+336, ir_state+312*3+336 );
    //_CONVOLVE_24p( ir_coeff+312*3+360, ir_state+312*3+360 );
    asm("lextract %0,%1,%2,%3,32":"=r"(samples[0]):"r"(ah),"r"(al),"r"(QQ));

    s0 = samples[4]; ah = samples[5]; al = samples[6];
    _CONVOLVE_24p( ir_coeff+312*8+  0, ir_state+312*8+  0 );
    _CONVOLVE_24p( ir_coeff+312*8+ 24, ir_state+312*8+ 24 );
    _CONVOLVE_24p( ir_coeff+312*8+ 48, ir_state+312*8+ 48 );
    _CONVOLVE_24p( ir_coeff+312*8+ 72, ir_state+312*8+ 72 );
    _CONVOLVE_24p( ir_coeff+312*8+ 96, ir_state+312*8+ 96 );
    _CONVOLVE_24p( ir_coeff+312*8+120, ir_state+312*8+120 );
    _CONVOLVE_24p( ir_coeff+312*8+144, ir_state+312*8+144 );
    _CONVOLVE_24p( ir_coeff+312*8+168, ir_state+312*8+168 );
    _CONVOLVE_24p( ir_coeff+312*8+192, ir_state+312*8+192 );
    _CONVOLVE_24p( ir_coeff+312*8+216, ir_state+312*8+216 );
    _CONVOLVE_24p( ir_coeff+312*8+240, ir_state+312*8+240 );
    _CONVOLVE_24p( ir_coeff+312*8+264, ir_state+312*8+264 );
    _CONVOLVE_24p( ir_coeff+312*8+312, ir_state+312*8+312 );
    //_CONVOLVE_24p( ir_coeff+312*8+336, ir_state+312*8+336 );
    //_CONVOLVE_24p( ir_coeff+312*8+360, ir_state+312*8+360 );
    asm("lextract %0,%1,%2,%3,32":"=r"(samples[4]):"r"(ah),"r"(al),"r"(QQ));
}
