https://www.kickstarter.com/projects/689850781/flexfxtm-programmable-digital-audio-effects-platfo

FlexFX&trade;
==================================

FlexFX supports USB Audio and MIDI, 32/64-bit DSP, up to 32 audio channels, 48 to 384 kHz sampling rates, input to output latency of 350 microseconds. Devices can be updated with pre-built effects or custom designed effects at any time using standard USB/MIDI. No apps purchases, download costs, or user accounts are needed to load FlexFX pre-built effects to to develop your own custom applications/effects.

**Customization**  
FlexFX development Kit:  https://github.com/flexfx/flexfx.github.io
Compiler and Linker:     xTIMEcomposer (current version for FlexFX is 14.3.3) is available from www.xmos.com  

**Support**  
FlexFX user's forum:     https://flexfx.discussion.community  
XMOS xCORE Forum:        https://www.xcore.com  

Introduction
--------------------------------

The FlexFX&trade; provides a light framework for developing audio processing applications running on 3Degrees XIOUSB/DSP modules.
The XIO modules and FlexFX framework provides USB class 2.0 audio input and output, USB MIDI handling and routing,
up to four I2S/TDM interfaces (for multiple ADCs/DACs/CODECs - up to 32x32 input/output audio channels), and firmware
upgrades allowing custom audio application development to remain focused on custom signal processing and
run-time algorithm control.

![alt tag](flexfx.png)

* Very simple framework for writing custom audio processing applications
* Up to 500 MIPs available for signal processing algorithms (FlexFX module with XUF216)
* 32/64 bit fixed point DSP support, single-cycle instructions
* Up to 32x32 (48 kHz) channels of USB and I2S audio, up to 192 kHz audio sample rate at 8x8
* Single audio cycle DSP processing for all 32 channels (e.g. audio block size = 1)
* System latency (ADC I2S to DSP to DAC I2S) of 16 audio cycles (16/Fs). 
* USB interface for USB audio streaming ad USB MIDI for effects control and firmware updating
* Functions for I2C bus (for peripheral control) and port input/output
* I2S supports up to 8-slot TDM and up to four separate ADCs and four separate DACs

Getting Started
--------------------------------

1) Download and install the free XMOS development tools from www.xmos.com.

2) Obtain the FlexFX™ dev-kit for building your own apps from “https://github.com/markseel/flexfxkit”.    
     Download the ZIP file or use GIT …
```
git clone https://github.com/flexfx/flexfx.github.io.git
```

3) Set environment variables for using XMOS command-line build tools …
```
Windows         c:\Program Files (x86)\XMOS\xTIMEcomposer\Community_14.3.3\SetEnv.bat
OS X or Linux   /Applications/XMOS_xTIMEcomposer_Community_14.3.3/SetEnv.command
```

4) Add your own source code to a new 'C' file (e.g. ‘application.c’).

5) Build the application …
```
xcc -report -O3 -lquadflash xio.xn xio.a flexfx.c your_app.c -o your_app.xe
```

6) Burn your firmware application directly to FLASH using the XMOS JTAG (XTAG-2 or XTAG3) adapter or create a binary image for loading over USB/MIDI using the FlexFX firwmare upgrade-via-midi protocol.  The Python script 'xio.py' and the HTML file
'flexfx.html' (using Google Chrome) both support MIDI over USB firmware updating.
```
xflash --no-compression --factory-version 14.3 —-upgrade 1 your_app.xe
 — or —
xflash --no-compression --factory-version 14.3 --upgrade 1 your_app.xe -o your_app.bin
python flexfx.py 0 your_app.bin
```

7) A FlexFX system can be firmware upgraded using USB/MIDI.  But if that fails due to misbehaving custom programs or FLASH data corruption you can to revert your system back to its original/factory condition.  After resotring the system to its original/factory condition use steps 4/5/6 for custom FlexFX aoplication develoopment and firmware upgrading.  Use the JTAG device (XMOS XTAG2 or XTAG3) to load the factory FlexFX firmware into the factory and upgrade portions of the FLASH boot partitioin ...
```
xflash --boot-partition-size 1048576 --no-compression --factory xio.xe --upgrade 1 xio.xe
```

You can create custom audio processing effects by downloading the FlexFX&trade; audio processing framework, adding custom audio processing DSP code and property handling code, and then compiling and linking using XMOS tools (xTIMEcomposer, free to download).
The custom firmware can then be burned to FLASH using xTIMEcomposer and the XTAG-2 or XTAG-3 JTAG board ($20 from Digikey), via USB/MIDI (there are special properties defined for firmware upgrading and boot image selection).

FlexFX&trade; implements the USB class 2.0 audio and USB MIDI interfaces, the I2S/CODEC interface, sample data transfer from USB/I2S/DSP threads, property/parameter routing, and firmware upgrading. FlexFX provides functions for peripheral control (GPIO's and I2C), FLASH memory access, fixed point scalar adn vector math, and a wide range of DSP functions.

All one has to do then is add DSP code and any relevant custom property handling code to the five audio processing threads which all run in parallel and implement a 5-stage audio processing pipeline.
Each processing thread is allocated approximately 100 MIPS and executes once per audio cycle.
All threads are executed once per audio cycle and therefore operate on USB and I2S sample data one audio frame at a time.

Prebuilt Effects
----------------------------------

The FlexFX kit contains highly optimized effects for delay, overdrive, equalization, reverb, and cabinet simulation as part of the FlexFX C99 project. These effects are in source and binary (\*.bin) form and can be used for free on FlexFX boards.  All effects support full control over each effect's presets and parameter settings via FlexFX properties sent and received over USB/MIDI using the 'xio.py' script, 'c99.html' Google Chrome HTML interface, or any other
application programmed to use USB/MIDI for FlexFX property data transfer.

Programming Interface
-----------------------------------------

The 'xio.h' file defines the application interface for USB/I2S audio and MIDI applications while 'dsp.i'/'dsp.h' define DSP macros/functions and helper functions for creating audio effects applications.

XIO.H

```C
#ifndef INCLUDED_XIO_H
#define INCLUDED_XIO_H

// FQ converts Q28 fixed-point to floating point
// QF converts floating-point to Q28 fixed-point

#define QQ 28
#define FQ(hh) (((hh)<0.0)?((int)((double)(1u<<QQ)*(hh)-0.5)):((int)(((double)(1u<<QQ)-1)*(hh)+0.5)))
#define QF(xx) (((int)(xx)<0)?((double)(int)(xx))/(1u<<QQ):((double)(xx))/((1u<<QQ)-1))

typedef unsigned char byte;
typedef unsigned int  bool;

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

extern const char controller_script[];

// The control task is called at a rate of 1000 Hz and should be used to implement audio CODEC
// initialization/control, pot and switch sensing via I2C ADC's, handling of properties from USB
// MIDI, and generation of properties to be consumed by the USB MIDI host and by the DSP threads.
// The incoming USB property 'rcv_prop' is valid if its ID is non-zero and an outgoing USB
// property, as a response to the incoming property, will be sent out if's ID is non-zero. DSP
// propertys can be sent to DSP threads (by setting the DSP property ID to zero) at any time.
// It's OK to use floating point calculations here as this thread is not a real-time audio thread.

extern void xio_control( const int rcv_prop[6], int snd_prop[6], int dsp_prop[6] );

// The mixer function is called once per audio sample and is used to route USB, I2S and DSP samples.
// This function should only be used to route samples and for very basic DSP processing - not for
// substantial sample processing since this may starve the I2S audio driver. Do not use floating
// point operations since this is a real-time audio thread - all DSP operations and calculations
// should be performed using fixed-point math.
// NOTE: IIR, FIR, and BiQuad coeff and state data *must* be declared non-static global!

extern void xio_mixer( const int usb_output[32], int usb_input[32],
                       const int i2s_output[32], int i2s_input[32],
                       const int dsp_output[32], int dsp_input[32], const int property[6] );

// Audio Processing Threads. These functions run on tile 1 and are called once for each audio sample
// cycle. They cannot share data with the controller task or the mixer functions above that run on
// tile 0. The number of incoming and outgoing samples in the 'samples' array is set by the constant
// 'dsp_chan_count' defined above. Do not use floating point operations since these are real-time
// audio threads - all DSP operations and calculations should be performed using fixed-point math.
// NOTE: IIR, FIR, and BiQuad coeff and state data *must* be declared non-static global!

// Process samples and properties from the app_mixer function. Send results to stage 2.
extern void xio_thread1( int samples[32], const int property[6] );
// Process samples and properties from stage 1. Send results to stage 3.
extern void xio_thread2( int samples[32], const int property[6] );
// Process samples and properties from stage 2. Send results to stage 4.
extern void xio_thread3( int samples[32], const int property[6] );
// Process samples and properties from stage 3. Send results to stage 5.
extern void xio_thread4( int samples[32], const int property[6] );
// Process samples and properties from stage 4. Send results to the app_mixer function.
extern void xio_thread5( int samples[32], const int property[6] );

// On-board FLASH read write functions.

void flash_read ( int page, byte data[256] );
void flash_write( int page, const byte data[256] );

unsigned timer_count( void );
void timer_delay( int microseconds );

// Functions for peripheral control (*** do not use these in real-time DSP threads ***).

void i2c_start( int speed );  // Set bit rate (bps), assert an I2C start condition.
byte i2c_write( byte value ); // Write 8-bit data value.
byte i2c_read ( void );       // Read 8-bit data value.
void i2c_ack  ( byte ack );   // Assert the ACK/NACK bit after a read.
void i2c_stop ( void );       // Assert an I2C stop condition.

void spi_config  ( int speed );  // Set the SPI bit rate (bps).
void spi_select  ( byte csel );  // Assert (csel==1) or de-select (csel==0) the chip select.
byte spi_transfer( byte value ); // Write and read the next byte.

void port_put( int mask, int value ); // Write 0 or 1 to ports/pins indicated by 'mask' where ...
void port_set( int mask );            // ... each mask bit set to 1 specifies an action on that port ...
void port_clr( int mask );            // ... and where mask bit pos 0 is port 0, pos 1 is port 1, etc.
byte port_get( int mask );            // Read ports indicated by 'mask', set to HiZ state.

#endif
```

DSP.H

```C
#ifndef INCLUDED_DSP_H
#define INCLUDED_DSP_H

#include "xio.h"

void flexfx_control( double presets[20], int property[6] );

void audio_mixer( const int usb_output[32], int usb_input[32],
                  const int adc_output[32], int dac_input[32],
                  const int dsp_output[32], int dsp_input[32], const int property[6] );

// FQ converts Q28 fixed-point to floating point
// QF converts floating-point to Q28 fixed-point

#define QQ 28
#define FQ(hh) (((hh)<0.0)?((int)((double)(1u<<QQ)*(hh)-0.5)):((int)(((double)(1u<<QQ)-1)*(hh)+0.5)))
#define QF(xx) (((int)(xx)<0)?((double)(int)(xx))/(1u<<QQ):((double)(xx))/((1u<<QQ)-1))

// MAC performs 32x32 multiply and 64-bit accumulation, SAT saturates a 64-bit result, EXT converts
// a 64-bit result to a 32-bit value (extract 32 from 64), LD2/ST2 loads/stores two 32-values
// from/to 64-bit aligned 32-bit data arrays at address PP. All 32-bit fixed-point values are QQQ
// fixed-point formatted.
//
// AH (high) and AL (low) form the 64-bit signed accumulator
// XX, YY, and AA are 32-bit QQQ fixed point values
// PP is a 64-bit aligned pointer to two 32-bit QQQ values

#define DSP_LD2( pp, xx, yy )     asm volatile("ldd %0,%1,%2[0]":"=r"(xx),"=r"(yy):"r"(pp));
#define DSP_ST2( pp, xx, yy )     asm volatile("std %0,%1,%2[0]"::"r"(xx), "r"(yy),"r"(pp));
#define DSP_MUL( ah, al, xx, yy ) asm volatile("maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(xx),"r"(yy),"0"(0),"1"(1<<(QQ-1)) );
#define DSP_MAC( ah, al, xx, yy ) asm volatile("maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(xx),"r"(yy),"0"(ah),"1"(al) );
#define DSP_SAT( ah, al )         asm volatile("lsats %0,%1,%2":"=r"(ah),"=r"(al):"r"(QQ),"0"(ah),"1"(al));
#define DSP_EXT( ah, al, xx )     asm volatile("lextract %0,%1,%2,%3,32":"=r"(xx):"r"(ah),"r"(al),"r"(QQ));
#define DSP_DIV( qq,rr,ah,al,xx ) asm volatile("ldivu %0,%1,%2,%3,%4":"=r"(qq):"r"(rr),"r"(ah),"r"(al),"r"(xx));

inline int dsp_mul( int xx, int yy ) // RR = XX * YY
{
    int ah = 0; unsigned al = 1<<(QQ-1);
    asm("maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(xx),"r"(yy),"0"(ah),"1"(al) );
    asm("lextract %0,%1,%2,%3,32":"=r"(ah):"r"(ah),"r"(al),"r"(QQ));
    return ah;
}

inline int dsp_mac( int xx, int yy, int zz ) // RR = XX * YY + ZZ
{
    int ah = 0; unsigned al = 0;
    asm("maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(xx),"r"(yy),"0"(0),"1"(zz) );
    asm("lextract %0,%1,%2,%3,32":"=r"(ah):"r"(ah),"r"(al),"r"(QQ));
    return ah;
}

inline int dsp_ext( int ah, int al ) // RR = AH:AL >> (64-QQ)
{
    asm volatile("lextract %0,%1,%2,%3,32":"=r"(ah):"r"(ah),"r"(al),"r"(QQ));
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
void math_sum_X  ( const int* xx, int nn, int* ah, unsigned* al ); // ah:al = sum(X[0:N-1])
void math_asm_X  ( const int* xx, int nn, int* ah, unsigned* al ); // ah:al = sum(abs(X[0:N-1))
void math_pwr_X  ( const int* xx, int nn, int* ah, unsigned* al ); // ah:al = sum(X[0:N-1]*X[0:N-1])
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
// CU is array of coefficients specifically for FIR upsampling - see 'mix_fir_coeffs'
// SS is array of 32-bit filter state - length is 'nn' for FIR, nn * 4 for IIR
// SS is array of 32-bit filter state - length is 3 for DCBLOCK, 2 for state-variable filter
// CC length is 3/5/7 and SS length is 2/4/6 fir IIR1/IIR2/IIR3 respectively
// RR is the up-sampling/interpolation or down-sampling/decimation ratio
// AH (high) and AL (low) form the 64-bit signed accumulator

int  dsp_blend   ( int dry, int wet, int blend );     // 0 (100% dry) <= MM <= 1 (100% wet)
int  dsp_interp  ( int  dd, int y1, int y2 );         // 1st order (linear) interpolation
int  dsp_lagrange( int  dd, int y1, int y2, int y3 ); // 2nd order (Lagrange) interpolation
void dsp_statevar( int* xx, const int* cc, int* ss ); // x[0]=in,xx[0:2]=lp/bp/hp out, c[0]=Fc,cc[1]=Q
int  dsp_iir1    ( int  xx, const int* cc, int* ss ); // 1st order IIR filter - cc[3]=b0,b1,a1
int  dsp_iir2    ( int  xx, const int* cc, int* ss ); // 2nd order IIR filter - cc[5]=b0,b1,b2,a1,a2
int  dsp_iir3    ( int  xx, const int* cc, int* ss ); // 3rd order IIR filter - cc[7]=b0..b3,a1..a3
int  dsp_biquad  ( int  xx, const int* cc, int* ss, int nn ); // nn Cascaded bi-quad IIR filters
int  dsp_fir     ( int  xx, const int* cc, int* ss, int nn ); // FIR filter of nn taps
void dsp_fir_up  ( int* xx, const int* cU, int* ss, int nn, int rr ); // FIR up-sampling/interpolation
void dsp_fir_dn  ( int* xx, const int* cc, int* ss, int nn, int rr ); // FIR dn-sampling/decimation
void dsp_cic_up  ( int* xx, const int* cc, int* ss, int nn, int rr ); // CIC up-sampling/interpolation
void dsp_cic_dn  ( int* xx, const int* cc, int* ss, int nn, int rr ); // CIC dn-sampling/decimation
int  dsp_convolve( int  xx, const int* cc, int* ss, int* ah, int* al, int nn ); // NN*24 taps

// FIXME: dsp_statevar is not working properly.

void mix_fir_coeffs( int* upsample_cc, int* fir_cc, int nn, int rr );
 
// Filter coefficient calculation functions (do not use these in real-time DSP threads).
//
// CC is an array of floating point filter coefficients
// FF, QQ, GG are the filter frequency, filter Q-factor, and filter gain
// For low/high pass filters set QQ to zero to create a single-pole 6db/octave filter
// GB/GM/GT are bass/mid/treble gains (0.0=min, 1.0=max).
// VB/VM/Vt are bass/mid/treble freq variation from standard (new_freq = standard_freq * variation).

void calc_notch    ( int cc[5], double ff, double qq );
void calc_lowpass  ( int cc[5], double ff, double qq );
void calc_highpass ( int cc[5], double ff, double qq );
void calc_allpass  ( int cc[5], double ff, double qq );
void calc_bandpassQ( int cc[5], double ff, double qq );
void calc_bandpassF( int cc[5], double ff1, double ff2 );
void calc_peaking  ( int cc[5], double ff, double qq, double gg );
void calc_lowshelf ( int cc[5], double ff, double qq, double gg );
void calc_highshelf( int cc[5], double ff, double qq, double gg );
void calc_tonestack( int cc[7], double gb, double gm, double gt, double vb, double vm, double vt );

#endif
```

Minimal Application
-----------------------------------------

The application programmer only needs to add control and audip processing code to create a complete application.  The code below is a complete application.  Just add code to the 'xio\_control', 'xio\_mixer', and 'xio\_thread' functions.

```C
#include "xio.h"
#include "dsp.i"
#include "dsp.h"

const char* product_name_string   = "Example";           // The product name
const char* usb_audio_output_name = "Example Audio Out"; // USB audio output name
const char* usb_audio_input_name  = "Example Audio In";  // USB audio input name
const char* usb_midi_output_name  = "Example MIDI Out";  // USB MIDI output name
const char* usb_midi_input_name   = "Example MIDI In";   // USB MIDI input name

const int audio_sample_rate     = 192000; // Default sample rate at boot-up
const int dsp_channel_count     = 1;
const int usb_output_chan_count = 2;     // 2 USB audio class 2.0 output channels
const int usb_input_chan_count  = 2;     // 2 USB audio class 2.0 input channels
const int i2s_channel_count     = 2;     // ADC/DAC channels per SDIN/SDOUT wire (2,4,or 8)
const int output_drive_type     = 0;     // 0 for instrument level, 1 for line level.

const int i2s_sync_word[8] = { 0xFFFFFFFF,0x00000000,0,0,0,0,0,0 }; // I2S WCLK values per slot

void xio_control( const int rcv_prop[6], int usb_prop[6], int dsp_prop[6] )
{
}

void xio_mixer( const int usb_output[32], int usb_input[32],
                const int adc_output[32], int dac_input[32],
                const int dsp_output[32], int dsp_input[32], const int property[6] )
{
}

void xio_initialize( void ) {}

void xio_thread1( int samples[32], const int property[6] ) {}
void xio_thread2( int samples[32], const int property[6] ) {}
void xio_thread3( int samples[32], const int property[6] ) {}
void xio_thread4( int samples[32], const int property[6] ) {}
void xio_thread5( int samples[32], const int property[6] ) {}
```

FlexFX Properties
----------------------------------

FlexFX applications can be controlled using FlexFX property exchanges over USB MIDI.
A property is composed of a non-zero 16-bit ID and five 32-bit data words.

An example property is shown below:

```
Property ID = 0x1234
Param 1     = 0x11223344
Param 2     = 0x55667788
Param 3     = 0x99aabbcc
Param 4     = 0x01234567
Param 5     = 0x89abcdef
```

FlexFX properties are transfered via over USB/MIDI using MIDI SYSEX messages.
The FlexFX framework handles parsing and rendering of MIDI SYSEX encapulated FlexFX data therefore the user
application need not deal with MIDi SYSEX - the audio firmware only sees 16-bit ID and five 32-word property values.

```
ID       DIRECTION        DESCRIPTION

0x0001   Bidirectional    Identify the device, return ID (3DEGFLEX) and version numbers
0x0002   Bidirectional    Begin firmware upgrade, echoed back to host
0x0003   Bidirectional    Next 32 bytes of firmware image data, echoed
0x0004   Host to Device   End firmware upgrade and reset (no USB property echo!)
0x0005   Bidirectional    Return four knob positions and footswitch state
0x0006   Bidirectional    Return the effect title string (up to 20 characters)
0x0007   Bidirectional    Return a parameter label for parameter 1 (up to 20 characters)
...
0x001a   Bidirectional    Return a parameter label for parameter 20 (up to 20 characters)
0x001b   Bidirectional    Read or write 20 parameters (0 for read, 1..99 to write) for preset #1
...
0x001f   Bidirectional    Read or write 20 parameters (0 for read, 1..99 to write) for preset #1
```

#### FlexFX ID = 0x0001: Identify; return ID (3DEGFLEX) and versions

```
USB host ---- [ 0x0001,    0,    0,    0,       0, 0 ] ---> Device
USB host <--- [ 0x0001, 3DEG, FLEX, serial_num, 0, 0 ] ---- Device
```
The USB host can use this property to solicit information about that attached device and determine whether or not it is a FlexFX device.  The device will return the flexfx signature words, its serial number, and will echo property words #4 and #5.  Currently the HTML interfrace ('flexfx.html') uses words #4 and #5 to echo back unique ID's used to bind HTML web application instances to a particular USB-attached FlexFX device.

#### FlexFX ID = 0x0002: Begin firmware upgrade

```
USB host ---- [ 0x0002, 0, 0, 0, 0, 0 ] ---> Device
USB host <--- [ 0x0002, 0, 0, 0, 0, 0 ] ---- Device
```
Open the FLASH device and erase it to begin the firmware upgrade process.

#### FlexFX ID = 0x0003: Continue firmware upgrade

```
USB host ---- [ 0x0003, data1, data2, data3, data4, data5 ] ---> Device
USB host <--- [ 0x0003, data1, data2, data3, data4, data5 ] ---- Device
```
Write the next 40 bytes of firmware data to FLASH.

#### FlexFX ID = 0x0004: End firmware upgrade

```
USB host ---- [ 0x0004, 0, 0, 0, 0, 0 ] ---> Device
USB host <--- [ 0x0004, 0, 0, 0, 0, 0 ] ---- Device
```
Close thge FLASH device and reboot to end the firmware upgrade process.

#### FlexFX ID = 0x0005: Read effect title (N=0) or label for parameter N (1 <= N <= 20)

```
USB host ---- [ 0x0005,     0,     0,     0,     0,     0 ] ---> Device
USB host <--- [ 0x0006, text1, text2, text3, text4, text5 ] ---- Device
```
Returns the ASCII/text for the attached device's name and parameter labels.
Eeach line contains 20 bytes of text with each 32-bit property word (property[1] ... property[5]) containing four 8-bit ASCII characters.

210p      Bidirectional    Read preset parameter values for preset P (0 <= P < 16)
211p      Bidirectional    Write preset parameter values for preset P (0 <= P < 16)

#### FlexFX ID = 0x0006 - 0x0019: Read preset parameter values

```
USB host ---- [ 0x0006,           0,           0,            0,             0,             0 ] ---> Device
USB host <--- [ 0x0006, values[1:4], values[5:8], values[9:12], values[13:16], values[17:20] ] ---- Device
```

Returns all parameter values for preset P.  Each parameter value ranges from 0 to 99.

#### FlexFX ID = 0x1a: Write preset parameter values for preset

```
USB host ---- [ 0x0006, values[1:4], values[5:8], values[9:12], values[13:16], values[17:20] ] ---> Device
USB host <--- [ 0x0006, values[1:4], values[5:8], values[9:12], values[13:16], values[17:20] ] ---- Device

where 1 <= value <= 99
```

Writes/updates all parameter values for preset P.  Each parameter value ranges from 1 to 99.

Programming Tools
-------------------------------------

The Python script 'xio.py' implements various host functions for testing XIO/FlexFX firmware via USB.

#### Usage #1

Display attached USB MIDI devices.  The FlexFX DSP board enumerates as device #0 in this example.
```
bash$ python xio.py
MIDI Output Devices: 0='FlexFX'
MIDI Input Devices:  0='FlexFX'
```

#### Usage #2

Burn a custom firmware application to the DSP board's FLASH memory (board is enumerated as MIDI device #0).  This takes about 10 seconds.
```
bash$ python xio.py 0 c99_cabsim.bin
.....................................................................................Done.
```

Design Tools
--------------------------------

The 'dsp.py' Python script has functions to aid with algorithm and filter design.  The FIR and IIR related functions require that the Python packeges 'scipy' and 'matplotlib' are installed.

#### FIR Filter Design Script

```
bash-3.2$ python dsp.py fir
Usage: python util_fir <pass_freq> <stop_freq> <ripple> <attenuation>
       <pass_freq> is the passband frequency relative to Fs (0 <= freq < 0.5)
       <stop_freq> is the stopband frequency relative to Fs (0 <= freq < 0.5)
       <ripple> is the maximum passband ripple
       <attenuation> is the stopband attenuation
```

'xio.py' is used to generate filter coefficients and to plot the filter response.  These coefficients can be used directly in your custom application 'C' file since the FQ macro converts floating point to Q28 fixed point format required by FlexFX DSP functions.  For this example the filter passband frequency is 0.2 (e.g. 9.6kHz @ 48 kHz Fs), a stopband frequency of 0.3 (e.g. 14.4 kHz @ 48 kHz Fs), a maximum passband ripple of 1.0 dB, and a stopband attenuation of -60 dB.  This script will display the magnitude response and the impulse response, and then print out the 38 filter coefficients for the resulting 38-tap FIR filter.

```
bash-3.2$ python dsp.py fir 0.2 0.3 1.0 -60       
int coeffs[38] = 
{	
    FQ(-0.000248008),FQ(+0.000533374),FQ(+0.000955507),FQ(-0.001549687),FQ(-0.002356083),
    FQ(+0.003420655),FQ(+0.004796811),FQ(-0.006548327),FQ(-0.008754448),FQ(+0.011518777),
    FQ(+0.014985062),FQ(-0.019366261),FQ(-0.025001053),FQ(+0.032472519),FQ(+0.042885423),
    FQ(-0.058618855),FQ(-0.085884667),FQ(+0.147517761),FQ(+0.449241500),FQ(+0.449241500),
    FQ(+0.147517761),FQ(-0.085884667),FQ(-0.058618855),FQ(+0.042885423),FQ(+0.032472519),
    FQ(-0.025001053),FQ(-0.019366261),FQ(+0.014985062),FQ(+0.011518777),FQ(-0.008754448),
    FQ(-0.006548327),FQ(+0.004796811),FQ(+0.003420655),FQ(-0.002356083),FQ(-0.001549687),
    FQ(+0.000955507),FQ(+0.000533374),FQ(-0.000248008)
};
```
![alt tag](https://raw.githubusercontent.com/markseel/flexfx_kit/master/util_fir.png)

#### IIR Filter Design Script

```
bash$ python dsp.py iir
Usage: python dsp.py iir <type> <freq> <Q> <gain>
       <type> filter type (notch, lowpass, highpass, allpass, bandpass, peaking, highshelf, or lowshelf
       <freq> is cutoff frequency relative to Fs (0 <= freq < 0.5)
       <Q> is the filter Q-factor
       <gain> is the filter positive or negative gain in dB
```

'dsp.py' is used to generate biquad IIR filter coefficients (B0, B1, B2, A1, and A2) and to plot the filter response.  These coefficients can be used directly in your custom application 'C' file since the FQ macro converts floating point to Q28 fixed point format required by FlexFX DSP functions.  For this example a bandpass filter and a peaking filter were designed.  This script will display the magnitude and phase responses and then print out the 38 filter coefficients for the resulting 38-tap FIR filter.

```
bash$ python dsp.py iir bandpass 0.25 0.707 12.0
int coeffs[5] = {FQ(+0.399564099),FQ(+0.000000000),FQ(-0.399564099),FQ(-0.000000000),FQ(+0.598256395)};

bash-3.2$ python dsp.py iir peaking 0.2 2.5 12.0       
int coeffs[5] = {FQ(+1.259455676),FQ(-0.564243794),FQ(+0.566475599),FQ(-0.564243794),FQ(+0.825931275)};
```
![alt tag](https://raw.githubusercontent.com/markseel/flexfx_kit/master/util_iir.png)

#### WAVE File Parsing Script

'dsp.py' is used to parse one or more WAVE files and print the WAVE file sample values to the console in floating point format.

```
bash$ python dsp.py wave ir1.wav ir2.wav
-0.00028253 +0.00012267 
+0.00050592 -0.00001955 
+0.00374091 +0.00060725 
+0.01383054 +0.00131202 
+0.03626263 +0.00480974 
+0.08285010 +0.01186383 
+0.16732728 +0.03065085 
+0.30201840 +0.08263147 
+0.47006404 +0.20321691 
+0.64849544 +0.41884899 
+0.81581724 +0.72721589 
+0.94237792 +0.97999990 
+0.92178667 +0.96471488 
+0.72284126 +0.66369593 
+0.34437060 +0.25017858 
-0.06371593 -0.16284466 
-0.39860737 -0.51766562 
...
```

#### Data Plotting Script

```
bash$ python dsp.py
Usage: python dsp.py plot <datafile> time
       python dsp.py plot <datafile> time [beg end]
       python dsp.py plot <datafile> freq lin
       python dsp.py plot <datafile> freq log
Where: <datafile> Contains one sample value per line.  Each sample is an
                  ASCII/HEX value (e.g. FFFF0001) representing a fixed-
                  point sample value.
       time       Indicates that a time-domain plot should be shown
       freq       Indicates that a frequency-domain plot should be shown
       [beg end]  Optional; specifies the first and last sample in order to
                  create a sub-set of data to be plotted
Examle: Create time-domain plot data in 'out.txt' showing samples 100
        through 300 ... bash$ python plot.py out.txt 100 300
Examle: Create frequency-domain plot data in 'out.txt' showing the Y-axis
        with a logarithmic scale ... bash$ python plot.py out.txt freq log
```

The 'dsp.py' script graphs data contained in a data file which can contan an arbitrary number of data columns of floating point values.  The data can be plotted in the time domain or in the frequency domain.  In this example two cabinet simulation data files are plotted to show their frequency domain impulse responses (i.e the cabinet frequency responses).

```
bash$ python dsp.py wave ir1.wav ir2.wav > output.txt
bash$ python dsp.py plot output.txt freq log
bash$ python dsp.py plot output.txt time 0 150
```
![alt tag](https://raw.githubusercontent.com/markseel/flexfx_kit/master/util_plot.png)
