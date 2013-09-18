#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <liquid/liquid.h>

// CRTS Header Files
#include "scenarios/scenario1.h"

void scenario1(float complex * transmit_buffer, unsigned int symbol_len)
{
    //options
    float dphi  = 0.001f;                   // carrier frequency offset
    float SNRdB = 20.0f;                    // signal-to-noise ratio [dB]

    // noise parameters
    float nstd = powf(10.0f, -SNRdB/20.0f); // noise standard deviation
    float phi = 0.0f;                       // channel phase

    int last_symbol=0;

    unsigned int i;

    // noise mixing
    for (i=0; i<symbol_len; i++) {
        transmit_buffer[i] *= cexpf(_Complex_I*phi); // apply carrier offset
        phi += dphi;                        // update carrier phase
        cawgn(&buffer[i], nstd);            // add noise
    }
    
    return 0;
}
	
