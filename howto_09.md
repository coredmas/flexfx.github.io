FlexFX&trade; Kit - Reading Pots/Knobs
--------------------------------------

```C
#include "flexfx.h"
#include <math.h>

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

void control( int rcv_prop[6], int usb_prop[6], int dsp_prop[6] )
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

void mixer( const int* usb_output, int* usb_input,
            const int* i2s_output, int* i2s_input,
            const int* dsp_output, int* dsp_input, const int* property )
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

void dsp_thread1( int* samples, const int* property )
{
}

```

