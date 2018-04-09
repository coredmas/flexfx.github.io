FlexFX&trade; Kit - Control Properties
==================================

FlexFX applications can be controlled using FlexFX property exchanges via USB MIDI.
A property is composed of a 16-bit IDC and five 32-bit data words for a total of 44 bytes of data.

An example property is shown below:

```
Property ID = 0x01011301     The ID must have a non-zero 16-bit upper word (0x0101 in this example)
Param 1     = 0x11223344
Param 2     = 0x55667788
Param 3     = 0x99aabbcc
Param 4     = 0x01234567
Param 5     = 0x89abcdef
```

Since FlexFX properties are transfered via MIDI the 44 bytes of data must be encapsulated withing a MIDI SYSEX message when
parsing and rendering FlexFX properties on the USB host.
The FlexFX framework handles parsing and rendering of MIDI SYSEX encapulated FlexFX data transfer therefore the user
application need not deal with MIDi SYSEX. The FlexFX SDK handles the USB MIDI and MIDI sysex rendering/parsing -
the audio firmware only sees 16-bit ID and five 32-word properties and is not aware of USB MIDI and sysex rendering/parsing as this is handled by the FlexFX SDK.

For detailed information regarding the rendering/parsing process and MIDI SYSEX formatting see the 'flexfx.py' script
that's used to send/receive properties to FlexFX applications via USB.

FlexFX supports predeifned properties with the 16-bit ID being less than 0x1000.
User defined properties should therefore use 16-bit ID's greater then or equal to 0x1000.

```
ID        DIRECTION        SUMMARY
0001      Bidirectional    Return 3DEGFLEX, versions and props[4:5] to host
0002      Bidirectional    Return the device product name (up to 40 bytes)
0003      Device to Host   Start dumping the text in this file
0004      Bidirectional    Next 40 bytes of property interface text
0005      Host to Device   End of property interface text dump
0006      Device to Host   Start a controller javascript code dump
0007      Bidirectional    Next 40 bytes of javascript code text
0008      Host to Device   End of controller javascript code text
0009      Bidirectional    Begin firmware upgrade, echoed back to host
000A      Bidirectional    Next 32 bytes of firmware image data, echoed
000B      Bidirectional    End firmware upgrade, echoed back to host
000C      Bidirectional    Begin flash user data load, echoed back to host
000D      Bidirectional    Next 32 bytes of flash user data, echoed
000E      Bidirectional    End flash data loading, echoed back to host
000F      Bidirectional    Query/return DSP thread run-time (audio cycles)
```

#### FlexFX ID = 0x0001
#### FlexFX ID = 0x0002
#### FlexFX ID = 0x0003
#### FlexFX ID = 0x0004
#### FlexFX ID = 0x0005
#### FlexFX ID = 0x0006
#### FlexFX ID = 0x0007
#### FlexFX ID = 0x0008
#### FlexFX ID = 0x0009
#### FlexFX ID = 0x000A
#### FlexFX ID = 0x000B
#### FlexFX ID = 0x000C
#### FlexFX ID = 0x000D
#### FlexFX ID = 0x000E
#### FlexFX ID = 0x000F
