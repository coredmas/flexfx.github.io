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

// FLASH read write functions.

void flash_read ( int blocknum, byte buffer[4096] );
void flash_write( int blocknum, const byte buffer[4096] );

void timer_delay( int microseconds );

// I2C functions for peripheral control (do not use these in real-time DSP threads).

void i2c_start( int speed );  // Set bit rate, assert an I2C start condition.
byte i2c_write( byte value ); // Write 8-bit data value.
byte i2c_read ( void );       // Read 8-bit data value.
void i2c_ack  ( byte ack );   // Assert the ACK/NACK bit after a read.
void i2c_stop ( void );       // Assert an I2C stop condition.

void port_set( int mask, int value ); // Write 0 or 1 to ports/pins indicated by 'mask'
int  port_get( int mask );            // Read ports indicated by 'mask', set to HiZ state

// Read ADC values via I2C.  This function supports the MAX11600-MAX11605 series of devices.

void adc_read( double values[8] ); // 0.0 <= value[n] < 1.0

void log_chr( char val );
void log_str( const char* val );
void log_bin( const byte* data, int len );
void log_hex( unsigned char val );
void log_hex2( unsigned val );
void log_hex4( unsigned val );
void log_dec( int value, int width, char pad );

#endif
