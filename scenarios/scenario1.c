#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <liquid/liquid.h>

// CRTS Header Files
#include "scenarios/scenario1.h"


void scenario1(float complex * transmit_buffer, unsigned int symbol_len)
{
    //printf("\nscenario1 called.\n\n");

    //options
    float dphi  = 0.001f;                   // carrier frequency offset
    float SNRdB = 7.0f;                    // signal-to-noise ratio [dB]
    
    // noise parameters
    float nstd = powf(10.0f, -SNRdB/20.0f); // noise standard deviation
    float phi = 0.0f;                       // channel phase

    //float complex buffer[symbol_len]; 

    unsigned int i=0;
    // noise mixing
    for (i=0; i<symbol_len; i++) {
        transmit_buffer[i] *= cexpf(_Complex_I*phi); // apply carrier offset
        phi += dphi;                        // update carrier phase
        cawgn(&transmit_buffer[i], nstd);            // add noise
    }

}
	
