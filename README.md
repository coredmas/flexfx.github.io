FlexFX&trade; Kit
==================================

FlexFX hardware supports USB Audio and MIDI, 32/64-bit DSP, up to 32 audio channels, 48 to 384 kHz sampling rates, input to output latency of 350 microseconds. Devices can be updated with pre-built effects or custom designed effects at any time using standard USB/MIDI. No apps purchases, download costs, or user accounts are needed to load FlexFX pre-built effects to to develop your own custom applications/effects.

**Customization**  
FlexFX development Kit:  https://github.com/markseel/flexfx_kit  
Compiler and Linker:     xTIMEcomposer (current version for FlexFX is 14.3.3) is available from www.xmos.com  

**Support**  
FlexFX user's forum:     https://flexfx.discussion.community  
XMOS xCORE Forum:        https://www.xcore.com  

Introduction
--------------------------------

The FlexFX&trade; Kit provides a light framework for developing audio processing applications running on FlexFX&trade;
hardware modules and boards.
It  implements USB class 2.0 audio input and output, USB MIDI handling and routing,
up to four I2S/TDM interfaces (for multiple ADCs/DACs/CODECs), and firmware
upgrades allowing custom audio application development to remain focused on custom signal processing and
run-time algorithm control.

![alt tag](flexfx.png)

* Simple framework for writing custom audio processing applications
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
xcc -report -O3 -lquadflash flexfx.xn flexfx.a application.c -o appliocation.xe
```

6) Burn the firmware executable directly to FLASH or create a binary image for loading over USB/MIDI using the firwmare upgrade process.  Firmware can be upgraded via via Pyhon and "flexfx.py", via Google Chrome and "flexfx.html", or via a custom application that uses USB/MIDI and FlexFX property data to perform a firmware upgrade.
```
xflash --no-compression --factory-version 14.3 —-upgrade 1 application.xe
 — or —
xflash --no-compression --factory-version 14.3 --upgrade 1 application.xe -o application.bin
python flexfx.py 0 application.bin
```

7) A FlexFX system can be firmware upgraded using USB/MIDI.  But if that fails due to misbehaving custom FLexFX programs or FLASH data corruption you can to revert your system back to its original/factory condition.  After resotring the system to its original/factory condition use steps 4/5/6 for custom FlexFX aoplication develoopment and firmware upgrading.  Use the JTAG device (XMOS XTAG2 or XTAG3) to load the factory FlexFX firmware into the factory and upgrade portions of the FLASH boot partitioin ...
```
xflash --boot-partition-size 1048576 --no-compression --factory flexfx.xe --upgrade 1 flexfx.xe
```

You can create custom audio processing effects by downloading the FlexFX&trade; audio processing framework, adding custom audio processing DSP code and property handling code, and then compiling and linking using XMOS tools (xTIMEcomposer, free to download).
The custom firmware can then be burned to FLASH using xTIMEcomposer and the XTAG-2 or XTAG-3 JTAG board ($20 from Digikey), via USB/MIDI (there are special properties defined for firmware upgrading and boot image selection).

FlexFX&trade; implements the USB class 2.0 audio and USB MIDI interfaces, the I2S/CODEC interface, sample data transfer from USB/I2S/DSP threads, property/parameter routing, and firmware upgrading. FlexFX provides functions for peripheral control (GPIO's and I2C), FLASH memory access, fixed point scalar adn vector math, and a wide range of DSP functions.

All one has to do then is add DSP code and any relevant custom property handling code to the five audio processing threads which all run in parallel and implement a 5-stage audio processing pipeline.
Each processing thread is allocated approximately 100 MIPS and executes once per audio cycle.
All threads are executed once per audio cycle and therefore operate on USB and I2S sample data one audio frame at a time.

Programming Interface
-----------------------------------------

The application programmer only needs to add control and audip processing code to create a complete application.  The code below is a complete application (that does nothing).  Just add code to the 'app\_control', 'app\_mixer', and 'app\_thread' functions.  The complete FlexFX programming interface is show below:


```C
#ifndef INCLUDED_FLEXFX_H
#define INCLUDED_FLEXFX_H

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

extern void app_control( const int rcv_prop[6], int snd_prop[6], int dsp_prop[6] );

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

// Process samples and properties from the app_mixer function. Send results to stage 2.
extern void app_thread1( int samples[32], const int property[6] );
// Process samples and properties from stage 1. Send results to stage 3.
extern void app_thread2( int samples[32], const int property[6] );
// Process samples and properties from stage 2. Send results to stage 4.
extern void app_thread3( int samples[32], const int property[6] );
// Process samples and properties from stage 3. Send results to stage 5.
extern void app_thread4( int samples[32], const int property[6] );
// Process samples and properties from stage 4. Send results to the app_mixer function.
extern void app_thread5( int samples[32], const int property[6] );

// Flash memory functions for data persistance (do not use these in real-time DSP threads).
// Each page consists of 3268 5-byte properties (total of 65530 bytes). There are 16
// property pages in FLASH and one property page in RAM used as a scratch buffer. Pages can
// be loaded and saved from/to FLASH. All USB MIDI property flow (except for properties with
// ID >= 0x8000 used for FLASH/RAM control) results in updates the RAM scratch buffer.

void page_load ( int page_num );                 // Load 64Kbyte page from FLASH to RAM.
void page_save ( int page_num );                 // Save 64Kbyte page from RAM to FLASH.
int  page_read ( int index, int data[5] );       // Read prop data from RAM, return page num.
void page_write( int index, const int data[5] ); // Write prop data to RAM at index.

// Port and pin I/O functions. DAC/ADC port reads/writes will disable I2S/TDM!

void io_write( int pin_num, bool value ); // Write binary value to IO pin
bool io_read ( int pin_num );             // Read binary value from IO pin

// I2C functions for peripheral control (do not use these in real-time DSP threads).

void i2c_start( int speed );  // Set bit rate, assert an I2C start condition.
byte i2c_write( byte value ); // Write 8-bit data value.
byte i2c_read ( void );       // Read 8-bit data value.
void i2c_ack  ( byte ack );   // Assert the ACK/NACK bit after a read.
void i2c_stop ( void );       // Assert an I2C stop condition.

void port_set1( int flag );
int  port_get1( void );
void port_set2( int flag );
int  port_get2( void );

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

#endif
```

FlexFX Properties
----------------------------------

FlexFX applications are controlled using FlexFX property exchanges over USB MIDI.
A property is composed of a 16-bit IDC and five 32-bit data words.

An example property is shown below:

```
Property ID = 0x1234       The ID must have a non-zero 16-bit upper word (0x0101 in this example)
Param 1     = 0x11223344
Param 2     = 0x55667788
Param 3     = 0x99aabbcc
Param 4     = 0x01234567
Param 5     = 0x89abcdef
```

FlexFX properties are transfered via over USB/MIDI using MIDI SYSEX messages.
The FlexFX framework handles parsing and rendering of MIDI SYSEX encapulated FlexFX data therefore the user
application need not deal with MIDi SYSEX - the audio firmware only sees 16-bit ID and five 32-word propertie.

For detailed information regarding the encapsulation of FlexFX properties within MIDI SYSEX see the 'flexfx.py' script
that's used to send/receive properties to FlexFX applications via USB.

FlexFX supports the predefined properties with the 16-bit ID being non-zero and less than 0x8000.
User defined properties should therefore use 16-bit ID's greater then or equal to 0x8000.
The predefied properties (0x1000 <= ID <= 0x1FFF) are all handled automatically by the FlexFX framework whereas
properties 0x2pxx are forwarded to the application control task ('app_control').

```
ID        DIRECTION        SUMMARY
1000      Bidirectional    Identify; return ID (3DEGFLEX) and versions
1100      Bidirectional    Return volume,tone,preset,bypass settings
120t      Bidirectional    Return tile T's DSP processing loads
13nn      Bidirectional    Read line NN (20 bytes) of GUI interface text
1401      Bidirectional    Begin firmware upgrade, echoed back to host
1402      Bidirectional    Next 32 bytes of firmware image data, echoed
1403      Host to Device   End firmware upgrade and reset
1501      Bidirectional    Begin data upload, echoed back to host
1502      Bidirectional    Next 32 bytes of bulk data, echoed
1601      Bidirectional    Begin data download, echoed back to host
1602      Bidirectional    Next 32 bytes of bulk data
1603      Device to host   End data download
1701      Device to Host   Send raw MIDI data from device to host
1702      Host to Device   Send raw MIDI data from host to device
1703      Internal         MIDI beat clock control (start/stop/setBPM)
1704      Internal         MIDI MTC/MPC control (MPC commands,MTC settings)
2pxx      Bidirectional    User/app props (preset=P,ID=xx) compatible with 'flexfx.html'
3xxx      Undefined        Reserved
4xxx      Undefined        Reserved
5xxx      Undefined        Reserved
6xxx      Undefined        Reserved
7xxx      Undefined        Reserved
8xxx      Undefined        User/application defined
9xxx      Undefined        User/application defined
Axxx      Undefined        User/application defined
Bxxx      Undefined        User/application defined
Cxxx      Undefined        User/application defined
Dxxx      Undefined        User/application defined
Exxx      Undefined        User/application defined
Fxxx      Undefined        User/application defined
```

#### FlexFX ID = 0x1000
#### FlexFX ID = 0x1100
#### FlexFX ID = 0x120t

#### FlexFX ID = 0x13nn

#### FlexFX ID = 0x1401
#### FlexFX ID = 0x1402
#### FlexFX ID = 0x1403
#### FlexFX ID = 0x1501
#### FlexFX ID = 0x1502
#### FlexFX ID = 0x1503
#### FlexFX ID = 0x1601
#### FlexFX ID = 0x1602
#### FlexFX ID = 0x1603
#### FlexFX ID = 0x1701
#### FlexFX ID = 0x1702
#### FlexFX ID = 0x1703
#### FlexFX ID = 0x1704

#### FlexFX ID = 0x2xxx

HTML Interfaces
----------------------------

Google Chrome with USB/MIDI support in conjunction with 'flexfx.html' allows any FlexFX application to have a custiom HTML5-based user interface for effects control.

If an appliction does not have a user interface it still must provide a minimal interface description in order for the firmware upgrade capability via 'flexfx.html' to function:

```
const char controller_script[] =  "ui_header(ID:0x00,'FlexFX',[]);";
```

Example #1 of a FlexFX user interface definition. See the 'EFX_CABSIM' screenshot in the 'Prebuild Effects' section for a screenshot of this HTML5 interface.
```
const char controller_script[] = \
	"" \
	"ui_header( ID:0x00, 'FlexFX Cabsim', [' Left/Mono ',' Right/Stereo ',' IR File Name(s) '] );" \
	"ui_file  ( ID:0x01, 'N', 1, 'Select IR', ID:03 );" \
	"ui_file  ( ID:0x02, 'N', 2, 'Select IR', ID:04 );" \
	"ui_label ( ID:0x03, 'F', 'Celestion G12H Ann 152 Open Room.wav' );" \
	"ui_label ( ID:0x04, 'L', 'Celestion G12H Ann 152 Open Room.wav' );" \
	"";
```

Example #1 of a FlexFX user interface definition. See the 'EFX_PREAMP' screenshot in the 'Prebuild Effects' section for a screenshot of this HTML5 interface.
```
const char controller_script[] = \
	"" \
	"ui_header ( ID:0x00, 'FlexFX Preamp', ['Stage','Low-Cut','Emphasis','Bias','High-Cut'] );" \
	"ui_label  ( ID:0x00, 'F', '1' );" /* First label  in column 1 ("Stage" column) */ \
	"ui_label  ( ID:0x00, 'N', '2' );" /* Next  label  in column 1 ("Stage" column) */ \
	"ui_label  ( ID:0x00, 'L', '3' );" /* Last  label  in column 1 ("Stage" column) */ \
	"ui_hslider( ID:0x01, 'F', 96 );"  /* First slider in column 2 ("Low-Cut" column) */ \
	"ui_hslider( ID:0x02, 'N', 96 );"  /* Next  slider in column 2 ("Low-Cut" column) */ \
	"ui_hslider( ID:0x03, 'L', 96 );"  /* Last  slider in column 2 ("Low-Cut" column) */ \
	"ui_hslider( ID:0x04, 'F', 96 );"  /* First slider in column 2 ("Emphasis" column) */ \
	"ui_hslider( ID:0x05, 'N', 96 );"  /* Next  slider in column 2 ("Emphasis" column) */ \
	"ui_hslider( ID:0x06, 'L', 96 );"  /* Last  slider in column 2 ("Emphasis" column) */ \
	"ui_hslider( ID:0x07, 'F', 96 );"  /* First slider in column 2 ("Bias" column) */ \
	"ui_hslider( ID:0x08, 'N', 96 );"  /* Next  slider in column 2 ("Bias" column) */ \
	"ui_hslider( ID:0x09, 'L', 96 );"  /* Last  slider in column 2 ("Bias" column) */ \
	"ui_hslider( ID:0x0A, 'F', 96 );"  /* First slider in column 2 ("High-Cut" column) */ \
	"ui_hslider( ID:0x0B, 'N', 96 );"  /* Next  slider in column 2 ("High-Cut" column) */ \
	"ui_hslider( ID:0x0C, 'L', 96 );"  /* Last  slider in column 2 ("High-Cut" column) */ \
	"";
```

Example #1 of a FlexFX user interface definition. See the 'EFX_GRAPHICEQ' screenshot in the 'Prebuild Effects' section for a screenshot of this HTML5 interface.
```
const char controller_script[] = \
	"" \
	"ui_header ( ID:0x00, 'FlexFX GraphicEQ', " \
	             "['Gain','56','84','126','190','284','426','640','960'," \
	             "'1K4','2K1','3K2','4K8','7K3','11K','16K'] );" \
	"ui_vslider( ID:0x10, 'S', 27 );" "ui_vslider( ID:0x11, 'S', 27 );" \
	"ui_vslider( ID:0x12, 'S', 27 );" "ui_vslider( ID:0x13, 'S', 27 );" \
	"ui_vslider( ID:0x14, 'S', 27 );" "ui_vslider( ID:0x15, 'S', 27 );" \
	"ui_vslider( ID:0x16, 'S', 27 );" "ui_vslider( ID:0x17, 'S', 27 );" \
	"ui_vslider( ID:0x18, 'S', 27 );" "ui_vslider( ID:0x19, 'S', 27 );" \
	"ui_vslider( ID:0x1A, 'S', 27 );" "ui_vslider( ID:0x1B, 'S', 27 );" \
	"ui_vslider( ID:0x1C, 'S', 27 );" "ui_vslider( ID:0x1D, 'S', 27 );" \
	"ui_vslider( ID:0x1E, 'S', 27 );" "ui_vslider( ID:0x1F, 'S', 27 );" \
	"";
```

**Control Object "ui_header"**
**Control Object "ui_label"**
**Control Object "ui_hslider"r**
**Control Object "ui_vslider"**
**Control Object "ui_file"**

Programming Tools
-------------------------------------

The Python script 'flexfx.py' implements various host functions for testing FlexFX firmware via USB.

#### Usage #1
Display attached USB MIDI devices.  The FlexFX DSP board enumerates as device #0 in this example.
```
bash$ python flexfx.py
MIDI Output Devices: 0='FlexFX'
MIDI Input Devices:  0='FlexFX'
```

#### Usage #2
Indefinitely display properties being sent from the DSP board, enumerated as USB MIDI device #0, to the USB host (CRTL-C to terminate).  The first six columns are the 32-bit property ID and five property values printed in hex/ASCII.  The last five columns are the same five property values converted from Q28 fixed-point to floating point.  These rows are printed at a very high rate - as fast as Python can obtain the USB data over MIDI and print to the console. 
```
bash$ python flexfx.py 0
11111111  0478ee7b 08f1dcf7 0478ee7b 00dcd765 fd3f6eac  +0.27952 +0.55905 +0.27952 +0.05392 -0.17201
11111111  0472eb5b 08e5d6b6 0472eb5b 00f5625e fd3ef034  +0.27806 +0.55611 +0.27806 +0.05991 -0.17213
11111111  0472eb5b 08e5d6b6 0472eb5b 00f5625e fd3ef034  +0.27806 +0.55611 +0.27806 +0.05991 -0.17213
11111111  0478ee7b 08f1dcf7 0478ee7b 00dcd765 fd3f6eac  +0.27952 +0.55905 +0.27952 +0.05392 -0.17201
...
```
This video shows the 'flexfx.py' script receiving properties from the DSP board and printing them to the console.
For this example FlexFX firmware is capturing four potentiometer values, packaging them up into a property,
and sending the property to the USB host (see code below).
One of the pots is being turned back and fourth resulting in changes to the corresponding property value.
https://raw.githubusercontent.com/markseel/flexfx_kit/master/flexfx.py.usage2.mp4
```
static void adc_read( double values[4] )
{
    byte ii, hi, lo, value;
    i2c_start( 100000 ); // Set bit clock to 400 kHz and assert start condition.
    i2c_write( 0x52+1 ); // Select I2C peripheral for Read.
    for( ii = 0; ii < 4; ++ii ) {
        hi = i2c_read(); i2c_ack(0); // Read low byte, assert ACK.
        lo = i2c_read(); i2c_ack(ii==3); // Read high byte, assert ACK (NACK on last read).
        value = (hi<<4) + (lo>>4); // Select correct value and store ADC sample.
        values[hi>>4] = ((double)value)/256.0; // Convert from byte to double (0<=val<1.0).
    }
    i2c_stop();
}
void control( int rcv_prop[6], int usb_prop[6], int dsp_prop[6] )
{
    // If outgoing USB or DSP properties are still use then come back later ...
    if( usb_prop[0] != 0 || usb_prop[0] != 0 ) return;
    // Read the potentiometers -- convert the values from float to Q28 using the FQ macro.
    double values[4]; adc_read( values );
    // Create property with three Q28 pot values to send to USB host
    usb_prop[0] = 0x01010000; dsp_prop[4] = dsp_prop[5] = 0;
    usb_prop[1] = FQ(values[0]); usb_prop[2] = FQ(values[1]); usb_prop[3] = FQ(values[2]);
}
```

#### Usage #3
Burn a custom firmware application to the DSP board's FLASH memory (board is enumerated as MIDI device #0).  This takes about 10 seconds.
```
bash$ python flexfx.py 0 app_cabsim.bin
.....................................................................................Done.
```

#### Usage #4
Load impulse response data, stored in a WAV file, to the cabinet simulator example running on the DSP board that is enumerated as MIDI device #0.  This takes less than one second.
```
bash$ python flexfx.py 0 your_IR_file.wav
Done.
```

Design Tools
--------------------------------

There are four Python scripts to aid with algorithm and filter design.  The FIR and IIR design scripts require that the Python packeges 'scipy' and 'matplotlib' are installed.

#### FIR Filter Design Script

```
bash-3.2$ python util_fir.py
Usage: python util_fir <pass_freq> <stop_freq> <ripple> <attenuation>
       <pass_freq> is the passband frequency relative to Fs (0 <= freq < 0.5)
       <stop_freq> is the stopband frequency relative to Fs (0 <= freq < 0.5)
       <ripple> is the maximum passband ripple
       <attenuation> is the stopband attenuation
```

'util_fir.py' is used to generate filter coefficients and to plot the filter response.  These coefficients can be used directly in your custom application 'C' file since the FQ macro converts floating point to Q28 fixed point format required by FlexFX DSP functions.  For this example the filter passband frequency is 0.2 (e.g. 9.6kHz @ 48 kHz Fs), a stopband frequency of 0.3 (e.g. 14.4 kHz @ 48 kHz Fs), a maximum passband ripple of 1.0 dB, and a stopband attenuation of -60 dB.  This script will display the magnitude response and the impulse response, and then print out the 38 filter coefficients for the resulting 38-tap FIR filter.

```
bash-3.2$ python util_fir.py 0.2 0.3 1.0 -60       
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
bash$ python util_iir.py
Usage: python design.py <type> <freq> <Q> <gain>
       <type> filter type (notch, lowpass, highpass, allpass, bandpass, peaking, highshelf, or lowshelf
       <freq> is cutoff frequency relative to Fs (0 <= freq < 0.5)
       <Q> is the filter Q-factor
       <gain> is the filter positive or negative gain in dB
```

'util_iir.py' is used to generate biquad IIR filter coefficients (B0, B1, B2, A1, and A2) and to plot the filter response.  These coefficients can be used directly in your custom application 'C' file since the FQ macro converts floating point to Q28 fixed point format required by FlexFX DSP functions.  For this example a bandpass filter and a peaking filter were designed.  This script will display the magnitude and phase responses and then print out the 38 filter coefficients for the resulting 38-tap FIR filter.

```
bash$ python util_iir.py bandpass 0.25 0.707 12.0
int coeffs[5] = {FQ(+0.399564099),FQ(+0.000000000),FQ(-0.399564099),FQ(-0.000000000),FQ(+0.598256395)};

bash-3.2$ python util_iir.py peaking 0.2 2.5 12.0       
int coeffs[5] = {FQ(+1.259455676),FQ(-0.564243794),FQ(+0.566475599),FQ(-0.564243794),FQ(+0.825931275)};
```
![alt tag](https://raw.githubusercontent.com/markseel/flexfx_kit/master/util_iir.png)

#### WAVE File Parsing Script

'util_wave.py' is used to parse one or more WAVE files and print the WAVE file sample values to the console in floating point format.

```
bash$ python util_wave.py ir1.wav ir2.wav
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
bash$ python util_plot.py
Usage: python plot.py <datafile> time
       python plot.py <datafile> time [beg end]
       python plot.py <datafile> freq lin
       python plot.py <datafile> freq log
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

The 'util_plot.py' script graphs data contained in a data file which can contan an arbitrary number of data columns of floating point values.  The data can be plotted in the time domain or in the frequency domain.  In this example two cabinet simulation data files are plotted to show their frequency domain impulse responses (i.e the cabinet frequency responses).

```
bash$ python util_wave.py ir1.wav ir2.wav > output.txt
bash$ python util_plot.py output.txt freq log
bash$ python util_plot.py output.txt time 0 150
```
![alt tag](https://raw.githubusercontent.com/markseel/flexfx_kit/master/util_plot.png)

Prebuilt Effects
----------------------------------

The FlexFX kit contains some highly optimized effects. These effects are in binary (\*.bin) form and can be used for free on FlexFX boards. The FlexFX properties definitions for prebuilt effects are documented in the effect's respective text file (e.g. efx_cabsim.txt for the prebuild efx_cabsim.bin effect). These properties allow for full control over each effect via FlexFX properties sent and received over USB/MIDI.

The effects also supports the HTML5 interface for controlling the device (firmware upgrading, uploading IR data, etc) since the web interfaces uses FlexFX properties and USB/MIDI for control. Javascript code for an effect is returned via USB MIDI by issueing the FlexFX USB/MIDI property for returning a device's javascript controller interface.  The HTML5 application called 'flexfx.html' does this automatically and will displayt this device's GUI interface if the device is pluged into the host computer via a USB cable. Google Chrome must be used.

**Prebuilt Effect - Cabinet Simulation (EFX_CABSIM)**

Stereo Cabinet Simulation with Tone/Volume and USB Audio Mixing.  30 msec of impulse response (IR) convolution in stereo mode and 70 msec in mono mode using 32/64 bit fixed-point DSP at a 48 kHz sampling rate.  Supports up to nine presets each with its own set if IR's (left and right channels) which can be downloaded as WAVE files via USB/MIDI using the FlexFX development kit, the flexfx browser interface (Google Chrome only), or other applications conforming to the FlexFX USB/MIDI download process.

![alt tag](efx_cabsim.png)

**Prebuilt Effect - Preamp/Overdrive (EFX_PREAMP)**

Three preamp gain stages in series for with an internal signal processing sample rate of 960 kHz for articulate overdrive/distortion voicing.  Each preamp stage incorporates adjustable pre-filtering, a dynamic tube-based gain model with slew-rate limiting and adjustable/modulated bias, and post-filtering creating a tube-like multi-stage guitar preamp.

![alt tag](efx_preamp.png)

**Prebuilt Effect - Graphic Equalizer (EFX_GRAPHICEQ)**

![alt tag](efx_graphiceq.png)

**Prebuilt Effect - Stereo Multi-Voice Chorus (EFX_CHORUS)**

Up to three chorus voices per channel (left and right) each with their own settings for LFO rate, base delay, modulated delay/depth,  high and low-pass filters, regeneration/feedback level, and wet/dry mix. Up to nine presets and USB/MIDI control.

![alt tag](efx_chorus.png)

**Prebuilt Effect - Stereo 'FreeVerb' Reverb (EFX_REVERB)**

Implements the Schroeder-Moorer approach to reverberation and this particular implementation is a port of the 'FreeVerb' algorithm that's used in a number of software packages.  Adjustments for the reverb's wet/dry mix, stereo width, room size, and room reflection/damping. Up to nine presets and USB/MIDI control.

![alt tag](efx_chorus.png)

Reading Pots/Knobs
--------------------------------------

```C
#include "flexfx.h" // Defines config variables, I2C and GPIO functions, DSP functions, etc.
#include <math.h>   // Floating point for filter coeff calculations in the background process.

const char* product_name_string    = "Example"; // Your company/product name
const int   audio_sample_rate      = 48000;     // Audio sampling frequency
const int   usb_output_chan_count  = 2;         // 2 USB audio class 2.0 output channels
const int   usb_input_chan_count   = 2;         // 2 USB audio class 2.0 input channels
const int   i2s_channel_count      = 2;         // ADC/DAC channels per SDIN/SDOUT wire
const char  interface_text[]       = "No interface is specified";
const char  controller_app[]       = "No controller is available";

const int   i2s_sync_word[8] = { 0xFFFFFFFF,0x00000000,0,0,0,0,0,0 }; // I2S WCLK values per slot

#define PROP_PRODUCT_ID      0x0101                      // Your product ID, must not be 0x0000!
#define PROP_EXAMPLE_PROPS   (PROP_PRODUCT_ID << 16)     // Base property ID value
#define PROP_EXAMPLE_VOL_MIX (PROP_EXAMPLE_PROPS+0x0001) // prop[1:5] = [volume,blend,0,0,0]
#define PROP_EXAMPLE_TONE    (PROP_EXAMPLE_PROPS+0x0002) // Coeffs, prop[1:5]=[B0,B1,B2,A1,A2]
#define PROP_EXAMPLE_IRDATA  (PROP_EXAMPLE_PROPS+0x8000) // 5 IR samples values per property

#define IR_PROP_ID(xx)  ((xx) & 0xFFFF8000) // IR property ID has two parts (ID and offset)
#define IR_PROP_IDX(xx) ((xx) & 0x00000FFF) // Up to 5*0xFFF samples (5 samples per property)

static void adc_read( double values[4] ) // I2C controlled 4-channel ADC (0.0 <= value < 1.0).
{
    byte ii, hi, lo, value;
    i2c_start( 100000 ); // Set bit clock to 400 kHz and assert start condition.
    i2c_write( 0x52+1 ); // Select I2C peripheral for Read.
    for( ii = 0; ii < 4; ++ii ) {
        hi = i2c_read(); i2c_ack(0); // Read low byte, assert ACK.
        lo = i2c_read(); i2c_ack(ii==3); // Read high byte, assert ACK (NACK on last read).
        value = (hi<<4) + (lo>>4); // Select correct value and store ADC sample.
        values[hi>>4] = ((double)value)/256.0; // Convert from byte to double (0<=val<1.0).
    }
    i2c_stop();
}

void make_prop( int prop[6], int id, int val1, int val2, int val3, int val4, int val5 )
{
    prop[0]=id; prop[1]=val1; prop[2]=val2; prop[3]=val3; prop[4]=val4; prop[5]=val5;
}
void copy_prop( int dst[6], const int src[6] )
{
    dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2]; dst[3]=src[3]; dst[4]=src[4]; dst[5]=src[5];
}
void copy_data( int dst[5], const int src[5] )
{
    dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2]; dst[3]=src[3]; dst[4]=src[4];
}

void app control( int rcv_prop[6], int usb_prop[6], int dsp_prop[6] )
{
    static bool intialized = 0, state = 0;

    // Initialize CODEC/ADC/DAC and other I2C-based peripherals here.
    if( !intialized ) { intialized = 1; }

    // If outgoing USB or DSP properties are still use then come back later ...
    if( usb_prop[0] != 0 || usb_prop[0] != 0 ) return;

    // Pass cabsim IR data to the DSP threads if usb and dsp send properties are available for use.
    if( IR_PROP_ID(rcv_prop[0]) == IR_PROP_ID(PROP_EXAMPLE_IRDATA) )
    {
        copy_prop(dsp_prop,rcv_prop); // Send to DSP threads.
        copy_prop(usb_prop,rcv_prop); // Send to USB host as an acknowledge and for flow-control.
        rcv_prop[0] = 0; // Mark the incoming USB property as consumed (allow RX of the next prop).
    }
    else // Generate property for the DSP threads if the prop buffer is free.
    {
        // Read the potentiometers -- Pot[2] is volume, Pot[1] is tone, Pot[0] is blend.
        double pot_values[4]; adc_read( pot_values );
        double volume = pot_values[2], tone = pot_values[1], blend = pot_values[0];

        // Send a CONTROL property containing knob settings (volume and blend) to the mixer thread.
        if( state == 0 ) { // State 1 is for generating and sending the VOLUME/MIX property.
            state = 1;
            dsp_prop[0] = PROP_EXAMPLE_VOL_MIX; dsp_prop[3] = dsp_prop[4] = dsp_prop[5] = 0;
            dsp_prop[1] = FQ(volume); dsp_prop[2] = FQ(blend); // float to Q28.
        }
        // Compute tone (12 dB/octave low-pass) coefficients and send them to DSP threads.
        else if( state == 1 ) { // State 2 is for generating and sending the tone coeffs property.
            state = 0;
            // This lowpass filter -3dB frequency value ranges from 1 kHz to 11 kHz via Pot[2]
            double freq = 2000.0/48000 + tone * 10000.0/48000;
            dsp_prop[0] = PROP_EXAMPLE_TONE;
            make_lowpass( dsp_prop+1, freq, 0.707 ); // Compute coefficients and populate property.
        }
    }
}

void app_mixer( const int[32] usb_output, int[32] usb_input,
                const int[32] i2s_output, int[32] i2s_input,
                const int[32] dsp_output, int[32] dsp_input, const int[6] property )
{
    static int volume = FQ(0.0), blend = FQ(0.5); // Initial volume and blend/mix levels.

    // Convert the two ADC inputs into a single pseudo-differential mono input (mono = L - R).
    // Route the guitar signal to the USB input and to the DSP input.
    //usb_input[0] = usb_input[1] = dsp_input[0] = dsp_input[1] = i2s_output[6] - i2s_output[7];
    usb_input[0] = usb_input[1] = dsp_input[0] = dsp_input[1] = usb_output[0];

    // Check for incoming properties from USB (volume, DSP/USB audio blend, and tone control).
    if( property[0] == PROP_EXAMPLE_VOL_MIX ) { volume = property[1]; blend = property[3]; }
    // Update coeffs B1 and A1 of the low-pass biquad but leave B0=1.0,B2=0,A2=0.
    if( property[0] == PROP_EXAMPLE_TONE ) { copy_data( lopass_coeffs, &(property[2]) ); }

    // Apply tone filter to left and right DSP outputs and route this to the I2S driver inputs.
    i2s_input[6] = dsp_iir_filt( dsp_output[0], lopass_coeffs, lopass_stateL, 1 );
    i2s_input[7] = dsp_iir_filt( dsp_output[1], lopass_coeffs, lopass_stateR, 1 );

    // Blend/mix USB output audio with the processed guitar audio from the DSP output.
    i2s_input[6] = dsp_multiply(blend,i2s_input[6])/2 + dsp_multiply(FQ(1)-blend,usb_output[0])/2;
    i2s_input[7] = dsp_multiply(blend,i2s_input[7])/2 + dsp_multiply(FQ(1)-blend,usb_output[1])/2;

    // Apply master volume to the I2S driver inputs (i.e. to the data going to the audio DACs).
    i2s_input[6] = dsp_multiply( i2s_input[6], volume );
    i2s_input[7] = dsp_multiply( i2s_input[7], volume );

    // Bypass DSP processing (pass ADC samples to the DAC) if a switch on I/O port #0 is pressed.
    if( port_read(1) == 0 ) { i2s_input[0] = i2s_output[0]; i2s_input[1] = i2s_output[1]; }
    // Light an LED attached to I/O port #2 if the bypass switch on I/O port #1 is pressed.
    port_write( 2, port_read(1) != 0 );
}

```

Cabsim Example
----------------------------------

Stereo Cabinet Simulation with Tone/Volume and USB Audio Mixing. See this video for a demonstration. The firmware is first
written to FLASH memory using the 'flexfx.py' script. After that the firmware reboots and enumerates as a USB audio device
resulting in audio. The first few chords are sent from the guitar ADC to the line out DAC unprocessed. The 'flexfx.py' script
is then used to load an IR file called 'ir1.wav'and then used to load another IR file called 'ir2.wav'.
The firmware is performing 25 msec of IR convolution (at a 48 kHz audio sample rate) on both left and right audio channels
using 32/64 bit fixed-point DSP.
https://raw.githubusercontent.com/markseel/flexfx_kit/master/app_cabsim.mp4

```C
#include "flexfx.h"

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

const char controller_script[] =  "ui_header(ID:0x00,'FlexFX',[]);";

void copy_prop( int dst[6], const int src[6] )
{
    dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2]; dst[3]=src[3]; dst[4]=src[4]; dst[5]=src[5];
}

void app_control( const int rcv_prop[6], int snd_prop[6], int dsp_prop[6] )
{
    // Pass cabsim IR data to the DSP threads if usb and dsp send properties are available for use.
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
    // Route DSP result to the left/right USB inputs.
    usb_input[1] = i2s_input[0] = dsp_output[0] * 8; // Q28 (DSP) to Q31 (USB/I2S)
    // Route DSP result added to USB outputs to the audio DAC.
    i2s_input[0] = (dsp_output[0]*8)/2 + usb_output[0]/2; // Q28 (DSP) to Q31 (USB/I2S)
    i2s_input[1] = (dsp_output[0]*8)/2 + usb_output[1]/2; // Q28 (DSP) to Q31 (USB/I2S)
}

int ir_coeff[2400], ir_state[2400]; // DSP data *must* be non-static global!

void app_thread1( int samples[32], const int property[6] )
{
    static int first = 1, offset = 0, muted = 0;
    if( first ) { first = 0; ir_coeff[0] = ir_coeff[1200] = FQ(+1.0); }
    // Check for properties containing new cabsim IR data, save new data to RAM
    if( property[0] == 0x1501 ) { offset = 0; muted = 1; }
    if( offset == 2400-5 ) muted = 0;
    if( property[0] == 0x1502 && offset < 2400-5 ) {
		ir_coeff[offset+0] = property[1] / 32; ir_coeff[offset+1] = property[2] / 32;
		ir_coeff[offset+2] = property[3] / 32; ir_coeff[offset+3] = property[4] / 32;
		ir_coeff[offset+4] = property[5] / 32; offset += 5;
    }
    samples[2] = 0; samples[3] = 1 << 31; // Initial 64-bit Q1.63 accumulator value
    samples[4] = 0; samples[5] = 1 << 31; // Initial 64-bit Q1.63 accumulator value
    // Perform 240-sample convolution (1st 240 of 1220 total) of sample with IR data
    samples[0] = dsp_convolve( samples[0], ir_coeff+240*0, ir_state+240*0, samples+2,samples+3 );
    samples[1] = dsp_convolve( samples[1], ir_coeff+240*5, ir_state+240*5, samples+4,samples+5 );
    samples[0] = muted ? 0 : samples[0];
    samples[1] = muted ? 0 : samples[1];
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
    // Perform 240-sample convolution (5th and last 240 of 1220 total) of sample with IR data
    samples[0] = dsp_convolve( samples[0], ir_coeff+240*4, ir_state+240*4, samples+2,samples+3 );
    samples[1] = dsp_convolve( samples[1], ir_coeff+240*9, ir_state+240*9, samples+4,samples+5 );
    // Extract 32-bit Q28 from 64-bit Q63 and then apply mute/un-mute based on IR loading activity.
    DSP_EXT( samples[0], samples[2], samples[3] );
    DSP_EXT( samples[1], samples[4], samples[5] );
}
```

Chorus Example
----------------------------------

Chorus example with two voices showing how to create LFO's and how to use 2nd order interpolation. See this video for a demonstration of building and loading the firmware exmaple, and the chorus audio effect.
https://raw.githubusercontent.com/markseel/flexfx_kit/master/app_chorus.mp4

```C
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

const char controller_script[] =  "ui_header(ID:0x00,'FlexFX',[]);";

void app_control( const int rcv_prop[6], int snd_prop[6], int dsp_prop[6] )
{
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

void app_initialize( void )
{
}

void app_thread1( int samples[32], const int property[6] )
{
    // Define LFO frequencies
    static int delta1 = FQ(3.3/48000.0); // LFO frequency 3.3 Hz @ 48 kHz
    static int delta2 = FQ(2.7/48000.0); // LFO frequency 2.7 Hz @ 48 kHz
    // Update LFO time: Increment each and limit to 1.0 -- wrap as needed.
    static int time1 = FQ(0.0); time1 += delta1; if(time1 > FQ(1.0)) time1 -= FQ(1.0);
    static int time2 = FQ(0.0); time2 += delta2; if(time2 > FQ(1.0)) time2 -= FQ(1.0);
    // II is index into sine table (0.0 < II < 1.0), FF is the fractional remainder.
    // Use 2nd order interpolation to smooth out lookup values.
    // Index and fraction portion of Q28 sample value: snnniiii,iiiiiiff,ffffffff,ffffffff
    int ii, ff;
    ii = (time1 & 0x0FFFFFFF) >> 18, ff = (time1 & 0x0003FFFF) << 10;
    samples[2] = dsp_lagrange( ff, dsp_sine_10[ii+0], dsp_sine_10[ii+1], dsp_sine_10[ii+2] );
    ii = (time2 & 0x0FFFFFFF) >> 18, ff = (time2 & 0x0003FFFF) << 10;
    samples[3] = dsp_lagrange( ff, dsp_sine_10[ii+0], dsp_sine_10[ii+1], dsp_sine_10[ii+2] );
    // Send LFO values downstream for use by other DSP threads, ensure that they're less than +1.0.
    samples[2] = dsp_multiply( samples[2], FQ(0.999) ); // LFO #1 in sample 2 for later use.
    samples[3] = dsp_multiply( samples[3], FQ(0.999) ); // LFO #2 in sample 3 for later use.
}

void app_thread2( int samples[32], const int property[6] )
{
    // --- Generate wet signal #1 using LFO #1
    static int delay_fifo[1024], delay_index = 0; // Chorus delay line
    static int depth = FQ(+0.10);
    // Scale lfo by chorus depth and convert from [-1.0 < lfo < +1.0] to [+0.0 < lfo < +1.0].
    int lfo = (dsp_multiply( samples[2], depth ) / 2) + FQ(0.4999);
    // Get index and fraction portion of Q28 LFO value: snnniiii,iiiiiiff,ffffffff,ffffffff
    int ii = (lfo & 0x0FFFFFFF) >> 18, ff = (lfo & 0x0003FFFF) << 10;
    delay_fifo[delay_index-- & 1023] = samples[0]; // Update the sample delay line.
    // Get samples from delay -- handle wrapping of index values.
    int i1 = (delay_index+ii)&1023, i2 = (delay_index+ii+1)&1023, i3 = (delay_index+ii+2)&1023;
    // Interpolate and store wet signal #1 for use in another DSP thread below.
    samples[2] = dsp_lagrange( ff, delay_fifo[i1], delay_fifo[i2], delay_fifo[i3] );
}

void app_thread3( int samples[32], const int property[6] )
{
    // --- Generate wet signal #2 using LFO #2
    static int delay_fifo[1024], delay_index = 0; // Chorus delay line
    static int depth = FQ(+0.10);
    // Scale lfo by chorus depth and convert from [-1.0 < lfo < +1.0] to [+0.0 < lfo < +1.0].
    int lfo = (dsp_multiply( samples[3], depth ) / 2) + FQ(0.4999);
    // Get index and fraction portion of Q28 LFO value: snnniiii,iiiiiiff,ffffffff,ffffffff
    int ii = (lfo & 0x0FFFFFFF) >> 18, ff = (lfo & 0x0003FFFF) << 10;
    delay_fifo[delay_index-- & 1023] = samples[0]; // Update the sample delay line.
    // Get samples from delay -- handle wrapping of index values.
    int i1 = (delay_index+ii)&1023, i2 = (delay_index+ii+1)&1023, i3 = (delay_index+ii+2)&1023;
    // Interpolate and store wet signal #1 for use in another DSP thread below.
    samples[3] = dsp_lagrange( ff, delay_fifo[i1], delay_fifo[i2], delay_fifo[i3] );
}

void app_thread4( int samples[32], const int property[6] )
{
    int blend1 = FQ(+0.5), blend2 = FQ(+0.3);
    // Mix dry signal with wet #1 and wet #2 and send to both left and right channels (0 and 1).
    samples[2] = dsp_blend( samples[0], samples[2], blend1 );
    //samples[3] = dsp_blend( samples[0], samples[3], blend2 );
    samples[0] = samples[0]/3 + samples[2]/3 + samples[3]/3;
}

void app_thread5( int samples[32], const int property[6] )
{
}
```

Overdrive Example
-------------------------------------

Overdrive example demonstrating up/down sampling, anti-aliasing filters, and the use of look-up tables and Lagrange interpolation to 
create a simple preamp model. Up/down sampling by a factor of 2 brings the internal sampling rate to 384 kHz to help manage the 
aliasing of harmonics created from the nonlinear behavior of the preamp.

```C
#include "flexfx.h" // Defines config variables, I2C and GPIO functions, etc.
#include <math.h>   // Floating point for filter coeff calculations in the background process.
#include <string.h> // Memory and string functions

const char* product_name_string   = "FlexFX Example";   // Your product name
const char* usb_audio_output_name = "FlexFX Audio Out"; // USB audio output name
const char* usb_audio_input_name  = "FlexFX Audio In";  // USB audio input name
const char* usb_midi_output_name  = "FlexFX MIDI Out";  // USB MIDI output name
const char* usb_midi_input_name   = "FlexFX MIDI In";   // USB MIDI input name

const int audio_sample_rate     = 192000; // Audio sampling frequency
const int usb_output_chan_count = 2;      // 2 USB audio class 2.0 output channels
const int usb_input_chan_count  = 2;      // 2 USB audio class 2.0 input channels
const int i2s_channel_count     = 2;      // 2,4,or 8 I2S channels per SDIN/SDOUT wire
const int i2s_is_bus_master     = 1;      // Set to 1 if FlexFX creates I2S clocks

const int i2s_sync_word[8] = { 0xFFFFFFFF,0x00000000,0,0,0,0,0,0 }; // I2S WCLK values per slot

const char controller_script[] =  "ui_header(ID:0x00,'FlexFX',[]);";

void app_control( const int rcv_prop[6], int snd_prop[6], int dsp_prop[6] )
{
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
    usb_input[1] = i2s_input[0] = i2s_input[1] = dsp_output[0] * 1; // Q28 to Q31
}

// util_fir.py 0.001 0.125 1.0 -100
int antialias_state1[64], antialias_state2[64], antialias_coeff[64] =
{
    FQ(-0.000000077),FQ(-0.000001205),FQ(-0.000005357),FQ(-0.000014901),FQ(-0.000030776),
    FQ(-0.000049283),FQ(-0.000058334),FQ(-0.000035116),FQ(+0.000052091),FQ(+0.000235288),
    FQ(+0.000530261),FQ(+0.000914292),FQ(+0.001304232),FQ(+0.001544675),FQ(+0.001417440),
    FQ(+0.000681095),FQ(-0.000857660),FQ(-0.003249472),FQ(-0.006305108),FQ(-0.009524756),
    FQ(-0.012085259),FQ(-0.012909300),FQ(-0.010823540),FQ(-0.004789122),FQ(+0.005836344),
    FQ(+0.021063799),FQ(+0.040127668),FQ(+0.061481322),FQ(+0.082955733),FQ(+0.102066436),
    FQ(+0.116416104),FQ(+0.124112485),FQ(+0.124112485),FQ(+0.116416104),FQ(+0.102066436),
    FQ(+0.082955733),FQ(+0.061481322),FQ(+0.040127668),FQ(+0.021063799),FQ(+0.005836344),
    FQ(-0.004789122),FQ(-0.010823540),FQ(-0.012909300),FQ(-0.012085259),FQ(-0.009524756),
    FQ(-0.006305108),FQ(-0.003249472),FQ(-0.000857660),FQ(+0.000681095),FQ(+0.001417440),
    FQ(+0.001544675),FQ(+0.001304232),FQ(+0.000914292),FQ(+0.000530261),FQ(+0.000235288),
    FQ(+0.000052091),FQ(-0.000035116),FQ(-0.000058334),FQ(-0.000049283),FQ(-0.000030776),
    FQ(-0.000014901),FQ(-0.000005357),FQ(-0.000001205),FQ(-0.000000077)
};

// util_iir.py highpass 0.0004 0.707 0, util_iir.py peaking 0.002 0.707 +6.0
int emphasis1_state[8] = {0,0,0,0,0,0,0,0}, emphasis1_coeff[10] =
{
    FQ(+0.998224158),FQ(-1.996448315),FQ(+0.998224158),FQ(+1.996445162),FQ(-0.996451468),
    FQ(+1.006222470),FQ(-1.987338895),FQ(+0.981273349),FQ(+1.987338895),FQ(-0.987495819),
};
// util_iir.py highpass 0.0002 0.707 0, util_iir.py peaking 0.002 0.707 +6.0
int emphasis2_state[8] = {0,0,0,0,0,0,0,0}, emphasis2_coeff[10] =
{
    FQ(+1.006222470),FQ(-1.987338895),FQ(+0.981273349),FQ(+1.987338895),FQ(-0.987495819),
    FQ(+1.006222470),FQ(-1.987338895),FQ(+0.981273349),FQ(+1.987338895),FQ(-0.987495819),
};
// util_iir.py highpass 0.0001 0.707 0, util_iir.py peaking 0.002 0.707 +6.0
int emphasis3_state[8] = {0,0,0,0,0,0,0,0}, emphasis3_coeff[10] =
{
    FQ(+1.003121053),FQ(-1.993688826),FQ(+0.990607128),FQ(+1.993688826),FQ(-0.993728181),
    FQ(+1.006222470),FQ(-1.987338895),FQ(+0.981273349),FQ(+1.987338895),FQ(-0.987495819),
};

// util_iir.py lowpass 0.09 0.707 0
int lowpass1_state[4] = {0,0,0,0}, lowpass1_coeff[5] =
{
    FQ(+0.056446120),FQ(+0.112892239),FQ(+0.056446120),FQ(+1.224600759),FQ(-0.450385238),
};
// util_iir.py lowpass 0.08 0.707 0
int lowpass2_state[4] = {0,0,0,0}, lowpass2_coeff[5] =
{
    FQ(+0.046130032),FQ(+0.092260064),FQ(+0.046130032),FQ(+1.307234861),FQ(-0.491754988),
};
// util_iir.py lowpass 0.07 0.707 0
int lowpass3_state[4] = {0,0,0,0}, lowpass3_coeff[5] =
{
    FQ(+0.036573558),FQ(+0.073147115),FQ(+0.036573558),FQ(+1.390846672),FQ(-0.537140902)
};

// Simple preamp model (-1.0 <= output < +1.0)
// Apply gain factor. Lookup the preamp transfer function and interpolate to smooth out the lookup
// result. Apply slew-rate limiting to the output.

int preamp1[4+3], preamp2[4+3], preamp3[4+3];

int preamp_model( int xx, int gain, int bias, int slewlim, int* state )
{
    // Add bias to input signal and apply additional gain (total preamp gain = 8 * gain)
    xx = dsp_multiply( xx + bias, gain );
    // Table lookup
    if( xx >= 0 ) { // sIIIiiii,iiiiiiii,iiffffff,ffffffff
        int ii = (xx & 0xFFFFC000) >> 14, ff = (xx & 0x00003FFF) << 14;
        if( ii > 16381 ) ii = 16381;
        xx = dsp_lagrange( ff, dsp_tanh_14[ii+0], dsp_tanh_14[ii+1], dsp_tanh_14[ii+2] );
    } else {
        int ii = (-xx & 0xFFFFC000) >> 14, ff = (-xx & 0x00003FFF) << 14;
        if( ii > 16381 ) ii = 16381;
        xx = -dsp_lagrange( ff, dsp_nexp_14[ii+0], dsp_nexp_14[ii+1], dsp_nexp_14[ii+2] );
    }
    // Slew rate limit and invert
    if( xx > state[6] + slewlim ) xx = state[6] + slewlim;
    if( xx < state[6] - slewlim ) xx = state[6] - slewlim;
    state[6] = xx;
    return -xx;
}

void app_initialize( void ) // Called once upon boot-up.
{
    memset( antialias_state1, 0, sizeof(antialias_state1) );
    memset( antialias_state2, 0, sizeof(antialias_state2) );
}

void app_thread1( int samples[32], const int property[6] ) // Upsample
{
    // Up-sample by 2x by inserting zeros then apply the anti-aliasing filter
    samples[0] = 1 * dsp_fir( samples[0], antialias_coeff, antialias_state1, 64 );
    samples[1] = 1 * dsp_fir( 0,              antialias_coeff, antialias_state1, 64 );
}

void app_thread2( int samples[32], const int property[6] ) // Preamp stage 1
{
    // Perform stage 1 overdrive on the two up-sampled samples for the left channel.
    samples[0] = dsp_iir     ( samples[0], emphasis1_coeff, emphasis1_state, 2 );
    samples[0] = preamp_model( samples[0], FQ(0.7), FQ(+0.0), FQ(0.4), preamp1 );
    samples[0] = dsp_iir     ( samples[0], lowpass1_coeff, lowpass1_state, 1 );
    samples[1] = dsp_iir     ( samples[1], emphasis1_coeff, emphasis1_state, 2 );
    samples[1] = preamp_model( samples[1], FQ(0.7), FQ(+0.0), FQ(0.4), preamp1 );
    samples[1] = dsp_iir     ( samples[1], lowpass1_coeff, lowpass1_state, 1 );
}

void app_thread3( int samples[32], const int property[6] ) // Preamp stage 2
{
    // Perform stage 2 overdrive on the two up-sampled samples for the left channel.
    samples[0] = dsp_iir     ( samples[0], emphasis2_coeff, emphasis2_state, 2 );
    samples[0] = preamp_model( samples[0], FQ(0.5), FQ(+0.0), FQ(0.3), preamp2 );
    samples[0] = dsp_iir     ( samples[0], lowpass2_coeff, lowpass2_state, 1 );
    samples[1] = dsp_iir     ( samples[1], emphasis2_coeff, emphasis2_state, 2 );
    samples[1] = preamp_model( samples[1], FQ(0.5), FQ(+0.0), FQ(0.3), preamp2 );
    samples[1] = dsp_iir     ( samples[1], lowpass2_coeff, lowpass2_state, 1 );
}

void app_thread4( int samples[32], const int property[6] ) // Preamp stage 3
{
    // Perform stage 3 overdrive on the two up-sampled samples for the left channel.
    samples[0] = dsp_iir     ( samples[0], emphasis3_coeff, emphasis3_state, 2 );
    samples[0] = preamp_model( samples[0], FQ(0.3), FQ(+0.0), FQ(0.2), preamp3 );
    samples[0] = dsp_iir     ( samples[0], lowpass3_coeff, lowpass3_state, 1 );
    samples[1] = dsp_iir     ( samples[1], emphasis3_coeff, emphasis3_state, 2 );
    samples[1] = preamp_model( samples[1], FQ(0.3), FQ(+0.0), FQ(0.2), preamp3 );
    samples[1] = dsp_iir     ( samples[1], lowpass3_coeff, lowpass3_state, 1 );
}

void app_thread5( int samples[32], const int property[6] ) // Downsample
{
    // Down-sample by 2x by band-limiting via anti-aliasing filter and then discarding 1 sample.
    samples[0] = dsp_fir( samples[0], antialias_coeff, antialias_state2, 64 );
                 dsp_fir( samples[1], antialias_coeff, antialias_state2, 64 );
}
```

Reverb Example
----------------------------------

Reverb example implemting a port of the FreeVerb algorithm based on the Schroeder-Moorer reverberation model.  A nice improvement would be to use a potentiometer (sensed via an ADC attached via I2C in the 'control' function) to select the reverb type.  The potentiometer code would perhaps generate a value of 1 through 9 depending on the pot's rotation setting.  The value would then be used to select the reverb's wet/dry mix, stereo width, room size, and room reflection/damping.  Note that there's plenty of processing power left over to implement other algorithms along with this reverb such as graphic EQ's, flanger/chorus, etc.

```C
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

const char controller_script[] =  "ui_header(ID:0x00,'FlexFX',[]);";

void app_control( const int rcv_prop[6], int snd_prop[6], int dsp_prop[6] )
{
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

int _comb_bufferL   [8][2048], _comb_bufferR   [8][2048]; // Delay lines for comb filters
int _comb_stateL    [8],       _comb_stateR    [8];       // Comb filter state (previous value)
int _allpass_bufferL[4][1024], _allpass_bufferR[4][1024]; // Delay lines for allpass filters

int _allpass_feedbk    = FQ(0.5); // Reflection decay/dispersion
int _stereo_spread     = 23;      // Buffer index spread for stereo separation
int _comb_delays   [8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 }; // From FreeVerb
int _allpass_delays[8] = { 556, 441, 341, 225 }; // From FreeVerb

int _wet_dry_blend  = FQ(0.2); // Parameter: Wet/dry mix setting (0.0=dry)
int _stereo_width   = FQ(0.2); // Parameter: Stereo width setting
int _comb_damping   = FQ(0.2); // Parameter: Reflection damping factor (aka 'reflectivity')
int _comb_feedbk    = FQ(0.2); // Parameter: Reflection feedback ratio (aka 'room size')

void app_initialize( void ) // Called once upon boot-up.
{
    memset( _comb_bufferL, 0, sizeof(_comb_bufferL) );
    memset( _comb_stateL,  0, sizeof(_comb_stateL) );
    memset( _comb_bufferR, 0, sizeof(_comb_bufferR) );
    memset( _comb_stateR,  0, sizeof(_comb_stateR) );
}

inline int _comb_filterL( int xx, int ii, int nn ) // yy[k] = xx[k] + g1*xx[k-M1] - g2*yy[k-M2]
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

void app_thread1( int samples[32], const int property[6] )
{
    // ----- Left channel reverb
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
    // ----- Right channel reverb
    static int index = 0; ++index; // Used to index into the sample FIFO delay buffer
    // Eight parallel comb filters ...
    samples[1] = _comb_filterR( samples[0]/8, index, 0 ) + _comb_filterR( samples[0]/8, index, 1 )
               + _comb_filterR( samples[0]/8, index, 2 ) + _comb_filterR( samples[0]/8, index, 3 )
               + _comb_filterR( samples[0]/8, index, 4 ) + _comb_filterR( samples[0]/8, index, 5 )
               + _comb_filterR( samples[0]/8, index, 6 ) + _comb_filterR( samples[0]/8, index, 7 );
    // Four series all-pass filters ...
    samples[3] = _allpass_filterR( samples[3], index, 0 );
    samples[3] = _allpass_filterR( samples[3], index, 1 );
    samples[3] = _allpass_filterR( samples[3], index, 2 );
    samples[3] = _allpass_filterR( samples[3], index, 3 );
}

void app_thread3( int samples[32], const int property[6] )
{
    // Final mixing and stereo synthesis
    int dry = _wet_dry_blend, wet = FQ(1.0) - _wet_dry_blend;
    // Coefficients for stereo separation
    int wet1 = _stereo_width / 2 + 0.5;
    int wet2 = (FQ(1.0) - _stereo_width) / 2;
    // Final mixing and stereo separation for left channel
    samples[0] = dsp_multiply( dry, samples[0] )
               + dsp_multiply( wet, dsp_multiply( samples[2], wet1 ) +
                                    dsp_multiply( samples[3], wet2 ) );
    // Final mixing and stereo separation for right channel
    samples[1] = dsp_multiply( dry, samples[1] )
               + dsp_multiply( wet, dsp_multiply( samples[2], wet2 ) +
                                    dsp_multiply( samples[3], wet1 ) );
}

void app_thread4( int samples[32], const int property[6] )
{
}

void app_thread5( int samples[32], const int property[6] )
{
}
```
