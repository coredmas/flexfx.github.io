import sys, struct, numpy

import numpy as np
import scipy.signal as dsp
import matplotlib.pyplot as plot

if len(sys.argv) < 4:

    print ""
    print "Usage 1: python design.py <type> <freq> <Q> <gain>"
    print ""
    print "         <type> filter type (notch, lowpass, highpass, allpass, bandpass,"
    print "                             peaking, highshelf, or lowshelf"
    print "         <freq> is cutoff frequency relative to Fs (0 <= freq < 0.5)"
    print "         <Q> is the filter Q-factor"
    print "         <gain> is the filter positive or negative gain in dB"
    print ""

    exit(0)

np.seterr( all='ignore' )

type = sys.argv[1]
freq = float( sys.argv[2] )
Q    = float( sys.argv[3] )
gain = float( sys.argv[4] )

def plot_response( bb, aa, xmin=None, xmax=None, ymin=-60.0, ymax=6.0 ):

    import matplotlib.pyplot as plt
    w, h = dsp.freqz(bb,aa)
    fig = plt.figure()
    plt.title('Digital filter frequency response')
    ax1 = fig.add_subplot(111)
    plt.plot(w/(2*np.pi)*1.001, 20 * np.log10(abs(h)), 'b')
    #plt.semilogx(w/(2*np.pi)*1.001, 20 * np.log10(abs(h)), 'b')
    plt.ylabel('Amplitude [dB]', color='b')
    plt.xlabel('Frequency [Normalized to Fs]')
    plt.ylim( -30,+6 )
    ax2 = ax1.twinx()
    angles = np.unwrap(np.angle(h)) / (2*np.pi) * 360.0
    plt.plot(w/(2*np.pi)*1.001, angles, 'g')
    plt.ylabel('Angle (degrees)', color='g')
    plt.grid()
    plt.axis('tight')
    plt.show()

def _make_biquad_notch( filter_freq, q_factor ):

	w0 = 2.0 * np.pi * filter_freq
	alpha = np.sin(w0)/(2.0 * q_factor)

	b0 = +1.0; b1 = -2.0 * np.cos(w0); b2 = +1.0
	a0 = +1.0 + alpha; a1 = -2.0 * np.cos(w0); a2 = +1.0 - alpha

	plot_response( [b0,b1,b2], [a0,a1,a2], ymin=-60, ymax=6 )

	print "FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f)" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0)

def _make_biquad_lowpass( filter_freq, q_factor ):

	w0 = 2.0 * np.pi * filter_freq
	alpha = np.sin(w0)/(2 * q_factor)

	b0 = (+1.0 - np.cos(w0)) / 2.0; b1 =  +1.0 - np.cos(w0); b2 = (+1.0 - np.cos(w0)) / 2.0
	a0 = +1.0 + alpha; a1 = -2.0 * np.cos(w0); a2 = +1.0 - alpha

	plot_response( [b0,b1,b2], [a0,a1,a2], ymin=-60, ymax=6 )

	print "FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f);" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0)

def _make_biquad_highpass( filter_freq, q_factor ):

	w0 = 2.0 * np.pi * filter_freq
	alpha = np.sin(w0)/(2 * q_factor)

	b0 = (1.0 + np.cos(w0)) / 2.0; b1 = -(1.0 + np.cos(w0)); b2 = (1.0 + np.cos(w0)) / 2.0
	a0 = +1.0 + alpha; a1 = -2.0 * np.cos(w0); a2 = +1.0 - alpha

	plot_response( [b0,b1,b2], [a0,a1,a2], ymin=-60, ymax=6 )

	print "FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f);" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0)

def _make_biquad_allpass( filter_freq, q_factor ):

	w0 = 2.0 * np.pi * filter_freq
	alpha = np.sin(w0)/(2.0 * q_factor)

	b0 = +1.0 - alpha; b1 = -2.0 * np.cos(w0); b2 = +1.0 + alpha
	a0 = +1.0 + alpha; a1 = -2.0 * np.cos(w0); a2 = +1.0 - alpha

	plot_response( [b0,b1,b2], [a0,a1,a2], ymin=-6, ymax=6 )

	print "FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f);" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0)

from scipy.signal import butter, lfilter

def butter_bandpass(lowcut, highcut, fs, order=5):
    nyq = 0.5 * fs
    low = lowcut / nyq
    high = highcut / nyq
    b, a = butter(order, [low, high], btype='band')
    return b, a


def butter_bandpass_filter(data, lowcut, highcut, fs, order=5):
    b, a = butter_bandpass(lowcut, highcut, fs, order=order)
    y = lfilter(b, a, data)
    return y

# Constant 0 dB peak gain
# FIXME: Results in a peaking filter at freq1 rather than BP filter that's flat within the pass-band
#def _make_biquad_bandpass( filter_freq1, filter_freq2 ):
def _make_biquad_bandpass( filter_freq, q_factor ):

    #filter_freq = (filter_freq1 + filter_freq2) / 2
    #w0 = 2.0 * np.pi * filter_freq
    #BW = (filter_freq2 -filter_freq1) / filter_freq1
    #alpha = np.sin(w0) * np.sinh( np.log(2)/2 * BW * w0/np.sin(w0) )

    w0 = 2.0 * np.pi * filter_freq
    alpha = np.sin(w0)/(2.0 * q_factor)
    
    b0 = alpha; b1 = +0.0; b2 = -alpha
    a0 = +1.0 + alpha; a1 = -2.0 * np.cos(w0); a2 = +1.0 - alpha

    plot_response( [b0,b1,b2], [a0,a1,a2] )
    
    b,a = dsp.iirfilter( N=1, Wn=[0.1,0.2], btype='bandpass' )
    b0 = b[0]; b1 = b[1]; a0 = a[0]; a1 = a[1];
    plot_response( b, a )

    #print "FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f);" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0)

# gain can be + or -
def _make_biquad_peaking( filter_freq, q_factor, gain_db ):

	A  = np.sqrt( 10 ** (gain_db/20) )
	w0 = 2.0 * np.pi * filter_freq
	alpha = np.sin(w0)/(2.0 * q_factor)

	b0 = +1.0 + alpha * A; b1 = -2.0 * np.cos(w0); b2 = +1.0 - alpha * A
	a0 = +1.0 + alpha / A; a1 = -2.0 * np.cos(w0); a2 = +1.0 - alpha / A

	if gain_db == 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3, ymax=3 )
	if gain_db  < 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3+gain_db, ymax=3 )
	if gain_db  > 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3, ymax=3+gain_db )

	print "FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f)};" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0)
	#print "    { FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),0 }," % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0)

def _make_biquad_lowshelf( filter_freq, q_factor, gain_db ):

	S = q_factor
	A  = 10.0 ** (gain_db / 40.0)
	w0 = 2.0 * np.pi * filter_freq
	alpha = np.sin(w0)/2 * np.sqrt( (A + 1/A)*(1/S - 1) + 2 )

	b0 = A*( (A+1) - (A-1)*np.cos(w0) + 2*np.sqrt(A)*alpha )
	b1 =  2*A*( (A-1) - (A+1)*np.cos(w0) )
	b2 = A*( (A+1) - (A-1)*np.cos(w0) - 2*np.sqrt(A)*alpha )
	a0 = (A+1) + (A-1)*np.cos(w0) + 2*np.sqrt(A)*alpha
	a1 = -2*( (A-1) + (A+1)*np.cos(w0) )
	a2 = (A+1) + (A-1)*np.cos(w0) - 2*np.sqrt(A)*alpha

	if gain_db == 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3, ymax=3 )
	if gain_db  < 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3+gain_db, ymax=3 )
	if gain_db  > 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3, ymax=3+gain_db )

	print "int coeffs[5] = {FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f)};" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0)

def _make_biquad_highshelf( filter_freq, q_factor, gain_db ):

	S = q_factor
	A  = 10.0 ** (gain_db / 40.0)
	w0 = 2.0 * np.pi * filter_freq
	alpha = np.sin(w0)/2 * np.sqrt( (A + 1/A)*(1/S - 1) + 2 )

	b0 = A*( (A+1) + (A-1)*np.cos(w0) + 2*np.sqrt(A)*alpha )
	b1 = -2*A*( (A-1) + (A+1)*np.cos(w0) )
	b2 = A*( (A+1) + (A-1)*np.cos(w0) - 2*np.sqrt(A)*alpha )
	a0 = (A+1) - (A-1)*np.cos(w0) + 2*np.sqrt(A)*alpha
	a1 = 2*( (A-1) - (A+1)*np.cos(w0) )
	a2 = (A+1) - (A-1)*np.cos(w0) - 2*np.sqrt(A)*alpha

	if gain_db == 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3, ymax=3 )
	if gain_db  < 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3+gain_db, ymax=3 )
	if gain_db  > 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3, ymax=3+gain_db )

	print "int coeffs[5] = {FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f)};" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0)

def _make_lowpass( filter_freq, q_factor ):

    #plot_response( [1/1.324919696,-1/1.324919696], [1.0,-0.5095254495] ) # high pass
    #plot_response( [1/1.324919696,+1/1.324919696], [1.0,+0.5095254495] ) # low pass
    plot_response( [1/4.077683537,+1/4.077683537], [1.0,-0.5095254495] ) # low pass Fc=0.1
    
    # 0.001*(1.22^15),
    
    # 0.00122          2.619084652e+02, 0.9923637443
    # 0.0014884        2.148588863e+02, 0.9906915649
    # 0.001815848      1.762935451e+02, 0.9886552851
    # 0.00221533456    1.446824724e+02, 0.9861766255
    # 0.002702708163   1.187715897e+02, 0.9831609562
    # 0.003297303959   9.753295683e+01, 0.9794941109
    # 0.00402271083    8.012399213e+01, 0.9750386876
    # 0.004907707213   6.585404479e+01, 0.9696298078
    # 0.0059874028     5.415699560e+01, 0.9630703296
    # 0.007304631415   4.456879768e+01, 0.9551255563
    # 0.008911650327   3.670906671e+01, 0.9455175470
    # 0.0108722134     3.026599036e+01, 0.9339192283
    # 0.01326410035    2.498395793e+01, 0.9199486324
    # 0.01618220242    2.065341971e+01, 0.9031637362
    # 0.01974228696    1.710257356e+01, 0.8830585354
    
    
    return

    C1 = 0.25e-9
    C2 = 20.0e-9
    C3 = 20.0e-9
    R1 = 250e3
    R2 = 1e6
    R3 = 25e3
    R4 = 56e3

    Fs = 48000.0
    k = 2 * Fs
    l = 0.5; m = 0.5; t = 0.5;

    m2 = m * m
    
    b1 = t*C1*R1 + m*C3*R3 + l*(C1*R2 + C2*R2) + (C1*R3 + C2*R3)
    b2=t*(C1*C2*R1*R4+C1*C3*R1*R4)-m2*(C1*C3*R3*R3+C2*C3*R3*R3)+m*(C1*C3*R1*R3+C1*C3*R3*R3+C2*C3*R3*R3)+l*(C1*C2*R1*R2+C1*C2*R2*R4+C1*C3*R2*R4)+l*m*(C1*C3*R2*R3+C2*C3*R2*R3)+(C1*C2*R1*R3+C1*C2*R3*R4+C1*C3*R3*R4)
    b3=l*m*(C1*C2*C3*R1*R2*R3+C1*C2*C3*R2*R3*R4)-m2*(C1*C2*C3*R1*R3*R3+C1*C2*C3*R3*R3*R4)+m*(C1*C2*C3*R1*R3*R3+C1*C2*C3*R3*R3*R4)+t*C1*C2*C3*R1*R3*R4-t*m*C1*C2*C3*R3*R3*R4+t*l*C1*C2*C3*R1*R2*R4
    a0=1
    a1=(C1*R1+C1*R3+C2*R3+C2*R4+C3*R4)+m*C3*R3+l*(C1*R2+C2*R2)
    a2=m*(C1*C3*R1*R3-C2*C3*R3*R4+C1*C3*R3*R3+C2*C3*R3*R3)+l*m*(C1*C3*R2*R3+C2*C3*R2*R3)-m2*(C1*C3*R3*R3+C2*C3*R3*R3)+l*(C1*C2*R2*R4+C1*C2*R1*R2+C1*C3*R2*R4+C2*C3*R2*R4)+(C1*C2*R1*R4+C1*C3*R1*R4+C1*C2*R3*R4+C1*C2*R1*R3+C1*C3*R3*R4+C2*C3*R3*R4)
    a3=l*m*(C1*C2*C3*R1*R2*R3+C1*C2*C3*R2*R3*R4)-m2*(C1*C2*C3*R1*R3*R3+C1*C2*C3*R3*R3*R4)+m*(C1*C2*C3*R3*R3*R4+C1*C2*C3*R1*R3*R3-C1*C2*C3*R1*R3*R4)+l*C1*C2*C3*R1*R2*R4+C1*C2*C3*R1*R3*R4

    B0 =       -b1*k -b2*k*k   -b3*k*k*k
    B1 =       -b1*k +b2*k*k +3*b3*k*k*k
    B2 =       +b1*k +b2*k*k -3*b3*k*k*k
    B3 =       +b1*k -b2*k*k   +b3*k*k*k
    A0 = -a0   -a1*k -a2*k*k   -a3*k*k*k
    A1 = -3*a0 -a1*k +a2*k*k +3*a3*k*k*k
    A2 = -3*a0 +a1*k +a2*k*k -3*a3*k*k*k
    A3 = -a0   +a1*k -a2*k*k   +a3*k*k*k
    
    plot_response( [B0,B1,B2,B3],[A0,A1,A2,A3] )
    return
    
    K = 0.9
    
    plot_response( [(1-K)**1,0,0],  [1,-K,0] )
    plot_response( [(1-K)**2,0,0],  [1,-2*K,K*K] )
    plot_response( [(1-K)**3,0,0,0],[1,-3*K,3*K*K,-K*K*K] )
    plot_response( [(1-K)**4,0,0,0],[1,-4*K,6*K*K,-4*K*K*K,K*K*K*K] )
    #plot_response( [0.1,0.09,0],[1,0,-0.81] )
    
    #if gain_db == 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3, ymax=3 )
    #if gain_db  < 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3+gain_db, ymax=3 )
    #if gain_db  > 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3, ymax=3+gain_db )

    #print "int coeffs[5] = {FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f)};" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0)

"""
y0 = x0 - Kx0 + Ky1
y1 = x1 - Kx1 + K(x2 - Kx2 + Kx3 - KKx3 + KKy4) = x1 - Kx1 + Kx2 - KKx2 + KKx3 - KKKx3 + KKKx4
y2 = x2 - Kx2 + K(x3 - Kx3 + Ky4) = x2 - Kx2 + Kx3 - KKx3 + KKy4
y3 = x3 - Kx3 + Ky4





"""


"""
Q = 1.0
_make_biquad_peaking(  400.0/960000, Q, 6.0 )
_make_biquad_peaking(  500.0/960000, Q, 6.0 )
_make_biquad_peaking(  600.0/960000, Q, 6.0 )
_make_biquad_peaking(  700.0/960000, Q, 6.0 )
_make_biquad_peaking(  800.0/960000, Q, 6.0 )
_make_biquad_peaking(  900.0/960000, Q, 6.0 )
_make_biquad_peaking( 1000.0/960000, Q, 6.0 )
_make_biquad_peaking( 1100.0/960000, Q, 6.0 )
_make_biquad_peaking( 1200.0/960000, Q, 6.0 )
exit(0)
"""

if type == "lp":         _make_lowpass  ( freq, Q )
if type == "notch":      _make_biquad_notch    ( freq, Q )
if type == "lowpass":    _make_biquad_lowpass  ( freq, Q )
if type == "highpass":   _make_biquad_highpass ( freq, Q )
if type == "allpass":    _make_biquad_allpass  ( freq, Q )
if type == "bandpass":   _make_biquad_bandpass ( freq, Q )
if type == "peaking":    _make_biquad_peaking  ( freq, Q, gain )
if type == "highshelf":  _make_biquad_highshelf( freq, Q, gain )
if type == "lowshelf":   _make_biquad_lowshelf ( freq, Q, gain )
