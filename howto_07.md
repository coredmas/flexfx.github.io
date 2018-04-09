FlexFX&trade; Kit - Design Tools
----------------------------------

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
