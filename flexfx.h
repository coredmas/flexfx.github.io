#ifndef INCLUDED_FLEXFX_H
#define INCLUDED_FLEXFX_H

// FQ converts Q28 fixed-point to floating point
// QF converts floating-point to Q28 fixed-point

#define QQ 28
#define FQ(hh) (((hh)<0.0)?((int)((double)(1u<<QQ)*(hh)-0.5)):((int)(((double)(1u<<QQ)-1)*(hh)+0.5)))
#define QF(xx) (((int)(xx)<0)?((double)(int)(xx))/(1u<<QQ):((double)(xx))/((1u<<QQ)-1))

typedef unsigned char byte;
typedef unsigned int  bool;

// The control task is called at a rate of 1000 Hz and should be used to implement audio CODEC
// initialization/control, pot and switch sensing via I2C ADC's, handling of properties from USB
// MIDI, and generation of properties to be consumed by the USB MIDI host and by the DSP threads.
// The incoming USB property 'rcv_prop' is valid if its ID is non-zero. Outgoing USB and DSP props
// will be sent out if their ID is non-zero - but they are only available for routing if their ID's
// are zero (non-zero indicating that routing is still in progress). It's OK to use floating point
// calculations here since this thread is not a real-time audio or DSP thread.

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

// Process samples and properties from the app_mixer function. Send results to stage 2.
void app_thread1( int samples[32], const int property[6] );
// Process samples and properties from stage 1. Send results to stage 3.
void app_thread2( int samples[32], const int property[6] );
// Process samples and properties from stage 2. Send results to stage 4.
void app_thread3( int samples[32], const int property[6] );
// Process samples and properties from stage 3. Send results to stage 5.
void app_thread4( int samples[32], const int property[6] );
// Process samples and properties from stage 4. Send results to the app_mixer function.
void app_thread5( int samples[32], const int property[6] );

// Flash memory functions for data persistance (do not use these in real-time DSP threads).
// Each page consists of 3268 5-byte properties (total of 65530 bytes). There are 16
// property pages in FLASH and one property page in RAM used as a scratch buffer. Pages can
// be loaded and saved from/to FLASH. All USB MIDI property flow (except for properties with
// ID >= 0x8000 used for FLASH/RAM control) results in updates the RAM scratch buffer.

extern void page_load ( int page_num );      // Load 64Kbyte page from FLASH to RAM
extern void page_save ( int page_num );      // Save 64Kbyte page from RAM to FLASH
extern void page_read ( int property[6] );   // Read one from RAM (prop index = property[0])
extern void page_write( const int prop[6] ); // Write one property to RAM at index=prop[0]

// Port and pin I/O functions. DAC/ADC port reads/writes will disable I2S/TDM!

extern void io_write( int pin_num, bool value ); // Write binary value to IO pin
extern bool io_read ( int pin_num );             // Read binary value from IO pin

// I2C functions for peripheral control (do not use these in real-time DSP threads).

extern void i2c_start( int speed );  // Set bit rate, assert an I2C start condition.
extern byte i2c_write( byte value ); // Write 8-bit data value.
extern byte i2c_read ( void );       // Read 8-bit data value.
extern void i2c_ack  ( byte ack );   // Assert the ACK/NACK bit after a read.
extern void i2c_stop ( void );       // Assert an I2C stop condition.

extern void port_set1( int flag );
extern int  port_get1( void );
extern void port_set2( int flag );
extern int  port_get2( void );

// MAC performs 32x32 multiply and 64-bit accumulation, SAT saturates a 64-bit result, EXT converts
// a 64-bit result to a 32-bit value (extract 32 from 64), LD2/ST2 loads/stores two 32-values
// from/to 64-bit aligned 32-bit data arrays at address PP. All 32-bit fixed-point values are QQQ
// fixed-point formatted.
//
// AH (high) and AL (low) form the 64-bit signed accumulator
// XX, YY, and AA are 32-bit QQQ fixed point values
// PP is a 64-bit aligned pointer to two 32-bit QQQ values

#define DSP_MUL( ah, al, xx, yy ) asm volatile("maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(xx),"r"(yy),"0"(0),"1"(1<<(QQ-1)) );
#define DSP_MAC( ah, al, xx, yy ) asm volatile("maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(xx),"r"(yy),"0"(ah),"1"(al) );
#define DSP_DIV( qq,rr,ah,al,xx ) asm volatile("ldivu %0,%1,%2,%3,%4":"=r"(qq):"r"(rr),"r"(ah),"r"(al),"r"(xx));
#define DSP_SAT( ah, al )         asm volatile("lsats %0,%1,%2":"=r"(ah),"=r"(al):"r"(QQ),"0"(ah),"1"(al));
#define DSP_EXT( xx, ah, al )     asm volatile("lextract %0,%1,%2,%3,32":"=r"(xx):"r"(ah),"r"(al),"r"(QQ));
#define DSP_LD2( pp, xx, yy )     asm volatile("ldd %0,%1,%2[0]":"=r"(xx),"=r"(yy):"r"(pp));
#define DSP_ST2( pp, xx, yy )     asm volatile("std %0,%1,%2[0]"::"r"(xx), "r"(yy),"r"(pp));

inline int dsp_multiply( int xx, int yy ) // RR = XX * YY
{
    int ah = 0; unsigned al = 1<<(QQ-1);
    asm("maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(xx),"r"(yy),"0"(ah),"1"(al) );
    asm("lextract %0,%1,%2,%3,32":"=r"(ah):"r"(ah),"r"(al),"r"(QQ));
    return ah;
}

extern int dsp_sine_10[ 1024], dsp_atan_10[ 1024], dsp_tanh_10[ 1024], dsp_nexp_10[ 1024];
extern int dsp_sine_12[ 4096], dsp_atan_12[ 4096], dsp_tanh_12[ 4096], dsp_nexp_12[ 4096];
extern int dsp_sine_14[16384], dsp_atan_14[16384], dsp_tanh_14[16384], dsp_nexp_14[16384];

// Math and filter functions.
//
// XX, CC, SS, Yn, MM, and AA are 32-bit fixed point samples/data in QQQ format
// DD is the distance (0<=DD<1) between the first two points for interpolation
// KK is a time constant, QQQ format
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
// XX, CC, SS, Yn, MM, and AA are 32-bit fixed point samples/data in QQQ format
// DD is the distance (0<=DD<1) between the first two points for interpolation
// KK is a time constant, QQQ format
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

// Biquad filter coefficient calculation functions (do not use these in real-time DSP threads).
//
// CC is an array of floating point filter coefficients
// FF, QQ, GG are the filter frequency, filter Q-factor, and filter gain
// For low/high pass filters set QQ to zero to create a single-pole 6db/octave filter

void calc_notch    ( int cc[5], double ff, double qq );
void calc_lowpass  ( int cc[5], double ff, double qq );
void calc_highpass ( int cc[5], double ff, double qq );
void calc_allpass  ( int cc[5], double ff, double qq );
void calc_bandpass ( int cc[5], double ff1, double ff2 );
void calc_peaking  ( int cc[5], double ff, double qq, double gg );
void calc_lowshelf ( int cc[5], double ff, double qq, double gg );
void calc_highshelf( int cc[5], double ff, double qq, double gg );

// Functions and variables to be implemented by the application ...

extern const char* product_name_string;   // Your product name
extern const char* usb_audio_output_name; // Your USB audio output name
extern const char* usb_audio_input_name;  // Your USB audio input name
extern const char* usb_midi_output_name;  // Your USB MIDI output name
extern const char* usb_midi_input_name;   // Your USB MIDI input name

extern const int audio_sample_rate;       // Audio sampling frequency (48K,96K,192K,or 384K)
extern const int usb_output_chan_count;   // 2 USB audio output channels (32 max)
extern const int usb_input_chan_count;    // 2 USB audio input channels (32 max)
extern const int i2s_channel_count;       // 2,4,or8 I2S channels per SDIN/SDOUT wire

extern const int i2s_sync_word[8];        // I2S WCLK words for each slot

// The control task is called at a rate of 1000 Hz and should be used to implement audio CODEC
// initialization/control, pot and switch sensing via I2C ADC's, handling of properties from USB
// MIDI, and generation of properties to be consumed by the USB MIDI host and by the DSP threads.
// The incoming USB property 'rcv_prop' is valid if its ID is non-zero. Outgoing USB and DSP props
// will be sent out if their ID is non-zero.  It's OK to use floating point calculations here since
// this thread is not a real-time audio/DSP thread.

extern void app_control( const int rcv_prop[6], int usb_prop[6], int dsp_prop[6] );

// The mixer function is called once per audio sample and is used to route USB, I2S and DSP samples.
// This function should only be used to route samples and for very basic DSP processing - not for
// substantial sample processing since this may starve the I2S audio driver. Do not use floating
// point operations since this is a real-time audio thread - all DSP operations and calculations
// should be performed using fixed-point math.
// NOTE: IIR, FIR, and BiQuad coeff and state data *must* be declared non-static global!

extern void app_mixer( const int usb_output[32], int usb_input[32],
                       const int i2s_output[32], int i2s_input[32],
                       const int dsp_output[32], int dsp_input[32], const int property[6] );

// Audio Processing Threads. These functions run on tile 1 and are called once for each audio sample
// cycle. They cannot share data with the controller task or the mixer functions above that run on
// tile 0. The number of incoming and outgoing samples in the 'samples' array is set by the constant
// 'dsp_chan_count' defined above. Do not use floating point operations since these are real-time
// audio threads - all DSP operations and calculations should be performed using fixed-point math.
// NOTE: IIR, FIR, and BiQuad coeff and state data *must* be declared non-static global!

// Initialize DSP thread data (filter data and other algorithm data) here.
extern void app_initialize( void ); // Called once upon boot-up.

// Process samples (from the mixer function) and properties. Send results to stage 2.
extern void app_thread1( int samples[32], const int property[6] );

// Process samples (from stage 1) and properties. Send results to stage 3.
extern void app_thread2( int samples[32], const int property[6] );

// Process samples (from stage 2) and properties. Send results to stage 4.
extern void app_thread3( int samples[32], const int property[6] );

// Process samples (from stage 3) and properties. Send results to stage 5.
extern void app_thread4( int samples[32], const int property[6] );

// Process samples (from stage 4) and properties. Send results to the mixer function.
extern void app_thread5( int samples[32], const int property[6] );

#endif
