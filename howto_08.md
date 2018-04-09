Prebuilt Effects
----------------------------------
The FlexFX kit contains some highly optimized effects. These effects are in binary form and can be used for free on FlexFX boards. Here's an example of how to use the optimzied stereo Cabinet simulator that supports 37.5 msec of IR processing in stereo mode at 48 kHz, and 75 msec of IR processing in mono mode at 48 kHz.

The FlexFX properties definitions for uploading IR data (stored in wave files on a USB atteched host computer) is documented in 'efx_cabsim.txt' and is also available via USB MIDI by issueing the FlexFX USB/MIDI property for returning a device's MIDI interface (the text is returned via USB).  

This effect also supports the HTML5 interface for controlling the device (firmware upgrading, uploading IR data, etc). The javascript code for the effect is returned via USB MIDI by issueing the FlexFX USB/MIDI property for returning a device's javascript controller interface.  The HTML5 application called 'flexfx.html' does this automatically and will displayt this device's GUI interface if the device is pluuged into the host computer via a USB cable. Google Chrome must be used.

```
#include "efx_cabsim.h"

const char* product_name_string = "FlexFX Cabsim";  // Your company/product name

void control( int rcv_prop[6], int usb_prop[6], int dsp_prop[6] )
{
    efx_cabsim__control( rcv_prop, usb_prop, dsp_prop );
}

void mixer( const int* usb_output, int* usb_input,
            const int* i2s_output, int* i2s_input,
            const int* dsp_output, int* dsp_input, const int* property )
{
    efx_cabsim__mixer( usb_output, usb_input, i2s_output, i2s_input,
                       dsp_output, dsp_input, property );
}

void dsp_thread1( int* samples, const int* property ) {efx_cabsim__dsp_thread1(samples,property);}
void dsp_thread2( int* samples, const int* property ) {efx_cabsim__dsp_thread2(samples,property);}
void dsp_thread3( int* samples, const int* property ) {efx_cabsim__dsp_thread3(samples,property);}
void dsp_thread4( int* samples, const int* property ) {efx_cabsim__dsp_thread4(samples,property);}
void dsp_thread5( int* samples, const int* property ) {efx_cabsim__dsp_thread5(samples,property);}
```

Discovery and Control
----------------------------
All USB MIDI data flow between a host computer and FlexFX devices occurs via FlexFX properties (see 'Run-time' Control above). FlexFX devices, including the optimized prebuilt effects (see 'Prebuilt Effects' above), support a discovery process whereby any host computer can querry a FlexFX device for both its MIDI interface and its Javascript control code via a FlexFX property.

The FlexFx property interface data, returned in a human readable text) is returned via USB MIDI if the device receives FlexFX properties with ID's of 0x21 (begin), 0x22 (next), and 0x23 (end). The returned text can be used to provide additonal FlexFX property definitions for device specific (or effect specific) control. This textual data is automatically included in the FlexFX firmware image (see 'Development Steps' above) if a .txt file with the same name as the effect being built exists (e.g. efx_cabsim.txt for efx_cabsim.c firmware).

Here's an example of a textual property interface definition for the 'efx_cabsim' effect. Note that this exact text
is returned upon issuing a FlexFX proeprty with ID = 0x0004 to the target FlexFX device.

```
# FLEXFX STEREO CABSIM Property Interface
#
# Stereo Cabinet Simulator using impulse responses. Impulse responses to upload
# must be stored in a wave file (RIFF/WAV format) and have a sampling frequency
# of 48 kHz. Both mono and stereo source data is supported.  Stereo can also be
# employed by specifying two mono WAV files.
#
# PROP ID   DIRECTION        DESCRIPTION
# 0001      Bidirectional    Return 3DEGFLEX, versions and props[4:5] to host
# 0002      Bidirectional    Return the device product name (up to 40 bytes)
# 0003      Device to Host   Start dumping the text in this file
# 0004      Bidirectional    Next 40 bytes of property interface text
# 0005      Host to Device   End of property interface text dump
# 0006      Device to Host   Start a controller javascript code dump
# 0007      Bidirectional    Next 40 bytes of javascript code text
# 0008      Host to Device   End of controller javascript code text
# 0009      Bidirectional    Begin firmware upgrade, echoed back to host
# 000A      Bidirectional    Next 32 bytes of firmware image data, echoed
# 000B      Bidirectional    End firmware upgrade, echoed back to host
# 000C      Bidirectional    Begin flash user data load, echoed back to host
# 000D      Bidirectional    Next 32 bytes of flash user data, echoed
# 000E      Bidirectional    End flash data loading, echoed back to host
# 000F      Bidirectional    Query/return DSP thread run-time (audio cycles)
# PROP ID   DIRECTION        DESCRIPTION
# 1000      Host to Device   Return volume,tone,preset control settings
# 1001      Bidirectional    Update controls (overrides physical controls)
# 1n01      Bidirectional    Up to 20 charactr name for preset N (1<=N<=9)
# 1n02      Bidirectional    Begin data upload for preset N, begin upload ACK
# 1n03      Bidirectional    Five IR data words for preset N or echoed data
# 1n04      Bidirectional    End data upload for preset N or end upload ACK
# 1n05      Bidirectional    First 20 chars of data file name for preset N
# 1n06      Bidirectional    Next 20 chars of data file name for preset N
# 1n07      Bidirectional    Last 20 chars of data file name for preset N
#
# Property layout for control (knobs, pushbuttons, etc) Values shown are 32-bit
# values represented in ASCII/HEX format or as floating point values ranging
# from +0.0 up to (not including) +1.0.
#
# +------- Effect parameter identifier (Property ID)
# |
# |    +-------------------------------- Volume level
# |    |     +-------------------------- Tone setting
# |    |     |     +-------------------- Reserved
# |    |     |     |     +-------------- Reserved
# |    |     |     |     |     +-------- Preset selection (1 through 9)
# |    |     |     |     |     |+------- Enabled (1=yes,0=bypassed)
# |    |     |     |     |     ||+------ InputL  (1=plugged,0=unplugged)
# |    |     |     |     |     |||+----- OutputL (1=plugged,0=unplugged)
# |    |     |     |     |     ||||+---- InputR  (1=plugged,0=unplugged)
# |    |     |     |     |     |||||+--- OutputR (1=plugged,0=unplugged)
# |    |     |     |     |     ||||||+-- Expression (1=plugged,0=unplugged)
# |    |     |     |     |     |||||||+- USB Audio (1=active)
# |    |     |     |     |     ||||||||
# 1001 0.500 0.500 0.500 0.500 91111111
#
# Property layout for preset data loading (loading IR data). Values shown are
# 32-bit values represented in ASCII/HEX format.
#
# +---------- Effect parameter identifier (Property ID)
# |
# |+--- Preset number (1 through 9)
# ||
# 1n02 0 0 0 0 0 # Begin IR data loading for preset N
# 1n03 A B C D E # Five of the next IR data words to load into preset N
# 1n04 0 0 0 0 0 # End IR data loading for preset N
```

The FlexFX HTML5 control javascript source code is returned via USB MIDI if the device receives FlexFX properties with ID's of 0x31 (begin), 0x32 (next), and 0x33 (end). The returned code can be used in HTML5 applications to access and control FlexFX devices via HTML MIDI whoch is supported by Google Chrome. The HTML5 application called 'flexfx.html' will sense USB MIDI events, including the plugging and unplugging of FlexFX devices, querry the device for its javascript controller code, and display the device's GUI interface on a webpage. This javascript codeis automatically included in the FlexFX firmware image (see 'Development Steps' above) if a .js file with the same name as the effect being built exists (e.g. efx_cabsim.js for efx_cabsim.c firmware).

Here's an example of HTML5 controller for the 'efx_cabsim' effect:
![alt tag](https://raw.githubusercontent.com/markseel/flexfx_kit/master/efx_cabsim.png)

