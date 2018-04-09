FlexFX&trade; Kit - Programming Interface
-----------------------------------------

The application programmer only needs to add control and audip processing code to create a complete application.  The code below is a complete application (that does nothing).  Just add code to the 'app\_control', 'app\_mixer', and 'app\_thread' functions.  The complete FlexFX programming interface is show below:


```C
#include <platform.h>

typedef unsigned char byte;
typedef unsigned int  bool;

// Flash memory functions for data persistance (do not use these in real-time DSP threads).

extern void flash_open ( void );                   // Open the flash device
extern void flash_seek ( int page_number );        // Set the page for the next write/read
extern void flash_read ( byte buffer[256] );       // Read next flash page, move to the next page
extern void flash_write( const byte buffer[256] ); // Write next flash page, move to the next page
extern void flash_close( void );                   // Close the flash device

// Port and pin I/O functions. DAC/ADC port reads/writes will disable I2S/TDM!

extern void io_write( int pin_num, bool value ); // Write binary value to IO pin
extern bool io_read ( int pin_num );             // Read binary value from IO pin

extern void port_write( byte value ); // Write 4-bit nibble to DAC port (dac0-dac3 pins)
extern byte port_read ( void );       // Read 4-bit nibble from ADC port (adc0-adc3 pins)

// I2C functions for peripheral control (do not use these in real-time DSP threads).

extern void i2c_start( int speed );    // Set bit rate, assert an I2C start condition.
extern byte i2c_write( byte value );   // Write 8-bit data value.
extern byte i2c_read ( void );         // Read 8-bit data value.
extern void i2c_ack  ( byte ack );     // Assert the ACK/NACK bit after a read.
extern void i2c_stop ( void );         // Assert an I2C stop condition.

// FQ converts Q28 fixed-point to floating point
// QF converts floating-point to Q28 fixed-point

#define FQ(hh) (((hh)<0.0)?((int)((double)(1u<<28)*(hh)-0.5)):((int)(((double)(1u<<28)-1)*(hh)+0.5)))
#define QF(xx) (((int)(xx)<0)?((double)(int)(xx))/(1u<<28):((double)(xx))/((1u<<28)-1))

// MAC performs 32x32 multiply and 64-bit accumulation, SAT saturates a 64-bit result, EXT converts
// a 64-bit result to a 32-bit value (extract 32 from 64), LD2/ST2 loads/stores two 32-values
// from/to 64-bit aligned 32-bit data arrays at address PP. All 32-bit fixed-point values are Q28
// fixed-point formatted.
//
// AH (high) and AL (low) form the 64-bit signed accumulator
// XX, YY, and AA are 32-bit Q28 fixed point values
// PP is a 64-bit aligned pointer to two 32-bit Q28 values

#define DSP_MAC( ah, al, xx, yy ) asm("maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(xx),"r"(yy),"0"(ah),"1"(al) );
#define DSP_SAT( ah, al )         asm("lsats %0,%1,%2":"=r"(ah),"=r"(al):"r"(28),"0"(ah),"1"(al));
#define DSP_EXT( xx, ah, al )     asm("lextract %0,%1,%2,%3,32":"=r"(xx):"r"(ah),"r"(al),"r"(28));
#define DSP_LD2( pp, xx, yy )     asm("ldd %0,%1,%2[0]":"=r"(xx),"=r"(yy):"r"(pp));
#define DSP_ST2( pp, xx, yy )     asm("std %0,%1,%2[0]"::"r"(xx), "r"(yy),"r"(pp));

inline int dsp_multiply( int xx, int yy ) // RR = XX * YY
{
    int ah = 0; unsigned al = 1<<(28-1);
    asm("maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(xx),"r"(yy),"0"(ah),"1"(al) );
    asm("lextract %0,%1,%2,%3,32":"=r"(ah):"r"(ah),"r"(al),"r"(28));
    return ah;
}

// Lookup tables for sine, various sigmoid functions, logarithm, etc. Sigmoid functions have slope
// of 8 at x=0, a minimum value of 0.0 at x=0, and maximim value approaching 1.0 as x approaches
// 1.0. Tables indexed by II are NN in length and consist of 32-bit Q28 fixed point values.
// All table values follow this rule: 0.0 <= xx < 1.0 where ii = int(xx*nn) --> 0.0 <= yy < 1.0.
// SINE(xx) = 0.5+sin(2pi*xx)/2, ATAN(xx) = c*atan(xx*8), TANH(xx) = tanh(xx*8),
// NEXP(xx) = 1-e^(-xx*8), RLOG(xx) = -log10(1.0-xx)/8,

extern int dsp_sine_08[256], dsp_sine_10[1024], dsp_sine_12[4096], dsp_sine_14[16384];
extern int dsp_atan_08[256], dsp_atan_10[1024], dsp_atan_12[4096], dsp_atan_14[16384];
extern int dsp_tanh_08[256], dsp_tanh_10[1024], dsp_tanh_12[4096], dsp_tanh_14[16384];
extern int dsp_nexp_08[256], dsp_nexp_10[1024], dsp_nexp_12[4096], dsp_nexp_14[16384];
extern int dsp_rlog_08[256], dsp_rlog_10[1024], dsp_rlog_12[4096], dsp_rlog_14[16384];

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

int  dsp_random  ( int  gg, int seed );               // Random number, gg = previous value
int  dsp_blend   ( int  xx, int yy, int mm );         // 0.0 (100% xx) <= mm <= 1.0 (100% yy)
int  dsp_interp  ( int  dd, int y1, int y2 );         // 1st order (linear) interpolation
int  dsp_lagrange( int  dd, int y1, int y2, int y3 ); // 2nd order (Lagrange) interpolation

int  dsp_dcblock ( int  xx, int* ss, int kk ); // DC blocker
int  dsp_envelope( int  xx, int* ss, int kk ); // Envelope detector,vo=vi*(1–e^(–t/RC)),kk=2*RC/Fs

int  dsp_sum     ( int* xx, int nn );          // sum(xx[0:N-1])
int  dsp_abs_sum ( int* xx, int nn );          // sum(abs(xx[0:N-1]))
int  dsp_power   ( int* xx, int nn );          // xx[0:N-1]^2
int  dsp_sca_sqrt( int  xx );                  // xx ^ 0.5
void dsp_vec_sqrt( int* xx, int nn );          // xx[0:N-1] = xx[0:N-1] ^ 0.5
void dsp_abs     ( int* xx, int nn );          // xx[0:N-1] = abs(xx[0:N-1])
void dsp_square  ( int* xx, int nn );          // xx[0:N-1] = xx[0:N-1]^2
void dsp_sca_add ( int* xx, int  kk, int nn ); // xx[0:N-1] += kk
void dsp_sca_mult( int* xx, int  kk, int nn ); // xx[0:N-1] *= kk
void dsp_vec_add ( int* xx, int* yy, int nn ); // xx[0:N-1] = xx[0:N-1] + yy[0:N-1]
void dsp_vec_mult( int* xx, int* yy, int nn ); // xx[0:N-1] = xx[0:N-1] * yy[0:N-1]

int  dsp_convolve( int  xx, const int* cc, int* ss, int* ah, int* al ); // 240 tap FIR convolution
int  dsp_iir     ( int  xx, const int* cc, int* ss, int nn ); // nn Cascaded bi-quad IIR filters
int  dsp_fir     ( int  xx, const int* cc, int* ss, int nn ); // FIR filter of nn taps
void dsp_fir_up  ( int* xx, const int* cc, int* ss, int nn, int rr ); // FIR up-sampling/interpolation
void dsp_fir_dn  ( int* xx, const int* cc, int* ss, int nn, int rr ); // FIR dn-sampling/decimation
void dsp_cic_up  ( int* xx, const int* cc, int* ss, int nn, int rr ); // CIC up-sampling/interpolation
void dsp_cic_dn  ( int* xx, const int* cc, int* ss, int nn, int rr ); // CIC dn-sampling/decimation

// Biquad filter coefficient calculation functions (do not use these in real-time DSP threads).
//
// CC is an array of floating point filter coefficients
// FF, QQ, GG are the filter frequency, filter Q-factor, and filter gain
// For low/high pass filters set QQ to zero to create a single-pole 6db/octave filter

void make_notch    ( int cc[5], double ff, double qq );
void make_lowpass  ( int cc[5], double ff, double qq );
void make_highpass ( int cc[5], double ff, double qq );
void make_allpass  ( int cc[5], double ff, double qq );
void make_bandpass ( int cc[5], double ff1, double ff2 );
void make_peaking  ( int cc[5], double ff, double qq, double gg );
void make_lowshelf ( int cc[5], double ff, double qq, double gg );
void make_highshelf( int cc[5], double ff, double qq, double gg );

// Functions and variables to be implemented by the application ...

const char* product_name_string   = "Example";   // Your product name
const char* usb_audio_output_name = "Audio Out"; // Your USB audio output name
const char* usb_audio_input_name  = "Audio In";  // Your USB audio input name
const char* usb_midi_output_name  = "MIDI Out";  // Your USB MIDI output name
const char* usb_midi_input_name   = "MIDI In";   // Your USB MIDI input name

const int audio_sample_rate     = 48000; // Audio sampling frequency (48K,96K,192K,or 384K)
const int usb_output_chan_count = 2;     // 2 USB audio output channels (32 max)
const int usb_input_chan_count  = 2;     // 2 USB audio input channels (32 max)
const int i2s_channel_count     = 2;     // 2,4,or8 I2S channels per SDIN/SDOUT wire

const int i2s_sync_word[8] = { 0xFFFFFFFF,0x00000000,0,0,0,0,0,0 }; // I2S WCLK words for each slot

// The control task is called at a rate of 1000 Hz and should be used to implement audio CODEC
// initialization/control, pot and switch sensing via I2C ADC's, handling of properties from USB
// MIDI, and generation of properties to be consumed by the USB MIDI host and by the DSP threads.
// The incoming USB property 'rcv_prop' is valid if its ID is non-zero. Outgoing USB and DSP props
// will be sent out if their ID is non-zero.  It's OK to use floating point calculations here since
// this thread is not a real-time audio/DSP thread.

void app_control( const int rcv_prop[6], int usb_prop[6], int dsp_prop[6] );

// The mixer function is called once per audio sample and is used to route USB, I2S and DSP samples.
// This function should only be used to route samples and for very basic DSP processing - not for
// substantial sample processing since this may starve the I2S audio driver. Do not use floating
// point operations since this is a real-time audio thread - all DSP operations and calculations
// should be performed using fixed-point math.
// NOTE: IIR, FIR, and BiQuad coeff and state data *must* be declared non-static global!

void app_mixer( const int usb_output[32], int usb_input[32],
                const int i2s_output[32], int i2s_input[32],
                const int dsp_output[32], int dsp_input[32], const int property[6] );

// Audio Processing Threads. These functions run on tile 1 and are called once for each audio sample
// cycle. They cannot share data with the controller task or the mixer functions above that run on
// tile 0. The number of incoming and outgoing samples in the 'samples' array is set by the constant
// 'dsp_chan_count' defined above. Do not use floating point operations since these are real-time
// audio threads - all DSP operations and calculations should be performed using fixed-point math.
// NOTE: IIR, FIR, and BiQuad coeff and state data *must* be declared non-static global!

// Initialize DSP thread data (filter data and other algorithm data) here.
void app_initialize( void ); // Called once upon boot-up.

// Process samples (from the mixer function) and properties. Send results to stage 2.
void app_thread1( int samples[32], const int property[6] );

// Process samples (from stage 1) and properties. Send results to stage 3.
void app_thread2( int samples[32], const int property[6] );

// Process samples (from stage 2) and properties. Send results to stage 4.
void app_thread3( int samples[32], const int property[6] );

// Process samples (from stage 3) and properties. Send results to stage 5.
void app_thread4( int samples[32], const int property[6] );

// Process samples (from stage 4) and properties. Send results to the mixer function.
void app_thread5( int samples[32], const int property[6] );
```
