#ifndef INCLUDED_C99_H
#define INCLUDED_C99_H

#include "xio.h"

void c99_control( const double parameters[20], int property[6] );

void c99_mixer( const int usb_output[32], int usb_input[32],
                const int adc_output[32], int dac_input[32],
                const int dsp_output[32], int dsp_input[32], const int property[6] );

#endif
