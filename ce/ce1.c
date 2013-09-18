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


int ce1(/*float complex * samples_buffer,*/ float * fb_buffer)
{
    // Iterators
    int i = 0;
    int j = 0;
    unsigned int M = 64; // number of subcarriers
    unsigned int cp_len = 16; // cyclic prefix length
    unsigned int taper_len = 4; // taper length
    unsigned int payload_len = 120; // length of payload (bytes)

    // allocate memory for header, payload, sample buffer
    unsigned int symbol_len = M + cp_len; // samples per OFDM symbol
    float complex samples_buffer[symbol_len]; // time-domain buffer
    unsigned char header[8]; // header
    unsigned char payload[payload_len]; // payload

    // Frame generation parameters
    crc_scheme check = LIQUID_CRC_32;
    fec_scheme fec0  = LIQUID_FEC_NONE;
    fec_scheme fec1  = LIQUID_FEC_HAMMING128;
    modulation_scheme ms = LIQUID_MODEM_QPSK;
    ofdmflexframegenprops_s fgprops;

    // array to copy data from fb_buffer to
    float fb_data[8];

    // Initialize frame synchronizer
    framesync64 fs = framesync64_create(mycallback,
        (void*)feedback);

    // Do until test is complete.
    int stop_tx = 0;
    while (!stop_tx)
    {
        // create (new) frame generator
        ofdmflexframegenprops_init_default(&fgprops);
        fgprops.check           = check;
        fgprops.fec0            = fec0;
        fgprops.fec1            = fec1;
        fgprops.mod_scheme      = ms;
        ofdmflexframegen fg = ofdmflexframegen_create(M, cp_len, taper_len, NULL, &fgprops);

        // Generate data
        for (i=0; i<8; i++)
            header[i] = i & 0xff;
        for (i=0; i<payload_len; i++)
            payload[i] = rand() & 0xff;

        // Assemble frame
        ofdmflexframegen_assemble(fg, header, payload, payload_len);

        // Write symbols from assembled frame to buffer 
        int last_symbol = 0;
        while (!last_symbol) {
            // generate symbol
            last_symbol = ofdmflexframegen_writesymbol(fg, samples_buffer);
            
            // TODO: Call to scenario (i.e. add noise)

            // synchronizer receives symbols
            ofdmflexframesync_execute(fs, samples_buffer, symbol_len);

        }

        // Delay to give server time to enter feedback data into fb_buffer
        sleep(1);

        // Receive feedback via TCP
        for (i=0; i<8; i++)
            fb_data[i] = fb_buffer[i];
        for (i=0; i<8; i++)
            printf("fbdata[%d]= %f", i, fbdata[i]);

        // Determine new FG parameters

    }

    //  Destroy framegen and framesync objects
    ofdmflexframegen_destroy(fg);
    ofdmflexframesync_destroy(fs);


}




        

