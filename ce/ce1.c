#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <liquid/liquid.h>
#include <time.h>
// For Threading (POSIX Threads)
#include <pthread.h>

//TCP Header Files
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <sys/types.h>
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <errno.h>

// CRTS Header Files
#include "ce/ce1.h"
#include "scenarios/scenario1.h"
#include "tx_JC.h"


int ce1(/*float complex * samples_buffer,*/ float * fb_buffer)
{
    printf("Cognitive Engine is initializing...\n");

    // Iterators
    int i = 0;
    int j = 0;

    // (Constant) Parameters for Frames
    unsigned int M = 64; // number of subcarriers
    unsigned int cp_len = 16; // cyclic prefix length
    unsigned int taper_len = 4; // taper length
    unsigned int payload_len = 120; // length of payload (bytes)

    // allocate memory for header, payload, sample buffer
    unsigned int symbol_len = M + cp_len; // samples per OFDM symbol
    float complex samples_buffer[symbol_len]; // time-domain buffer
    unsigned char header[8]; // header
    unsigned char payload[payload_len]; // payload

    printf("Setting frame generation parameters...\n");
    // Frame generation parameters
    crc_scheme check = LIQUID_CRC_32;
    fec_scheme fec0  = LIQUID_FEC_NONE;
    fec_scheme fec1  = LIQUID_FEC_HAMMING128;
    modulation_scheme ms = LIQUID_MODEM_QPSK;
    ofdmflexframegenprops_s fgprops;

    printf("Initializing fg and fs\n");
    // Initialize Frame generator and Frame Synchronizer Objects
    ofdmflexframegenprops_init_default(&fgprops);
    fgprops.check           = check;
    fgprops.fec0            = fec0;
    fgprops.fec1            = fec1;
    fgprops.mod_scheme      = ms;
    ofdmflexframegen fg = ofdmflexframegen_create(M, cp_len, taper_len, NULL, &fgprops);
    ofdmflexframesync fs =
        ofdmflexframesync_create(M, cp_len, taper_len, NULL, mycallback, (void *) fb_buffer);

    printf("fg and fs initializtion complete.\n");

    // array to copy data from fb_buffer to
    float fb_data[8];

    // Do until test is complete.
    int stop_tx = 0;
    while (!stop_tx)
    {

        
        // Generate data
        printf("generating data that will go in frame...\n");
        for (i=0; i<8; i++)
            header[i] = i & 0xff;
        for (i=0; i<payload_len; i++)
            payload[i] = rand() & 0xff;

        // Assemble frame
        printf("Assembling frame...\n");
        ofdmflexframegen_assemble(fg, header, payload, payload_len);

        // Write symbols from assembled frame to buffer 
        int last_symbol = 0;
        printf("Writing frame samples to samples_buffer...\n");
        while (!last_symbol) {
            // generate symbol
            //printf("Writing symbol.\n");
            last_symbol = ofdmflexframegen_writesymbol(fg, samples_buffer);
            
            // TODO: Call to scenario (i.e. add noise)
            
            // synchronizer receives symbols
            //printf("receiving symbol.\n");
            ofdmflexframesync_execute(fs, samples_buffer, symbol_len);

        }
        printf("Finished writing samples");

        // Delay to give server time to enter feedback data into fb_buffer
        sleep(1);

        // Receive feedback via TCP
        printf("Receiving feedback via fb_buffer...\n");
        for (i=0; i<8; i++)
            fb_data[i] = fb_buffer[i];
        for (i=0; i<8; i++)
            printf("fbdata[%d]= %f\n", i, fb_data[i]);

        // Determine new FG parameters


        // Set new FG parameters and create new FG
        ofdmflexframegenprops_init_default(&fgprops);
        fgprops.check           = check;
        fgprops.fec0            = fec0;
        fgprops.fec1            = fec1;
        fgprops.mod_scheme      = ms;
        ofdmflexframegen fg = ofdmflexframegen_create(M, cp_len, taper_len, NULL, &fgprops);

    }

    //  Destroy framegen and framesync objects
    ofdmflexframegen_destroy(fg);
    ofdmflexframesync_destroy(fs);

    return 0;

}




        

