// To build, FLASH, etc using XMOS tools version 14.3 (download from www.xmos.com) ...
//
// Download kit:  git clone https://github.com/markseel/flexfx_kit.git
// Build source:  xcc -report -O3 -lquadflash flexfx.xn flexfx.a efx_cabsim.c efx_cabsim.c.o -o app_cabsim.xe
// Create binary: xflash --noinq --no-compression --factory-version 14.3 --upgrade 1 efx_cabsim.xe -o efx_cabsim.bin
// Burn via XTAG: xflash --no-compression --factory-version 14.3 --upgrade 1 efx_cabsim.xe
// Burn via USB:  python flexfx.py 0 efx_cabsim.bin  (Note: FlexFX must be the only USB device attached)
//       .. or :  Run 'flexfx.html' in Google Chrome, press "Upload Firmware", select 'efx_cabsim.bin'

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

// Using the EFX_CABSIM prebuilt effect ...

// Three-stage tube preamp. Each stage implements a triode preamp model. The three stages are
// followed by a grapic EQ. Preamp and tone parameters range from 1 (lowest frequency, minimum
// level, etc) to 9 (highest frequency, maximum level, etc) with 5 being the middle or neutral
// value.
//
// The FlexFX audio sample rate must be set to 192 kHz. The guitar samples are up-sampled to 960
// kHz for non-linear preamp/tube model processing and then down-sampled back to 192 kHz. Therefore
// the ADC, DAC and USB audio sample rates are all 192 khz.
//
// Each preamp stage incorporates adjustable pre-filtering (bass attenuation), a 12AX7 amplifier
// model with slew-rate limiting and adjustable bias, and post-filtering, all creating a simple
// 12AX7-based guitar preamp stage. The graphic EQ is at the end of the three-stage-preamp chain and
// is used as the final tone-shaping component (a.k.a the preamp's the tone stack).
//
// Property layout for control (knobs, pushbuttons, etc) Values shown are 32-bit
// values represented in ASCII/HEX format or as floating point values ranging
// from +0.0 up to (not including) +1.0.
//
// +------- Effect parameter identifier (Property ID)
// |
// |    +-------------------------------- Volume level
// |    |     +-------------------------- Tone setting
// |    |     |     +-------------------- Reserved
// |    |     |     |     +-------------- Reserved
// |    |     |     |     |     +-------- Preset selection (1 through 9)
// |    |     |     |     |     |+------- Enabled (1=yes,0=bypassed)
// |    |     |     |     |     ||+------ InputL  (1=plugged,0=unplugged)
// |    |     |     |     |     |||+----- OutputL (1=plugged,0=unplugged)
// |    |     |     |     |     ||||+---- InputR  (1=plugged,0=unplugged)
// |    |     |     |     |     |||||+--- OutputR (1=plugged,0=unplugged)
// |    |     |     |     |     ||||||+-- Expression (1=plugged,0=unplugged)
// |    |     |     |     |     |||||||+- USB Audio (1=active)
// |    |     |     |     |     ||||||||
// 1001 0.500 0.500 0.500 0.500 91111111
//
// Property layout for preset data loading (loading IR data). Values shown are
// 32-bit values represented in ASCII/HEX format.
//
// +---------- Effect parameter identifier (Property ID)
// |
// |+--- Preset number (1 through 9)
// ||
// 1n02 0 0 0 0 0 // Begin IR data loading for preset N
// 1n03 A B C D E // Five of the next IR data words to load into preset N
// 1n04 0 0 0 0 0 // End IR data loading for preset N

extern void app_control__cabsim( const int rcv_prop[6], int usb_prop[6], int dsp_prop[6] );

extern void app_mixer__cabsim( const int usb_output[32], int usb_input[32],
                               const int i2s_output[32], int i2s_input[32],
                               const int dsp_output[32], int dsp_input[32], const int prop[6] );

extern void app_thread1__cabsim( int samples[32], const int property[6] );
extern void app_thread2__cabsim( int samples[32], const int property[6] );
extern void app_thread3__cabsim( int samples[32], const int property[6] );
extern void app_thread4__cabsim( int samples[32], const int property[6] );
extern void app_thread5__cabsim( int samples[32], const int property[6] );

// Device configuration ...

const char* product_name_string   = "FlexFX Cabsim";    // Your product name
const char* usb_audio_output_name = "FlexFX Audio Out"; // USB audio output endpoint name
const char* usb_audio_input_name  = "FlexFX Audio In";  // USB audio input endpoint name
const char* usb_midi_output_name  = "FlexFX MIDI Out";  // USB MIDI output endpoint name
const char* usb_midi_input_name   = "FlexFX MIDI In";   // USB MIDI input endpoint name

const int audio_sample_rate     = 48000; // Audio sampling frequency
const int usb_output_chan_count = 2;     // 2 USB audio class 2.0 output channels
const int usb_input_chan_count  = 2;     // 2 USB audio class 2.0 input channels
const int i2s_channel_count     = 2;     // ADC/DAC channels per SDIN/SDOUT wire

const int i2s_sync_word[8] = { 0xFFFFFFFF,0x00000000,0,0,0,0,0,0 }; // I2S WCLK values per slot

static void adc_read( double values[4] ) // I2C control for the Analog Devices AD7999 ADC.
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

void app_control( const int rcv_prop[6], int usb_prop[6], int dsp_prop[6] )
{
	return;
    // Pass incoming properties on to CABSIM control ...
    efx_cabsim__app_control( rcv_prop, usb_prop, dsp_prop );
    // If outgoing USB or DSP properties are now use then come back later ...
    if( usb_prop[0] != 0 || dsp_prop[0] != 0 ) return;

    double volume = 0, tone = 0, preset = 0, pot_values[4];
    adc_read( pot_values ); // Read pot values for volume, tone and preset

    // Check to see if any of the pots have changed in a meaningful way ...
    int updated[3] = {0,0,0};
    updated[0] = (pot_values[3] < volume-FQ(0.01)) || (pot_values[3] > volume+FQ(0.01));
    updated[1] = (pot_values[2] < tone-FQ(0.01)) || (pot_values[2] > tone+FQ(0.01));
    static int focus = 0; double step = FQ(0.1111111), gap = QF(0.03);
    for( int click = 0, ii = 0; ii < 9; ++ii ) {
        click = preset >= (ii*step+gap) && preset <= (ii*(step+1)-gap);
        if( !click && focus ) focus = 0;
        if( !focus && click ) updated[2] = 1;
    }

    // Check to see of any pot changes result in a need to update control settings ...
    if( updated[0] || updated[1] || updated[2] ) {
        if( updated[0] ) volume = pot_values[3];
        if( updated[1] ) tone = pot_values[2];
        if( updated[2] ) preset = pot_values[1];
        // Send property with updated controll settings to the DSP threads via cabsim control ...
        dsp_prop[0] = 0x00008001;
        dsp_prop[1] = volume; dsp_prop[2] = tone; dsp_prop[3] = preset;
        // Pass props along to prebuilt effect 'app_control' function.
        app_control__cabsim( rcv_prop, usb_prop, dsp_prop );
    }
}

void app_mixer( const int usb_output[32], int usb_input[32],
                const int i2s_output[32], int i2s_input[32],
                const int dsp_output[32], int dsp_input[32], const int property[6] )
{
    app_mixer__cabsim( usb_output, usb_input, i2s_output, i2s_input,
                       dsp_output, dsp_input, property );
}

void app_thread1( int samples[32], const int property[6] ) {app_thread1__cabsim(samples,property);}
void app_thread2( int samples[32], const int property[6] ) {app_thread2__cabsim(samples,property);}
void app_thread3( int samples[32], const int property[6] ) {app_thread3__cabsim(samples,property);}
void app_thread4( int samples[32], const int property[6] ) {app_thread4__cabsim(samples,property);}
void app_thread5( int samples[32], const int property[6] ) {app_thread5__cabsim(samples,property);}

function flexfx_create( tag )
{
    var x = "";

    x += "<p>";
    x += "Stereo Cabinet Simulator using impulse responses. Impulse responses to upload must be ";
    x += "stored in a wave file (RIFF/WAV format) and have a sampling frequency of 48 kHz. Both ";
    x += "mono and stereo source data is supported. Stereo can also be employed by specifying two ";
    x += "separate mono WAV files.";
    x += "</p>";
    x += "<table class='flexfx'>";
    x += "<thead>";
    x += "<tr><th>Preset</th><th>Left/Mono</th><th>Right/Stereo</th><th>File Name(s)</th></tr>";
    x += "</thead>";

    x += "<tbody>";
    for( var p = 1; p <= 9; ++p ) {
    x += "<tr><td class='preset' id='"+tag+"_preset"+p+"'>"+p+"</td>";
    x += "<td><input type='file' style='display:none' id='"+tag+"_input"+p+"L'/><button id='"+tag+"_button"+p+"L'>Select IR</button></td>";
    x += "<td><input type='file' style='display:none' id='"+tag+"_input"+p+"R'/><button id='"+tag+"_button"+p+"R'>Select IR</button></td>";
    x += "<td><div style='display:inline-block'>";
    x += "<div id='"+tag+"_text"+p+"L'>Celestion G12H Ann 152 Open Room.wav</div>";
    x += "<div id='"+tag+"_text"+p+"R'>Celestion G12H Ann 152 Open Room.wav</div>";
    x += "</div></td>"; x += "</tr>"; }
    x += "</tbody></table>";

    return x;
}

function flexfx_initialize( tag )
{
    for( var i = 1; i <= 9; ++i )
    {
        $(tag+"_preset"+i).onclick     = _on_cabsim_select;
        $(tag+"_button"+i+"L").onclick = _on_cabsim_button;
        $(tag+"_button"+i+"R").onclick = _on_cabsim_button;
        $(tag+"_input"+i+"L").onchange = _on_cabsim_input;
        $(tag+"_input"+i+"R").onchange = _on_cabsim_input;
    }
    return _on_property_received;
}

function _on_cabsim_select( event )
{
    var tag = flexfx_get_tag( event.target.id );

    if( event.target.innerHTML[0] == '[' ) preset = parseInt( event.target.innerHTML[1] );
    else preset = parseInt( event.target.innerHTML );

    parent = $(tag+"_preset"+preset).parentNode.parentNode;
    for( var i = 1; i <= 9; ++i ) parent.children[i-1].children[0].innerHTML = i;
    parent.children[preset-1].children[0].innerHTML = "[" + preset + "]";

    //flexfx_send_property( tag, property );
}

function _on_cabsim_input( event )
{
    var tag    = flexfx_get_tag( event.target.id );
    var unit   = parseInt( event.target.id[(tag+"_input").length+0] );
    var preset = parseInt( event.target.id[(tag+"_input").length+1] );
    var side   =           event.target.id[(tag+"_input").length+2];
    var file   = $(tag+"_input"+unit+""+preset+side).files[0];

    $(tag+"_text"+unit+""+preset+side).textContent = file.name;

    var reader = new FileReader();
    reader.onload = function(e)
    {
        var samples = flexfx_wave_to_samples( new Uint8Array( reader.result ));
        console.log( samples.length );

        var offset = 0;
        while( offset < 1200 ) {
            if( samples.length >= 4 ) {
                var property = [ 0x01018000+offset/5, samples[0],samples[1],samples[2],samples[3],samples[4] ];
                samples = samples.slice( 5 );
                flexfx_property = [0,0,0,0,0,0];
                flexfx_send_property( tag, property );
            }
            while( 1 ) { if( flexfx_property[0] == 0x01018000+offset/5 ) break; }
            offset += 5;
        }
    }
    reader.readAsArrayBuffer( file );
}

function _on_cabsim_button( event )
{
    var tag    = flexfx_get_tag( event.target.id );
    var preset = parseInt( event.target.id[(tag+"_button").length+0] );
    var side   =           event.target.id[(tag+"_button").length+1];
    $(tag+"_input"+preset+""+side).click();
}

function _on_property_received( property )
{
}
"";