FlexFX&trade; Kit - Programming Tools
----------------------------------

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
