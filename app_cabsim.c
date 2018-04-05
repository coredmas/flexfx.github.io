// To build, FLASH, etc using XMOS tools version 14.3 (download from www.xmos.com) ...
//
// Download kit:  git clone https://github.com/markseel/flexfx_kit.git
// Build source:  xcc -report -O3 -lquadflash flexfx.xn flexfx.a app_cabsim.c -o app_cabsim.xe
// Create binary: xflash --noinq --no-compression --factory-version 14.3 --upgrade 1 app_cabsim.xe -o app_cabsim.bin
// Burn via XTAG: xflash --no-compression --factory-version 14.3 --upgrade 1 app_cabsim.xe
// Burn via USB:  python flexfx.py 0 app_cabsim.bin  (Note: FlexFX must be the only USB device attached)
//       .. or :  Run 'flexfx.html' in Google Chrome, press "Upload Firmware", select 'app_cabsim.bin'

// FQ converts Q28 fixed-point to floating point, QF converts floating-point to Q28 fixed-point.

#define FQ(hh) (((hh)<0.0)?((int)((double)(1u<<28)*(hh)-0.5)):((int)(((double)(1u<<28)-1)*(hh)+0.5)))
#define QF(xx) (((int)(xx)<0)?((double)(int)(xx))/(1u<<28):((double)(xx))/((1u<<28)-1))

typedef unsigned char byte;
typedef unsigned int  bool;

// Flash memory functions for data persistance (only use these in the 'app_control' thread!).
//
// Each page consists of 3268 5-byte properties (total of 65530 bytes). There are 16
// property pages in FLASH and one property page in RAM used as a scratch buffer. Pages can
// be loaded and saved from/to FLASH. All USB MIDI property flow (except for properties with
// ID >= 0x8000 used for FLASH/RAM control) results in updates the RAM scratch buffer.

extern void page_load ( int page_num );      // Load 64Kbyte page from FLASH to RAM
extern void page_save ( int page_num );      // Save 64Kbyte page from RAM to FLASH
extern void page_read ( int property[6] );   // Read one from RAM (prop index = property[0])
extern void page_write( const int prop[6] ); // Write one property to RAM at index=prop[0]

// Port I/O functions.

extern void port_write( int pin_num, bool value ); // Write binary value to IO port
extern bool port_read ( int pin_num );             // Read binary value from IO port

// I2C functions for peripheral control (only use these in the 'app_control' thread!).

extern void i2c_start( int speed );  // Set bit rate, assert an I2C start condition.
extern byte i2c_write( byte value ); // Write 8-bit data value.
extern byte i2c_read ( void );       // Read 8-bit data value.
extern void i2c_ack  ( byte ack );   // Assert the ACK/NACK bit after a read.
extern void i2c_stop ( void );       // Assert an I2C stop condition.

// DSP MATH primitives
//
//MAC performs 32x32 multiply and 64-bit accumulation, SAT saturates a 64-bit
// result, EXT converts a 64-bit value to a 32-bit value (extract 32 from 64), LD2/ST2 loads/stores
// two 32-values from/to 64-bit aligned 32-bit data arrays at address PP.
//
// AH (high) and AL (low) form the 64-bit signed accumulator
// XX, YY, and AA are 32-bit Q28 fixed point values
// PP is a 64-bit aligned pointer to two 32-bit Q28 values

#define DSP_MUL( ah, al, xx, yy ) asm volatile("maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(xx),"r"(yy),"0"(0),"1"(1<<(28-1)) );
#define DSP_MAC( ah, al, xx, yy ) asm volatile("maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(xx),"r"(yy),"0"(ah),"1"(al) );
#define DSP_DIV( qq,rr,ah,al,xx ) asm volatile("ldivu %0,%1,%2,%3,%4":"=r"(qq):"r"(rr),"r"(ah),"r"(al),"r"(xx));
#define DSP_SAT( ah, al )         asm volatile("lsats %0,%1,%2":"=r"(ah),"=r"(al):"r"(28),"0"(ah),"1"(al));
#define DSP_EXT( xx, ah, al )     asm volatile("lextract %0,%1,%2,%3,32":"=r"(xx):"r"(ah),"r"(al),"r"(28));
#define DSP_LD2( pp, xx, yy )     asm volatile("ldd %0,%1,%2[0]":"=r"(xx),"=r"(yy):"r"(pp));
#define DSP_ST2( pp, xx, yy )     asm volatile("std %0,%1,%2[0]"::"r"(xx), "r"(yy),"r"(pp));

inline int dsp_multiply( int xx, int yy ) // RR = XX * YY
{
    int ah = 0; unsigned al = 1<<(28-1);
    asm("maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(xx),"r"(yy),"0"(ah),"1"(al) );
    asm("lextract %0,%1,%2,%3,32":"=r"(ah):"r"(ah),"r"(al),"r"(28));
    return ah;
}

// Sine lookup tables of length 2^10, 2^12, and 2^14.  Values are in Q28 format.
extern int dsp_sine_10[ 1024], dsp_atan_10[ 1024], dsp_tanh_10[ 1024], dsp_nexp_10[ 1024];
extern int dsp_sine_12[ 4096], dsp_atan_12[ 4096], dsp_tanh_12[ 4096], dsp_nexp_12[ 4096];
extern int dsp_sine_14[16384], dsp_atan_14[16384], dsp_tanh_14[16384], dsp_nexp_14[16384];

// Math and filter functions.
//
// XX, CC, SS, Yn, MM, and AA are 32-bit fixed point samples/data in Q28 format
// DD is the distance (0<=DD<1) between the first two points for interpolation
// KK is a time constant, Q28 format
// Yn are the data points to be interpolated
// NN is FIR filter tap-count for 'fir', 'upsample', 'dnsample' and 'convolve' functions
// NN is IIR filter order or or IIR filter count for cascaded IIR's
// NN is number of samples in XX for scalar and vector math functions
// CC is array of 32-bit filter coefficients - length is 'nn' for FIR, nn * 5 for IIR
// SS is array of 32-bit filter state - length is 'nn' for FIR, nn * 4 for IIR, 3 for DCBLOCK
// RR is the up-sampling/interpolation or down-sampling/decimation ratio
// AH (high) and AL (low) form the 64-bit signed accumulator

int  math_random ( int  gg, int seed );              // Random number, gg = previous value
int  math_sqr_x  ( int xx );                         // r = xx^0.5
int  math_min_X  ( const int* xx, int nn );          // r = min(X[0:N-1])
int  math_max_X  ( const int* xx, int nn );          // r = max(X[0:N-1])
int  math_avg_X  ( const int* xx, int nn );          // r = mean(X[0:N-1])
int  math_rms_X  ( const int* xx, int nn );          // r = sum(X[0:N-1]*X[0:N-1]) ^ 0.5
void math_sum_X  ( const int* xx, int nn, int* ah, unsigned* al ); // r = sum(X[0:N-1])
void math_asm_X  ( const int* xx, int nn, int* ah, unsigned* al ); // r = sum(abs(X[0:N-1))
void math_pwr_X  ( const int* xx, int nn, int* ah, unsigned* al ); // r = sum(X[0:N-1]*X[0:N-1])
void math_abs_X  ( int* xx, int nn );                // X[0:N-1] = abs(X[0:N-1])
void math_sqr_X  ( int* xx, int nn );                // X[0:N-1] = X[0:N-1]^0.5
void math_mac_X1z( int* xx, int        zz, int nn ); // X[0:N-1] = X[0:N-1] + zz
void math_mac_X1Z( int* xx, const int* zz, int nn ); // X[0:N-1] = X[0:N-1] + zz[0:N-1]
void math_mac_Xy0( int* xx, int        yy, int nn ); // X[0:N-1] = X[0:N-1] * y
void math_mac_XY0( int* xx, const int* yy, int nn ); // X[0:N-1] = X[0:N-1] * Y[0:N-1]
void math_mac_Xyz( int* xx, int        yy, int zz,        int nn ); // X[] = X[] * y + z
void math_mac_XyZ( int* xx, int        yy, const int* zz, int nn ); // X[] = X[] * y + Z[]
void math_mac_XYz( int* xx, const int* yy, int        zz, int nn ); // X[] = X[] * Y[] + z
void math_mac_XYZ( int* xx, const int* yy, const int* zz, int nn ); // X[] = X[] * Y[] + Z[]

// Math and filter functions.
//
// XX, CC, SS, Yn, MM, and AA are 32-bit fixed point samples/data in Q28 format
// DD is the distance (0<=DD<1) between the first two points for interpolation
// KK is a time constant, Q28 format
// Yn are the data points to be interpolated
// NN is FIR filter tap-count for 'fir', 'upsample', 'dnsample' and 'convolve' functions
// NN is IIR filter order or or IIR filter count for cascaded IIR's
// NN is number of samples in XX for scalar and vector math functions
// CC is array of 32-bit filter coefficients - length is 'nn' for FIR, nn * 5 for IIR
// SS is array of 32-bit filter state - length is 'nn' for FIR, nn * 4 for IIR, 3 for DCBLOCK
// RR is the up-sampling/interpolation or down-sampling/decimation ratio
// AH (high) and AL (low) form the 64-bit signed accumulator

int  dsp_interp  ( int  dd, int y1, int y2 );         // 1st order (linear) interpolation
int  dsp_lagrange( int  dd, int y1, int y2, int y3 ); // 2nd order (Lagrange) interpolation
int  dsp_blend   ( int  xx, int yy, int mm );         // 0.0 (100% xx) <= mm <= 1.0 (100% yy)
int  dsp_dcblock ( int  xx, int kk, int* ss ); // DC blocker
int  dsp_envelope( int  xx, int kk, int* ss ); // Envelope detector,vo=vi*(1–e^(–t/RC)),kk=2*RC/Fs
int  dsp_convolve( int  xx, const int* cc, int* ss, int* ah, int* al ); // 240 tap FIR convolution
int  dsp_iir     ( int  xx, const int* cc, int* ss, int nn ); // nn Cascaded bi-quad IIR filters
int  dsp_fir     ( int  xx, const int* cc, int* ss, int nn ); // FIR filter of nn taps
void dsp_fir_up  ( int* xx, const int* cc, int* ss, int nn, int rr ); // FIR up-sampling/interpolation
void dsp_fir_dn  ( int* xx, const int* cc, int* ss, int nn, int rr ); // FIR dn-sampling/decimation
void dsp_cic_up  ( int* xx, const int* cc, int* ss, int nn, int rr ); // CIC up-sampling/interpolation
void dsp_cic_dn  ( int* xx, const int* cc, int* ss, int nn, int rr ); // CIC dn-sampling/decimation

// Biquad filter coefficient calculation functions (only use these in the 'app_control' thread!).
//
// CC is an array of floating point filter coefficients.
// FF, QQ, GG are the filter frequency, filter Q-factor, and filter gain.
// For low/high pass filters set filter Q (qq) to zero to create a single-pole 6db/octave filter.

void calc_notch    ( int cc[5], double ff, double qq );
void calc_lowpass  ( int cc[5], double ff, double qq );
void calc_highpass ( int cc[5], double ff, double qq );
void calc_allpass  ( int cc[5], double ff, double qq );
void calc_bandpass ( int cc[5], double ff1, double ff2 );
void calc_peaking  ( int cc[5], double ff, double qq, double gg );
void calc_lowshelf ( int cc[5], double ff, double qq, double gg );
void calc_highshelf( int cc[5], double ff, double qq, double gg );

// Using prebuilt FlexFX effects? If so #include one of the following ...
// #include "efc_cabsim.h", "efc_preamp.h", "efc_chorus.h", "efc_reverb.h", "efc_graphiceq.h"

// Device configuration ...

const char* product_name_string   = "FlexFX Example";   // Your product name
const char* usb_audio_output_name = "FlexFX Audio Out"; // USB audio output name
const char* usb_audio_input_name  = "FlexFX Audio In";  // USB audio input name
const char* usb_midi_output_name  = "FlexFX MIDI Out";  // USB MIDI output name
const char* usb_midi_input_name   = "FlexFX MIDI In";   // USB MIDI input name

const int audio_sample_rate     = 48000; // Audio sampling frequency
const int usb_output_chan_count = 2;     // 2 USB audio class 2.0 output channels
const int usb_input_chan_count  = 2;     // 2 USB audio class 2.0 input channels
const int i2s_channel_count     = 2;     // ADC/DAC channels per SDIN/SDOUT wire

const int i2s_sync_word[8] = { 0xFFFFFFFF,0x00000000,0,0,0,0,0,0 }; // I2S WCLK values per slot

void copy_prop( int dst[6], const int src[6] )
{
    dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2]; dst[3]=src[3]; dst[4]=src[4]; dst[5]=src[5];
}

void app_control( const int rcv_prop[6], int usb_prop[6], int dsp_prop[6] )
{
    // Pass cabsim IR data to the DSP threads if usb and dsp send properties are available for use.
    //if( dsp_prop[0] == 0x9000 ) exit(0);
    copy_prop( dsp_prop, rcv_prop ); // Send to DSP threads.
}

void app_mixer( const int usb_output[32], int usb_input[32],
                const int i2s_output[32], int i2s_input[32],
                const int dsp_output[32], int dsp_input[32], const int property[6] )
{
    // Convert the two ADC inputs into a single pseudo-differential mono input (mono = L - R).
    int guitar_in = i2s_output[0] - i2s_output[1];
    // Route instrument input to the left USB input and to the DSP input.
    dsp_input[0] = (usb_input[0] = guitar_in) / 8; // DSP samples need to be Q28 formatted.
    // Route DSP result to the right USB input and the audio DAC.
    usb_input[1] = i2s_input[0] = i2s_input[1] = dsp_output[0] * 8; // Q28 to Q31
}

int ir_coeff[2400], ir_state[2400]; // DSP data *must* be non-static global!

void app_thread1( int samples[32], const int property[6] )
{
    static int first = 1;
    if( first ) { first = 0; ir_coeff[0] = ir_coeff[1200] = FQ(+1.0); }
    // Check for properties containing new cabsim IR data, save new data to RAM
    if( (property[0] & 0xF000) == 0x9000 )
    {
    	int offset = 5 * (property[0] & 0x0FFF);
    	if( offset <= 2400-5 ) {
			ir_coeff[offset+0] = property[1] / 32; ir_coeff[offset+1] = property[2] / 32;
			ir_coeff[offset+2] = property[3] / 32; ir_coeff[offset+3] = property[4] / 32;
			ir_coeff[offset+4] = property[5] / 32;
		}
    }
    samples[2] = 0; samples[3] = 1 << 31; // Initial 64-bit Q1.63 accumulator value
    samples[4] = 0; samples[5] = 1 << 31; // Initial 64-bit Q1.63 accumulator value
    // Perform 240-sample convolution (1st 240 of 1220 total) of sample with IR data
    samples[0] = dsp_convolve( samples[0], ir_coeff+240*0, ir_state+240*0, samples+2,samples+3 );
    samples[1] = dsp_convolve( samples[1], ir_coeff+240*5, ir_state+240*5, samples+4,samples+5 );
}

void app_thread2( int samples[32], const int property[6] )
{
    // Perform 240-sample convolution (2nd 240 of 1220 total) of sample with IR data
    samples[0] = dsp_convolve( samples[0], ir_coeff+240*1, ir_state+240*1, samples+2,samples+3 );
    samples[1] = dsp_convolve( samples[1], ir_coeff+240*6, ir_state+240*6, samples+4,samples+5 );
}

void app_thread3( int samples[32], const int property[6] )
{
    // Perform 240-sample convolution (3rd 240 of 1220 total) of sample with IR data
    samples[0] = dsp_convolve( samples[0], ir_coeff+240*2, ir_state+240*2, samples+2,samples+3 );
    samples[1] = dsp_convolve( samples[1], ir_coeff+240*7, ir_state+240*7, samples+4,samples+5 );
}

void app_thread4( int samples[32], const int property[6] )
{
    // Perform 240-sample convolution (4th 240 of 1220 total) of sample with IR data
    samples[0] = dsp_convolve( samples[0], ir_coeff+240*3, ir_state+240*3, samples+2,samples+3 );
    samples[1] = dsp_convolve( samples[1], ir_coeff+240*8, ir_state+240*8, samples+4,samples+5 );
}

void app_thread5( int samples[32], const int property[6] )
{
    static bool muted = 0;
    // Check IR property -- Mute at start of new IR loading, un-mute when done.
    if( property[0] == 0x9000 ) muted = 1;
    if( property[0] == 0x9000 + 480 ) muted = 0;
    // Perform 240-sample convolution (5th and last 240 of 1220 total) of sample with IR data
    samples[0] = dsp_convolve( samples[0], ir_coeff+240*4, ir_state+240*4, samples+2,samples+3 );
    samples[1] = dsp_convolve( samples[1], ir_coeff+240*9, ir_state+240*9, samples+4,samples+5 );
    // Extract 32-bit Q28 from 64-bit Q63 and then apply mute/un-mute based on IR loading activity.
    DSP_EXT( samples[0], samples[2], samples[3] );
    DSP_EXT( samples[1], samples[4], samples[5] );
    samples[0] = muted ? 0 : samples[0];
    samples[1] = muted ? 0 : samples[1];
}

const char controller_script[] =
""\
"function flexfx_create( key )"\
"{"\
"	var x = \"\";"\
"	x += \"<p>\";"\
"	x += \"This FlexFX device does not have effects firmware loaded into it. Use the \";"\
"	x += \"'LOAD FIRMWARE' button to select a firmware image to load into this device.\";"\
"	x += \"</p>\";"\
"	return x;"\
"}"\
""\
"function flexfx_initialize( key )"\
"{"\
"	return _on_property_received;"\
"}"\
""\
"function _on_property_received( property )"\
"{"\
"}"\
"";
