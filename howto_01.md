FlexFX&trade; Kit - Getting Started
--------------------------------

1) Download and install the free XMOS development tools from www.xmos.com.   
2) Obtain the FlexFX&trade; dev-kit for building your own apps from "https://github.com/markseel/flexfx_kit".  
3) Set environment variables for using XMOS command-line build tools.  
4) Add your own source code to a new 'C' file (e.g. 'your_application.c').   
5) Build the application using the build script / batch file (uses the XMOS command line compiler/linker).   
6) Burn the firmware binary via USB using the 'flexfx.py' Python script.   

```
Get the kit ............... git clone https://github.com/markseel/flexfx_kit.git
Set XMOS build tools environment variables
  on Windows .............. c:\Program Files (x86)\XMOS\xTIMEcomposer\Community_14.2.0\SetEnv.bat
  on OS X / Linux ......... /Applications/XMOS_xTIMEcomposer_Community_14.1.1/SetEnv.command
Build your custom application (be sure to exclude the .c file extension)
  on Windows .............. build.bat your_application
  on OS X / Linux ......... ./build.sh your_application
Burn to FLASH via USB ..... flexfx.py 0 your_application.bin
```

One can create custom audio processing effects by downloading the FlexFX&trade; audio processing framework, adding custom audio processing DSP code and property handling code, and then compiling and linking using XMOS tools (xTIMEcomposer, free to download).
The custom firmware can then be burned to FLASH using xTIMEcomposer and the XTAG-2 or XTAG-3 JTAG board ($20 from Digikey), via USB/MIDI (there are special properties defined for firmware upgrading and boot image selection).

FlexFX&trade; implements the USB class 2.0 audio and USB MIDI interfaces, the I2S/CODEC interface, sample data transfer from USB/I2S/DSP threads, property/parameter routing, and firmware upgrading. FlexFX provides functions for peripheral control (GPIO's and I2C), FLASH memory access, fixed point scalar adn vector math, and a wide range of DSP functions.

All one has to do then is add DSP code and any relevant custom property handling code to the five audio processing threads which all run in parallel and implement a 5-stage audio processing pipeline.
Each processing thread is allocated approximately 100 MIPS and executes once per audio cycle.
All threads are executed once per audio cycle and therefore operate on USB and I2S sample data one audio frame at a time.


