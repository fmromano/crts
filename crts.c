#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <liquid/liquid.h>
#include <time.h>
#include <string.h>
// For Threading (POSIX Threads)
#include <pthread.h>


// CRTS Header Files
#include "ce/ce1.h"
#include "tx_JC.h"

//TCP Header Files
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <sys/types.h>
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <errno.h>
#define PORT 1351


struct CognitiveEngine {
    char ModScheme[20];
    float default_tx_power;
    char option_to_adapt[20];
    char goal[20];
    float threshold;
    float latestGoalValue;
    int iterations;
    int payloadLen;
}

struct Scenario {
    int addNoise; //Does the Scenario have noise?
    int noiseSNR;
    int noise dPhi;
    
    int addInterference; // Does the Scenario have interference?
    
    int addFading; // Does the Secenario have fading?


}

// Default parameters for a Cognitive Engine
struct CognitiveEngine CreateCognitiveEngine() {
    struct CognitiveEngine ce = {
        //.default_mod_scheme = "QPSK";
        strcpy(.mod_scheme, "QPSK");
        .default_tx_power = 10.0;
        //.option_to_adapt = "mod_scheme";
        strcpy(.option_to_adapt, "mod_scheme");
        //.goal = "ber";                    // parameter to be minimized/maximized
        strcpy(.goal, "ber");
        .threshold = 0.0;                 // Desired value for goal
        .latestGoalValue = 0.0;           // Value of goal to be compared to threshold
        .iterations = 100;                // Number of transmissions made before attemting to receive them.
        .payloadLen = 120;                      // Length of payload in frame generation
    }
    return ce;
}

// Default parameter for Scenario
struct Scenario CreateScenario() {
    struct Scenario sc = {
        .addNoise = 1;
        .noiseSNR = 7; // in dB
        .noise dPhi = 0.01; 

        .addInterference = 0;

        .addFading = 0;
    }
    return sc;
}

// Create Frame generator with initial CE and Scenario parameters
ofdmflexframegen CreateFG(struct CognitiveEngine ce, struct Scenario sc) {

    // Set Modulation Scheme
    modulation_scheme ms;
    if (strcmp(ce.mod_scheme, "QPSK") == 0) {
        ms = LIQUID_MODEM_QPSK;
    }
    else if ( strcmp(ce.mod_scheme, "BPSK") ==0) {
        ms = LIQUID_MODEM_BPSK;
    }
    else {
        printf("ERROR: Unkown Modulation Scheme");
        //TODO: Skip current test if given an unkown parameter.
    }

    // TODO: Have all parameters be set by ce and se objects.
    
    // Frame generation parameters
    crc_scheme check = LIQUID_CRC_32;
    fec_scheme fec0  = LIQUID_FEC_NONE;
    fec_scheme fec1  = LIQUID_FEC_HAMMING128;
    ofdmflexframegenprops_s fgprops;

    // Initialize Frame generator and Frame Synchronizer Objects
    ofdmflexframegenprops_init_default(&fgprops);
    fgprops.check           = check;
    fgprops.fec0            = fec0;
    fgprops.fec1            = fec1;
    fgprops.mod_scheme      = ms;
    ofdmflexframegen fg = ofdmflexframegen_create(M, cp_len, taper_len, NULL, &fgprops);

    return fg;
}

// Transmit a packet of data.
// This will need to be modified once we implement the USRPs.
int txGeneratePacket(struct CognitiveEngine ce, ofdmflexframegen * _fg, char * header, char * payload)
{
    // Buffers for data
    //char header[8];
    //char payload[ce.payloadLen];

    // Generate data
    printf("\n\n\ngenerating data that will go in frame...\n");
    for (i=0; i<8; i++)
    header[i] = i & 0xff;
    for (i=0; i<payload_len; i++)
    payload[i] = rand() & 0xff;

    // Assemble frame
    ofdmflexframegen_assemble(fg, header, payload, ce.payloadLen);

    return 1;
} // End txTransmitPacket()

int main()
{
    // Number of Cognitive Engines
    int NumCE = 3;
    int NumSc = 3;

    // List of Cognitive Engines 
    char CEList[NumCE][20];         // char arrays of 20 chars each.
    strcpy(CEList[0], "ce1.conf");
    strcpy(CEList[1], "ce2.conf");
    strcpy(CEList[2], "ce3.conf");

    // List of Scenarios
    char ScList[NumSc][20];         // char arrays of 20 chars each.
    strcpy(ScList[0], "Sc1.conf");
    strcpy(ScList[1], "Sc2.conf");
    strcpy(ScList[2], "Sc3.conf");

    // Iterators
    int i_CE = 0;
    int i_Sc = 0;
    int DoneTransmitting = 0;
    int N = 0; // Iterations of transmission before receiving.
    int i_N = 0;

    // Cognitive engine struct used in each test
    struct CognitiveEngine ce = CreateCognitiveEngine();
    // Scenario struct used in each test
    struct Scenario sc = CreateScenario();
    // framegenerator object used in each test
    ofdmflexframe fg;

    // Buffers for packet/frame data
    char header[8];             // Must always be 8 bytes for ofdmflexframe
    char payload[1000];         // Large enough to accomodate any (reasonable) payload that
                                // the CE wants to use.
    float complex frameSamples[10000];   // Large enough to accomodate any (reasonable) payload that 
                                // the CE wants to use.

    // For each Cognitive Engine
    for (i_CE=0; i<NumCE; i_CE++)
    {
        // Initialize current CE
        ce = CreateCognitiveEngine();
        // TODO: Implemenet reading from configuration files

        // Run each CE through each scenario
        for (i_Sc= 0; Sci<NumSc; i_Sc++)
        {
            // Initialize current Scenario
            sc = CreateScenario();
            // TODO: Implement reading from config files

            // Initialize Transmitter Defaults for current CE and Sc
            fg = CreateFG(ce, sc);  // Create ofdmflexframegen object with given parameters
                //TODO: Initialize Connection to USRP                                     

            // Begin Testing Scenario
            DoneTransmitting = 0;
            while(!DoneTransmitting)
            {
                // Generate data to go into frame (packet)
                txGeneratePacket(ce, &fg, header, payload);
                // Simulate N tranmissions before simulating receiving them.
                N = ce.iterations;
                for (i_N= 0; i_N< N; i_N++)
                {
                    // TODO: Create this function
                    txTransmitPacket(ce, &fg);
                    // TODO: Create this function
                    //noiseAddNoise();
                    // TODO: Create this function
                    // Store a copy of the packet that was transmitted. For reference.
                    // txStoreTransmittedPacket();
                
                } // End Transmition For loop

                // Rx Receives packet

                // Receive and analyze data from rx
                    // Modify transmission parameters (in fg and in USRP) accordingly

            } // End Test While loop
            
        } // End Scenario For loop

    } // End CE for loop

    return 0;
}








