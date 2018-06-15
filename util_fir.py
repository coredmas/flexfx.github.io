import os, sys
import numpy as np
from scipy.signal import kaiserord, firwin, freqz

if len(sys.argv) < 4:

    print ""
    print "Usage: python util_fir <pass_freq> <stop_freq> <ripple> <attenuation>"
    print ""
    print "       <pass_freq> is the passband frequency relative to Fs (0 <= freq < 0.5)"
    print "       <stop_freq> is the stopband frequency relative to Fs (0 <= freq < 0.5)"
    print "       <ripple> is the maximum passband ripple"
    print "       <attenuation> is the stopband attenuation"
    print ""

    exit(0)

passband_freq   = float( sys.argv[1] )
stopband_freq   = float( sys.argv[2] )
passband_ripple = float( sys.argv[3] )
stopband_atten  = float( sys.argv[4] )

width = abs(passband_freq - stopband_freq) / 0.5
(tap_count,beta) = kaiserord( ripple = stopband_atten, width = width )
taps = firwin( numtaps = tap_count, \
               cutoff  = ((passband_freq+stopband_freq)/2)/0.5, \
               window  = ('kaiser', beta) )

import matplotlib.pyplot as plt
w, h = freqz( taps, worN=8000 )
fig = plt.figure()
plt.title('Digital filter frequency response')
ax1 = fig.add_subplot(111)
plt.plot(w/(2*np.pi)*1.001, 20 * np.log10(abs(h)), 'b')
plt.ylabel('Amplitude [dB]', color='b')
plt.xlabel('Frequency [Normalized to Fs]')
ax2 = ax1.twinx()
angles = np.unwrap(np.angle(h)) / (2*np.pi) * 360.0
plt.plot(w/(2*np.pi)*1.001, angles, 'g')
plt.ylabel('Angle (degrees)', color='g')
plt.grid()
plt.axis('tight')
plt.show()

ii = 0
print "%u Taps" % len(taps)
for cc in taps[0:len(taps)-1]:
    if (ii % 5) == 0: sys.stdout.write('    ')
    sys.stdout.write( "FQ(%+1.9f)," % cc )
    ii += 1
    if (ii % 5) == 0: sys.stdout.write('\n')
sys.stdout.write( "FQ(%+1.9f)\n" % taps[len(taps)-1] )
