// Delay/chorus/flanger with settings for LFO depth and rate, base delay, modulated delay
// high and low-pass filters, feedback ration, delay regeneration ratio, and wet/dry blending.
// Three bypass-switch controlled parameters, USB/MIDI/HTML control, and parameter-set morphing.

#include <math.h>
#include <string.h>
#include "dsp.h"
#include "dsp.i"
#include "c99.h"

const char* product_name_string   = "C99 Delay";
const char* usb_audio_output_name = "Delay Audio Out";
const char* usb_audio_input_name  = "Delay Audio In";
const char* usb_midi_output_name  = "Delay MIDI Out";
const char* usb_midi_input_name   = "Delay MIDI In";

const int audio_sample_rate     = 192000; // Default sample rate at boot-up
const int audio_clock_mode      = 0; // 0=internal/master,1=external/master,2=slave
const int usb_output_chan_count = 2; // 2 USB audio class 2.0 output channels
const int usb_input_chan_count  = 2; // 2 USB audio class 2.0 input channels
const int i2s_channel_count     = 2; // Channels per SDIN/SDOUT wire (2,4,or 8)

const int i2s_sync_word[8] = { 0xFFFFFFFF,0x00000000,0,0,0,0,0,0 };

const int one_over_e = FQ(1.0/2.71828182845);

const char* control_labels[21] = { "C99 Delay",
                                   "Input Drive", "Delay (Range)", "Delay (Time)",
                                   "LFO Rate", "LFO Depth",
                                   "Filter Freq", "Filter Q",
                                   "Diffusion", "Feedback", "Dry/Wet Mix",
                                   "Output Volume",
                                   "","","","","","","","","" };

int _delay_dnsample_coeff[80] = // pass=0.036 stop=0.125 atten=108
{
    FQ(+0.000001056),FQ(+0.000002210),FQ(+0.000001012),FQ(-0.000006332),FQ(-0.000023047),
    FQ(-0.000048192),FQ(-0.000072728),FQ(-0.000077788),FQ(-0.000038090),FQ(+0.000067758),
    FQ(+0.000241332),FQ(+0.000448186),FQ(+0.000610142),FQ(+0.000615375),FQ(+0.000351568),
    FQ(-0.000241310),FQ(-0.001113295),FQ(-0.002065878),FQ(-0.002759114),FQ(-0.002782434),
    FQ(-0.001787396),FQ(+0.000346009),FQ(+0.003362514),FQ(+0.006568982),FQ(+0.008911079),
    FQ(+0.009210219),FQ(+0.006529874),FQ(+0.000585332),FQ(-0.007927782),FQ(-0.017215583),
    FQ(-0.024569582),FQ(-0.026866189),FQ(-0.021300211),FQ(-0.006183892),FQ(+0.018404299),
    FQ(+0.050324203),FQ(+0.085558920),FQ(+0.118885031),FQ(+0.144898057),FQ(+0.159155684),
    FQ(+0.159155684),FQ(+0.144898057),FQ(+0.118885031),FQ(+0.085558920),FQ(+0.050324203),
    FQ(+0.018404299),FQ(-0.006183892),FQ(-0.021300211),FQ(-0.026866189),FQ(-0.024569582),
    FQ(-0.017215583),FQ(-0.007927782),FQ(+0.000585332),FQ(+0.006529874),FQ(+0.009210219),
    FQ(+0.008911079),FQ(+0.006568982),FQ(+0.003362514),FQ(+0.000346009),FQ(-0.001787396),
    FQ(-0.002782434),FQ(-0.002759114),FQ(-0.002065878),FQ(-0.001113295),FQ(-0.000241310),
    FQ(+0.000351568),FQ(+0.000615375),FQ(+0.000610142),FQ(+0.000448186),FQ(+0.000241332),
    FQ(+0.000067758),FQ(-0.000038090),FQ(-0.000077788),FQ(-0.000072728),FQ(-0.000048192),
    FQ(-0.000023047),FQ(-0.000006332),FQ(+0.000001012),FQ(+0.000002210),FQ(+0.000001056)
};
int _delay_upsample_coeff[80], _delay_dnsample_state[80], _delay_upsample_state[80];

int _sine_lut[1024];

void c99_control( const double parameters[20], int property[6] )
{
	static int state = 1;
	
    if( state == 1 ) // Volume
    {
        property[0] = state; state = 2;
        property[1] = FQ( 0.25 + 0.75 * parameters[10] ); // Volume
    }
    else if( state == 2 ) // delay,rate,depth,blend
    {
        property[0] = state; state = 3;   
        property[2] = FQ( parameters[0] ); // Input drive
        property[2] = FQ( parameters[1] * parameters[2] ); // Delay base time
        property[3] = FQ( 0.0000005 +  parameters[3] * 0.00002 ); // LFO time delta
        property[4] = FQ( parameters[4] ); // Modulation depth
        property[5] = FQ( parameters[9] ); // Wet/dry mix
    }
    else if( state == 3 ) // diffusion,feedback,regeneration
    {
        property[0] = state; state = 4;
        property[1] = FQ( parameters[7] ); // Diffusion
        property[2] = FQ( parameters[8] ); // Feedback ratio
    }
    else if( state == 4 ) // filtF,filtQ
    {
        double fc = parameters[5]; // Filter frequency
        double qq = parameters[6]; // Filter bandwidth
        
        property[0] = state; state = 5;
        calc_bandpassQ( property+1, 0.001+fc*0.01, 0.1+qq*0.9 );
    }
    else if( state == 5 ) // 
    {
        property[0] = state; state = 1;
    }
}

void c99_mixer( const int usb_output[32], int usb_input[32],
                const int adc_output[32], int dac_input[32],
                const int dsp_output[32], int dsp_input[32], const int property[6] )
{
    dsp_input[1] = dsp_output[1];
}

int _delay_drive = 0;
int _delay_base = 0, _delay_depth = 0, _delay_rate = 0, _delay_blend = 0;
int _delay_diffuse = 0, _delay_fback = 0, _delay_volume = 0;
int _delay_filter_coeff[5], _delay_filter_state[4];

void xio_initialize( void )
{
    mix_fir_coeffs( _delay_upsample_coeff, _delay_dnsample_coeff, 80, 5 );
}

/*
        +-------------------------------------------------------+
        |                                                       |
        |                                                      \|/
input --+--> Delay ------> Modulation ----+--> Filter ---+--> Mixer --->
              /|\                         |
               |                          |
               +<----------Feedback-------+
*/
        
void xio_thread1( int samples[32], const int property[6] )
{
    static int delay_fifo[38400], insert = 0, remove = 0, phase = 0;
    static int samples_dn[5] = {0,0,0,0,0 }, samples_up[5] = {0,0,0,0,0 };
    static int samples_xx[5] = {0,0,0,0,0 };
    
    int count = _delay_base >> 13; // delay_q28 = 0000nnnn,nnnnnnnn,nnnfffff,ffffffff
    if( count > 38400 ) count = 38400;

    samples_dn[4] = samples_dn[3]; samples_dn[3] = samples_dn[2];
    samples_dn[2] = samples_dn[1]; samples_dn[1] = samples_dn[0];
    
    int fdback = dsp_mul( _delay_fback, FQ(0.8) );
    samples_dn[0] = samples[0] + dsp_mul( samples[1], fdback );

    if( phase == 0 ) // Downsample by 5 from 192k to 38.4k
    {
        memcpy( samples_xx, samples_dn, 5 * sizeof(int) );
        _dsp_fir_dn( samples_xx, _delay_dnsample_coeff, _delay_dnsample_state, 80, 5 ); 
    }
    else if( phase == 2 )
    {
        remove = insert + count;              if( remove >= 38400 ) remove -= 38400;
        delay_fifo[insert--] = samples_xx[0]; if( insert < 0 ) insert = 38400-1;
        samples_xx[0] = delay_fifo[remove--]; if( remove < 0 ) remove = 38400-1;
    }
    else if( phase == 3 )
    {
        _dsp_fir_up( samples_xx, _delay_upsample_coeff, _delay_upsample_state, 80, 5 ); 
        memcpy( samples_up, samples_xx, 5 * sizeof(int) );
    }
    if( ++phase == 5 ) phase = 0;

    samples[1] = samples_up[4]; // Delayed signals
    
    samples_up[4] = samples_up[3]; samples_up[3] = samples_up[2];
    samples_up[2] = samples_up[1]; samples_up[1] = samples_up[0];
}

void xio_thread2( int samples[32], const int property[6] )
{    
    //samples[1] = _delay_drive( samples[0], _delay_drive_coeff, _delay_drive_state );
    //samples[1] = samples[0];
}

void xio_thread3( int samples[32], const int property[6] )
{
    int lfo, ii,ff, i1,i2,i3;

    // Generate the LFO signal for delay modulation
    static int time = FQ(0.0); time += _delay_rate; if(time > FQ(1.0)) time -= FQ(1.0);
    ii = (time & 0x0FFFFFFF) >> 18; ff = (time & 0x0003FFFF) << 10;
    lfo = dsp_lagrange( ff, dsp_sine_10[ii+0], dsp_sine_10[ii+1], dsp_sine_10[ii+2] );
    lfo = dsp_mul( lfo, FQ(0.999) ); // Make sure it doesn't overflow beyond +/- 1.0
    
    // Update the sample delay line with input (for chorous) and feedback (for flanger).
    static int delay_fifo[1024], delay_index = 0;
    delay_fifo[delay_index-- & 1023] = samples[1];
    
    // Generate chorus wet signal
    lfo = dsp_mul(lfo,_delay_depth/2) / 2 + _delay_depth / 2;
    ii = (lfo & 0x0FFFFFFF) >> 18; ff = (lfo & 0x0003FFFF) << 10;
    i1 = (delay_index+ii+0)&1023, i2 = (delay_index+ii+1)&1023, i3 = (delay_index+ii+2)&1023;
    samples[2] = dsp_lagrange( ff, delay_fifo[i1], delay_fifo[i2], delay_fifo[i3] );
}

void xio_thread4( int samples[32], const int property[6] )
{
    int lfo, ii,ff, i1,i2,i3;
    int rate = dsp_mul( _delay_rate, one_over_e );
    
    // Generate the LFO signal for delay modulation
    static int time = FQ(0.0); time += rate; if(time > FQ(1.0)) time -= FQ(1.0);
    ii = (time & 0x0FFFFFFF) >> 18; ff = (time & 0x0003FFFF) << 10;
    lfo = dsp_lagrange( ff, dsp_sine_10[ii+0], dsp_sine_10[ii+1], dsp_sine_10[ii+2] );
    lfo = dsp_mul( lfo, FQ(0.999) ); // Make sure it doesn't overflow beyond +/- 1.0

    // Update the sample delay line with input (for chorous #2)
    static int delay_fifo[1024], delay_index = 0;
    delay_fifo[delay_index-- & 1023] = samples[1];

    // Generate chorus #2 wet signal, mix with chorus #1
    lfo = dsp_mul(lfo,_delay_depth/6) / 2 + _delay_depth / 6;
    ii = (lfo & 0x0FFFFFFF) >> 18; ff = (lfo & 0x0003FFFF) << 10;
    i1 = (delay_index+ii+0)&1023, i2 = (delay_index+ii+1)&1023, i3 = (delay_index+ii+2)&1023;
    samples[3] = dsp_lagrange( ff, delay_fifo[i1], delay_fifo[i2], delay_fifo[i3] );
}

void xio_thread5( int samples[32], const int property[6] )
{
    //sample[1] = diffuse( sample[1], _delay_diffuse );

    samples[1] = dsp_iir2( samples[1], _delay_filter_coeff, _delay_filter_state );

    samples[1] = dsp_mul( samples[2], FQ(0.75) ) + dsp_mul( samples[3], FQ(0.25) );
    samples[0] = dsp_blend( samples[0], samples[2], _delay_blend/2 ); // Max is 50%
    samples[0] = dsp_mul( samples[0], _delay_volume );
    
    if( property[0] == 1 ) _delay_volume = property[1];
    if( property[0] == 2 ) {
        _delay_drive = property[1];
        _delay_base = property[2]; _delay_rate = property[3];
        _delay_depth = property[4]; _delay_blend = property[5];
    }
    if( property[0] == 3 ) {
        _delay_diffuse = property[1]; _delay_fback = property[2];
    }
    if( property[0] == 4 ) {
        _delay_filter_coeff[0] = property[1]; _delay_filter_coeff[1] = property[2];
        _delay_filter_coeff[2] = property[3]; _delay_filter_coeff[3] = property[4];
        _delay_filter_coeff[4] = property[5];
    }
}

int _sine_lut[1024] =
{
    FQ(+0.000000000),FQ(+0.006135885),FQ(+0.012271538),FQ(+0.018406730),FQ(+0.024541229),
    FQ(+0.030674803),FQ(+0.036807223),FQ(+0.042938257),FQ(+0.049067674),FQ(+0.055195244),
    FQ(+0.061320736),FQ(+0.067443920),FQ(+0.073564564),FQ(+0.079682438),FQ(+0.085797312),
    FQ(+0.091908956),FQ(+0.098017140),FQ(+0.104121634),FQ(+0.110222207),FQ(+0.116318631),
    FQ(+0.122410675),FQ(+0.128498111),FQ(+0.134580709),FQ(+0.140658239),FQ(+0.146730474),
    FQ(+0.152797185),FQ(+0.158858143),FQ(+0.164913120),FQ(+0.170961889),FQ(+0.177004220),
    FQ(+0.183039888),FQ(+0.189068664),FQ(+0.195090322),FQ(+0.201104635),FQ(+0.207111376),
    FQ(+0.213110320),FQ(+0.219101240),FQ(+0.225083911),FQ(+0.231058108),FQ(+0.237023606),
    FQ(+0.242980180),FQ(+0.248927606),FQ(+0.254865660),FQ(+0.260794118),FQ(+0.266712757),
    FQ(+0.272621355),FQ(+0.278519689),FQ(+0.284407537),FQ(+0.290284677),FQ(+0.296150888),
    FQ(+0.302005949),FQ(+0.307849640),FQ(+0.313681740),FQ(+0.319502031),FQ(+0.325310292),
    FQ(+0.331106306),FQ(+0.336889853),FQ(+0.342660717),FQ(+0.348418680),FQ(+0.354163525),
    FQ(+0.359895037),FQ(+0.365612998),FQ(+0.371317194),FQ(+0.377007410),FQ(+0.382683432),
    FQ(+0.388345047),FQ(+0.393992040),FQ(+0.399624200),FQ(+0.405241314),FQ(+0.410843171),
    FQ(+0.416429560),FQ(+0.422000271),FQ(+0.427555093),FQ(+0.433093819),FQ(+0.438616239),
    FQ(+0.444122145),FQ(+0.449611330),FQ(+0.455083587),FQ(+0.460538711),FQ(+0.465976496),
    FQ(+0.471396737),FQ(+0.476799230),FQ(+0.482183772),FQ(+0.487550160),FQ(+0.492898192),
    FQ(+0.498227667),FQ(+0.503538384),FQ(+0.508830143),FQ(+0.514102744),FQ(+0.519355990),
    FQ(+0.524589683),FQ(+0.529803625),FQ(+0.534997620),FQ(+0.540171473),FQ(+0.545324988),
    FQ(+0.550457973),FQ(+0.555570233),FQ(+0.560661576),FQ(+0.565731811),FQ(+0.570780746),
    FQ(+0.575808191),FQ(+0.580813958),FQ(+0.585797857),FQ(+0.590759702),FQ(+0.595699304),
    FQ(+0.600616479),FQ(+0.605511041),FQ(+0.610382806),FQ(+0.615231591),FQ(+0.620057212),
    FQ(+0.624859488),FQ(+0.629638239),FQ(+0.634393284),FQ(+0.639124445),FQ(+0.643831543),
    FQ(+0.648514401),FQ(+0.653172843),FQ(+0.657806693),FQ(+0.662415778),FQ(+0.666999922),
    FQ(+0.671558955),FQ(+0.676092704),FQ(+0.680600998),FQ(+0.685083668),FQ(+0.689540545),
    FQ(+0.693971461),FQ(+0.698376249),FQ(+0.702754744),FQ(+0.707106781),FQ(+0.711432196),
    FQ(+0.715730825),FQ(+0.720002508),FQ(+0.724247083),FQ(+0.728464390),FQ(+0.732654272),
    FQ(+0.736816569),FQ(+0.740951125),FQ(+0.745057785),FQ(+0.749136395),FQ(+0.753186799),
    FQ(+0.757208847),FQ(+0.761202385),FQ(+0.765167266),FQ(+0.769103338),FQ(+0.773010453),
    FQ(+0.776888466),FQ(+0.780737229),FQ(+0.784556597),FQ(+0.788346428),FQ(+0.792106577),
    FQ(+0.795836905),FQ(+0.799537269),FQ(+0.803207531),FQ(+0.806847554),FQ(+0.810457198),
    FQ(+0.814036330),FQ(+0.817584813),FQ(+0.821102515),FQ(+0.824589303),FQ(+0.828045045),
    FQ(+0.831469612),FQ(+0.834862875),FQ(+0.838224706),FQ(+0.841554977),FQ(+0.844853565),
    FQ(+0.848120345),FQ(+0.851355193),FQ(+0.854557988),FQ(+0.857728610),FQ(+0.860866939),
    FQ(+0.863972856),FQ(+0.867046246),FQ(+0.870086991),FQ(+0.873094978),FQ(+0.876070094),
    FQ(+0.879012226),FQ(+0.881921264),FQ(+0.884797098),FQ(+0.887639620),FQ(+0.890448723),
    FQ(+0.893224301),FQ(+0.895966250),FQ(+0.898674466),FQ(+0.901348847),FQ(+0.903989293),
    FQ(+0.906595705),FQ(+0.909167983),FQ(+0.911706032),FQ(+0.914209756),FQ(+0.916679060),
    FQ(+0.919113852),FQ(+0.921514039),FQ(+0.923879533),FQ(+0.926210242),FQ(+0.928506080),
    FQ(+0.930766961),FQ(+0.932992799),FQ(+0.935183510),FQ(+0.937339012),FQ(+0.939459224),
    FQ(+0.941544065),FQ(+0.943593458),FQ(+0.945607325),FQ(+0.947585591),FQ(+0.949528181),
    FQ(+0.951435021),FQ(+0.953306040),FQ(+0.955141168),FQ(+0.956940336),FQ(+0.958703475),
    FQ(+0.960430519),FQ(+0.962121404),FQ(+0.963776066),FQ(+0.965394442),FQ(+0.966976471),
    FQ(+0.968522094),FQ(+0.970031253),FQ(+0.971503891),FQ(+0.972939952),FQ(+0.974339383),
    FQ(+0.975702130),FQ(+0.977028143),FQ(+0.978317371),FQ(+0.979569766),FQ(+0.980785280),
    FQ(+0.981963869),FQ(+0.983105487),FQ(+0.984210092),FQ(+0.985277642),FQ(+0.986308097),
    FQ(+0.987301418),FQ(+0.988257568),FQ(+0.989176510),FQ(+0.990058210),FQ(+0.990902635),
    FQ(+0.991709754),FQ(+0.992479535),FQ(+0.993211949),FQ(+0.993906970),FQ(+0.994564571),
    FQ(+0.995184727),FQ(+0.995767414),FQ(+0.996312612),FQ(+0.996820299),FQ(+0.997290457),
    FQ(+0.997723067),FQ(+0.998118113),FQ(+0.998475581),FQ(+0.998795456),FQ(+0.999077728),
    FQ(+0.999322385),FQ(+0.999529418),FQ(+0.999698819),FQ(+0.999830582),FQ(+0.999924702),
    FQ(+0.999981175),FQ(+1.000000000),FQ(+0.999981175),FQ(+0.999924702),FQ(+0.999830582),
    FQ(+0.999698819),FQ(+0.999529418),FQ(+0.999322385),FQ(+0.999077728),FQ(+0.998795456),
    FQ(+0.998475581),FQ(+0.998118113),FQ(+0.997723067),FQ(+0.997290457),FQ(+0.996820299),
    FQ(+0.996312612),FQ(+0.995767414),FQ(+0.995184727),FQ(+0.994564571),FQ(+0.993906970),
    FQ(+0.993211949),FQ(+0.992479535),FQ(+0.991709754),FQ(+0.990902635),FQ(+0.990058210),
    FQ(+0.989176510),FQ(+0.988257568),FQ(+0.987301418),FQ(+0.986308097),FQ(+0.985277642),
    FQ(+0.984210092),FQ(+0.983105487),FQ(+0.981963869),FQ(+0.980785280),FQ(+0.979569766),
    FQ(+0.978317371),FQ(+0.977028143),FQ(+0.975702130),FQ(+0.974339383),FQ(+0.972939952),
    FQ(+0.971503891),FQ(+0.970031253),FQ(+0.968522094),FQ(+0.966976471),FQ(+0.965394442),
    FQ(+0.963776066),FQ(+0.962121404),FQ(+0.960430519),FQ(+0.958703475),FQ(+0.956940336),
    FQ(+0.955141168),FQ(+0.953306040),FQ(+0.951435021),FQ(+0.949528181),FQ(+0.947585591),
    FQ(+0.945607325),FQ(+0.943593458),FQ(+0.941544065),FQ(+0.939459224),FQ(+0.937339012),
    FQ(+0.935183510),FQ(+0.932992799),FQ(+0.930766961),FQ(+0.928506080),FQ(+0.926210242),
    FQ(+0.923879533),FQ(+0.921514039),FQ(+0.919113852),FQ(+0.916679060),FQ(+0.914209756),
    FQ(+0.911706032),FQ(+0.909167983),FQ(+0.906595705),FQ(+0.903989293),FQ(+0.901348847),
    FQ(+0.898674466),FQ(+0.895966250),FQ(+0.893224301),FQ(+0.890448723),FQ(+0.887639620),
    FQ(+0.884797098),FQ(+0.881921264),FQ(+0.879012226),FQ(+0.876070094),FQ(+0.873094978),
    FQ(+0.870086991),FQ(+0.867046246),FQ(+0.863972856),FQ(+0.860866939),FQ(+0.857728610),
    FQ(+0.854557988),FQ(+0.851355193),FQ(+0.848120345),FQ(+0.844853565),FQ(+0.841554977),
    FQ(+0.838224706),FQ(+0.834862875),FQ(+0.831469612),FQ(+0.828045045),FQ(+0.824589303),
    FQ(+0.821102515),FQ(+0.817584813),FQ(+0.814036330),FQ(+0.810457198),FQ(+0.806847554),
    FQ(+0.803207531),FQ(+0.799537269),FQ(+0.795836905),FQ(+0.792106577),FQ(+0.788346428),
    FQ(+0.784556597),FQ(+0.780737229),FQ(+0.776888466),FQ(+0.773010453),FQ(+0.769103338),
    FQ(+0.765167266),FQ(+0.761202385),FQ(+0.757208847),FQ(+0.753186799),FQ(+0.749136395),
    FQ(+0.745057785),FQ(+0.740951125),FQ(+0.736816569),FQ(+0.732654272),FQ(+0.728464390),
    FQ(+0.724247083),FQ(+0.720002508),FQ(+0.715730825),FQ(+0.711432196),FQ(+0.707106781),
    FQ(+0.702754744),FQ(+0.698376249),FQ(+0.693971461),FQ(+0.689540545),FQ(+0.685083668),
    FQ(+0.680600998),FQ(+0.676092704),FQ(+0.671558955),FQ(+0.666999922),FQ(+0.662415778),
    FQ(+0.657806693),FQ(+0.653172843),FQ(+0.648514401),FQ(+0.643831543),FQ(+0.639124445),
    FQ(+0.634393284),FQ(+0.629638239),FQ(+0.624859488),FQ(+0.620057212),FQ(+0.615231591),
    FQ(+0.610382806),FQ(+0.605511041),FQ(+0.600616479),FQ(+0.595699304),FQ(+0.590759702),
    FQ(+0.585797857),FQ(+0.580813958),FQ(+0.575808191),FQ(+0.570780746),FQ(+0.565731811),
    FQ(+0.560661576),FQ(+0.555570233),FQ(+0.550457973),FQ(+0.545324988),FQ(+0.540171473),
    FQ(+0.534997620),FQ(+0.529803625),FQ(+0.524589683),FQ(+0.519355990),FQ(+0.514102744),
    FQ(+0.508830143),FQ(+0.503538384),FQ(+0.498227667),FQ(+0.492898192),FQ(+0.487550160),
    FQ(+0.482183772),FQ(+0.476799230),FQ(+0.471396737),FQ(+0.465976496),FQ(+0.460538711),
    FQ(+0.455083587),FQ(+0.449611330),FQ(+0.444122145),FQ(+0.438616239),FQ(+0.433093819),
    FQ(+0.427555093),FQ(+0.422000271),FQ(+0.416429560),FQ(+0.410843171),FQ(+0.405241314),
    FQ(+0.399624200),FQ(+0.393992040),FQ(+0.388345047),FQ(+0.382683432),FQ(+0.377007410),
    FQ(+0.371317194),FQ(+0.365612998),FQ(+0.359895037),FQ(+0.354163525),FQ(+0.348418680),
    FQ(+0.342660717),FQ(+0.336889853),FQ(+0.331106306),FQ(+0.325310292),FQ(+0.319502031),
    FQ(+0.313681740),FQ(+0.307849640),FQ(+0.302005949),FQ(+0.296150888),FQ(+0.290284677),
    FQ(+0.284407537),FQ(+0.278519689),FQ(+0.272621355),FQ(+0.266712757),FQ(+0.260794118),
    FQ(+0.254865660),FQ(+0.248927606),FQ(+0.242980180),FQ(+0.237023606),FQ(+0.231058108),
    FQ(+0.225083911),FQ(+0.219101240),FQ(+0.213110320),FQ(+0.207111376),FQ(+0.201104635),
    FQ(+0.195090322),FQ(+0.189068664),FQ(+0.183039888),FQ(+0.177004220),FQ(+0.170961889),
    FQ(+0.164913120),FQ(+0.158858143),FQ(+0.152797185),FQ(+0.146730474),FQ(+0.140658239),
    FQ(+0.134580709),FQ(+0.128498111),FQ(+0.122410675),FQ(+0.116318631),FQ(+0.110222207),
    FQ(+0.104121634),FQ(+0.098017140),FQ(+0.091908956),FQ(+0.085797312),FQ(+0.079682438),
    FQ(+0.073564564),FQ(+0.067443920),FQ(+0.061320736),FQ(+0.055195244),FQ(+0.049067674),
    FQ(+0.042938257),FQ(+0.036807223),FQ(+0.030674803),FQ(+0.024541229),FQ(+0.018406730),
    FQ(+0.012271538),FQ(+0.006135885),FQ(+0.000000000),FQ(-0.006135885),FQ(-0.012271538),
    FQ(-0.018406730),FQ(-0.024541229),FQ(-0.030674803),FQ(-0.036807223),FQ(-0.042938257),
    FQ(-0.049067674),FQ(-0.055195244),FQ(-0.061320736),FQ(-0.067443920),FQ(-0.073564564),
    FQ(-0.079682438),FQ(-0.085797312),FQ(-0.091908956),FQ(-0.098017140),FQ(-0.104121634),
    FQ(-0.110222207),FQ(-0.116318631),FQ(-0.122410675),FQ(-0.128498111),FQ(-0.134580709),
    FQ(-0.140658239),FQ(-0.146730474),FQ(-0.152797185),FQ(-0.158858143),FQ(-0.164913120),
    FQ(-0.170961889),FQ(-0.177004220),FQ(-0.183039888),FQ(-0.189068664),FQ(-0.195090322),
    FQ(-0.201104635),FQ(-0.207111376),FQ(-0.213110320),FQ(-0.219101240),FQ(-0.225083911),
    FQ(-0.231058108),FQ(-0.237023606),FQ(-0.242980180),FQ(-0.248927606),FQ(-0.254865660),
    FQ(-0.260794118),FQ(-0.266712757),FQ(-0.272621355),FQ(-0.278519689),FQ(-0.284407537),
    FQ(-0.290284677),FQ(-0.296150888),FQ(-0.302005949),FQ(-0.307849640),FQ(-0.313681740),
    FQ(-0.319502031),FQ(-0.325310292),FQ(-0.331106306),FQ(-0.336889853),FQ(-0.342660717),
    FQ(-0.348418680),FQ(-0.354163525),FQ(-0.359895037),FQ(-0.365612998),FQ(-0.371317194),
    FQ(-0.377007410),FQ(-0.382683432),FQ(-0.388345047),FQ(-0.393992040),FQ(-0.399624200),
    FQ(-0.405241314),FQ(-0.410843171),FQ(-0.416429560),FQ(-0.422000271),FQ(-0.427555093),
    FQ(-0.433093819),FQ(-0.438616239),FQ(-0.444122145),FQ(-0.449611330),FQ(-0.455083587),
    FQ(-0.460538711),FQ(-0.465976496),FQ(-0.471396737),FQ(-0.476799230),FQ(-0.482183772),
    FQ(-0.487550160),FQ(-0.492898192),FQ(-0.498227667),FQ(-0.503538384),FQ(-0.508830143),
    FQ(-0.514102744),FQ(-0.519355990),FQ(-0.524589683),FQ(-0.529803625),FQ(-0.534997620),
    FQ(-0.540171473),FQ(-0.545324988),FQ(-0.550457973),FQ(-0.555570233),FQ(-0.560661576),
    FQ(-0.565731811),FQ(-0.570780746),FQ(-0.575808191),FQ(-0.580813958),FQ(-0.585797857),
    FQ(-0.590759702),FQ(-0.595699304),FQ(-0.600616479),FQ(-0.605511041),FQ(-0.610382806),
    FQ(-0.615231591),FQ(-0.620057212),FQ(-0.624859488),FQ(-0.629638239),FQ(-0.634393284),
    FQ(-0.639124445),FQ(-0.643831543),FQ(-0.648514401),FQ(-0.653172843),FQ(-0.657806693),
    FQ(-0.662415778),FQ(-0.666999922),FQ(-0.671558955),FQ(-0.676092704),FQ(-0.680600998),
    FQ(-0.685083668),FQ(-0.689540545),FQ(-0.693971461),FQ(-0.698376249),FQ(-0.702754744),
    FQ(-0.707106781),FQ(-0.711432196),FQ(-0.715730825),FQ(-0.720002508),FQ(-0.724247083),
    FQ(-0.728464390),FQ(-0.732654272),FQ(-0.736816569),FQ(-0.740951125),FQ(-0.745057785),
    FQ(-0.749136395),FQ(-0.753186799),FQ(-0.757208847),FQ(-0.761202385),FQ(-0.765167266),
    FQ(-0.769103338),FQ(-0.773010453),FQ(-0.776888466),FQ(-0.780737229),FQ(-0.784556597),
    FQ(-0.788346428),FQ(-0.792106577),FQ(-0.795836905),FQ(-0.799537269),FQ(-0.803207531),
    FQ(-0.806847554),FQ(-0.810457198),FQ(-0.814036330),FQ(-0.817584813),FQ(-0.821102515),
    FQ(-0.824589303),FQ(-0.828045045),FQ(-0.831469612),FQ(-0.834862875),FQ(-0.838224706),
    FQ(-0.841554977),FQ(-0.844853565),FQ(-0.848120345),FQ(-0.851355193),FQ(-0.854557988),
    FQ(-0.857728610),FQ(-0.860866939),FQ(-0.863972856),FQ(-0.867046246),FQ(-0.870086991),
    FQ(-0.873094978),FQ(-0.876070094),FQ(-0.879012226),FQ(-0.881921264),FQ(-0.884797098),
    FQ(-0.887639620),FQ(-0.890448723),FQ(-0.893224301),FQ(-0.895966250),FQ(-0.898674466),
    FQ(-0.901348847),FQ(-0.903989293),FQ(-0.906595705),FQ(-0.909167983),FQ(-0.911706032),
    FQ(-0.914209756),FQ(-0.916679060),FQ(-0.919113852),FQ(-0.921514039),FQ(-0.923879533),
    FQ(-0.926210242),FQ(-0.928506080),FQ(-0.930766961),FQ(-0.932992799),FQ(-0.935183510),
    FQ(-0.937339012),FQ(-0.939459224),FQ(-0.941544065),FQ(-0.943593458),FQ(-0.945607325),
    FQ(-0.947585591),FQ(-0.949528181),FQ(-0.951435021),FQ(-0.953306040),FQ(-0.955141168),
    FQ(-0.956940336),FQ(-0.958703475),FQ(-0.960430519),FQ(-0.962121404),FQ(-0.963776066),
    FQ(-0.965394442),FQ(-0.966976471),FQ(-0.968522094),FQ(-0.970031253),FQ(-0.971503891),
    FQ(-0.972939952),FQ(-0.974339383),FQ(-0.975702130),FQ(-0.977028143),FQ(-0.978317371),
    FQ(-0.979569766),FQ(-0.980785280),FQ(-0.981963869),FQ(-0.983105487),FQ(-0.984210092),
    FQ(-0.985277642),FQ(-0.986308097),FQ(-0.987301418),FQ(-0.988257568),FQ(-0.989176510),
    FQ(-0.990058210),FQ(-0.990902635),FQ(-0.991709754),FQ(-0.992479535),FQ(-0.993211949),
    FQ(-0.993906970),FQ(-0.994564571),FQ(-0.995184727),FQ(-0.995767414),FQ(-0.996312612),
    FQ(-0.996820299),FQ(-0.997290457),FQ(-0.997723067),FQ(-0.998118113),FQ(-0.998475581),
    FQ(-0.998795456),FQ(-0.999077728),FQ(-0.999322385),FQ(-0.999529418),FQ(-0.999698819),
    FQ(-0.999830582),FQ(-0.999924702),FQ(-0.999981175),FQ(-1.000000000),FQ(-0.999981175),
    FQ(-0.999924702),FQ(-0.999830582),FQ(-0.999698819),FQ(-0.999529418),FQ(-0.999322385),
    FQ(-0.999077728),FQ(-0.998795456),FQ(-0.998475581),FQ(-0.998118113),FQ(-0.997723067),
    FQ(-0.997290457),FQ(-0.996820299),FQ(-0.996312612),FQ(-0.995767414),FQ(-0.995184727),
    FQ(-0.994564571),FQ(-0.993906970),FQ(-0.993211949),FQ(-0.992479535),FQ(-0.991709754),
    FQ(-0.990902635),FQ(-0.990058210),FQ(-0.989176510),FQ(-0.988257568),FQ(-0.987301418),
    FQ(-0.986308097),FQ(-0.985277642),FQ(-0.984210092),FQ(-0.983105487),FQ(-0.981963869),
    FQ(-0.980785280),FQ(-0.979569766),FQ(-0.978317371),FQ(-0.977028143),FQ(-0.975702130),
    FQ(-0.974339383),FQ(-0.972939952),FQ(-0.971503891),FQ(-0.970031253),FQ(-0.968522094),
    FQ(-0.966976471),FQ(-0.965394442),FQ(-0.963776066),FQ(-0.962121404),FQ(-0.960430519),
    FQ(-0.958703475),FQ(-0.956940336),FQ(-0.955141168),FQ(-0.953306040),FQ(-0.951435021),
    FQ(-0.949528181),FQ(-0.947585591),FQ(-0.945607325),FQ(-0.943593458),FQ(-0.941544065),
    FQ(-0.939459224),FQ(-0.937339012),FQ(-0.935183510),FQ(-0.932992799),FQ(-0.930766961),
    FQ(-0.928506080),FQ(-0.926210242),FQ(-0.923879533),FQ(-0.921514039),FQ(-0.919113852),
    FQ(-0.916679060),FQ(-0.914209756),FQ(-0.911706032),FQ(-0.909167983),FQ(-0.906595705),
    FQ(-0.903989293),FQ(-0.901348847),FQ(-0.898674466),FQ(-0.895966250),FQ(-0.893224301),
    FQ(-0.890448723),FQ(-0.887639620),FQ(-0.884797098),FQ(-0.881921264),FQ(-0.879012226),
    FQ(-0.876070094),FQ(-0.873094978),FQ(-0.870086991),FQ(-0.867046246),FQ(-0.863972856),
    FQ(-0.860866939),FQ(-0.857728610),FQ(-0.854557988),FQ(-0.851355193),FQ(-0.848120345),
    FQ(-0.844853565),FQ(-0.841554977),FQ(-0.838224706),FQ(-0.834862875),FQ(-0.831469612),
    FQ(-0.828045045),FQ(-0.824589303),FQ(-0.821102515),FQ(-0.817584813),FQ(-0.814036330),
    FQ(-0.810457198),FQ(-0.806847554),FQ(-0.803207531),FQ(-0.799537269),FQ(-0.795836905),
    FQ(-0.792106577),FQ(-0.788346428),FQ(-0.784556597),FQ(-0.780737229),FQ(-0.776888466),
    FQ(-0.773010453),FQ(-0.769103338),FQ(-0.765167266),FQ(-0.761202385),FQ(-0.757208847),
    FQ(-0.753186799),FQ(-0.749136395),FQ(-0.745057785),FQ(-0.740951125),FQ(-0.736816569),
    FQ(-0.732654272),FQ(-0.728464390),FQ(-0.724247083),FQ(-0.720002508),FQ(-0.715730825),
    FQ(-0.711432196),FQ(-0.707106781),FQ(-0.702754744),FQ(-0.698376249),FQ(-0.693971461),
    FQ(-0.689540545),FQ(-0.685083668),FQ(-0.680600998),FQ(-0.676092704),FQ(-0.671558955),
    FQ(-0.666999922),FQ(-0.662415778),FQ(-0.657806693),FQ(-0.653172843),FQ(-0.648514401),
    FQ(-0.643831543),FQ(-0.639124445),FQ(-0.634393284),FQ(-0.629638239),FQ(-0.624859488),
    FQ(-0.620057212),FQ(-0.615231591),FQ(-0.610382806),FQ(-0.605511041),FQ(-0.600616479),
    FQ(-0.595699304),FQ(-0.590759702),FQ(-0.585797857),FQ(-0.580813958),FQ(-0.575808191),
    FQ(-0.570780746),FQ(-0.565731811),FQ(-0.560661576),FQ(-0.555570233),FQ(-0.550457973),
    FQ(-0.545324988),FQ(-0.540171473),FQ(-0.534997620),FQ(-0.529803625),FQ(-0.524589683),
    FQ(-0.519355990),FQ(-0.514102744),FQ(-0.508830143),FQ(-0.503538384),FQ(-0.498227667),
    FQ(-0.492898192),FQ(-0.487550160),FQ(-0.482183772),FQ(-0.476799230),FQ(-0.471396737),
    FQ(-0.465976496),FQ(-0.460538711),FQ(-0.455083587),FQ(-0.449611330),FQ(-0.444122145),
    FQ(-0.438616239),FQ(-0.433093819),FQ(-0.427555093),FQ(-0.422000271),FQ(-0.416429560),
    FQ(-0.410843171),FQ(-0.405241314),FQ(-0.399624200),FQ(-0.393992040),FQ(-0.388345047),
    FQ(-0.382683432),FQ(-0.377007410),FQ(-0.371317194),FQ(-0.365612998),FQ(-0.359895037),
    FQ(-0.354163525),FQ(-0.348418680),FQ(-0.342660717),FQ(-0.336889853),FQ(-0.331106306),
    FQ(-0.325310292),FQ(-0.319502031),FQ(-0.313681740),FQ(-0.307849640),FQ(-0.302005949),
    FQ(-0.296150888),FQ(-0.290284677),FQ(-0.284407537),FQ(-0.278519689),FQ(-0.272621355),
    FQ(-0.266712757),FQ(-0.260794118),FQ(-0.254865660),FQ(-0.248927606),FQ(-0.242980180),
    FQ(-0.237023606),FQ(-0.231058108),FQ(-0.225083911),FQ(-0.219101240),FQ(-0.213110320),
    FQ(-0.207111376),FQ(-0.201104635),FQ(-0.195090322),FQ(-0.189068664),FQ(-0.183039888),
    FQ(-0.177004220),FQ(-0.170961889),FQ(-0.164913120),FQ(-0.158858143),FQ(-0.152797185),
    FQ(-0.146730474),FQ(-0.140658239),FQ(-0.134580709),FQ(-0.128498111),FQ(-0.122410675),
    FQ(-0.116318631),FQ(-0.110222207),FQ(-0.104121634),FQ(-0.098017140),FQ(-0.091908956),
    FQ(-0.085797312),FQ(-0.079682438),FQ(-0.073564564),FQ(-0.067443920),FQ(-0.061320736),
    FQ(-0.055195244),FQ(-0.049067674),FQ(-0.042938257),FQ(-0.036807223),FQ(-0.030674803),
    FQ(-0.024541229),FQ(-0.018406730),FQ(-0.012271538),FQ(-0.006135885)
};
