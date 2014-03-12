#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex>
#include <liquid/liquid.h>
// Definition of liquid_float_complex changes depending on
// whether <complex> is included before or after liquid.h
#include <liquid/ofdmtxrx.h>
#include <time.h>
#include <string.h>
// For Threading (POSIX Threads)
#include <pthread.h>
// For config file
#include <libconfig.h>

//TCP Header Files
#include <sys/socket.h> // for socket(), connect(), send(), and recv() 
#include <sys/types.h>
#include <arpa/inet.h>  // for sockaddr_in and inet_addr() 
#include <string.h>     // for memset() 
#include <unistd.h>     // for close() 
#include <errno.h>
#include <uhd/usrp/multi_usrp.hpp>
#define MAXPENDING 5


void usage() {
    printf("crts_rx -- Rx component for crts when using USRP's.\n");
    printf("  -u,-h  :   usage/help\n");
    //printf("  -q     :   quiet - do not print debug info\n");
    //printf("  -v     :   verbose - print debug info to stdout (default)\n");
    //printf("  -d     :   print data to stdout rather than to file (implies -q unless -v given)\n");
    //printf("  -r     :   real transmissions using USRPs (opposite of -s)\n");
    //printf("  -s     :   simulation mode (default)\n");
    printf("  -a     :   server IP address (default: 127.0.0.1)\n");
    printf("  -p     :   server port (default: 1402)\n");
    //printf("  f     :   center frequency [Hz], default: 462 MHz\n");
    //printf("  b     :   bandwidth [Hz], default: 250 kHz\n");
    //printf("  G     :   uhd rx gain [dB] (default: 20dB)\n");
    //printf("  t     :   run time [seconds]\n");
    //printf("  z     :   number of subcarriers to notch in the center band, default: 0\n");
}

// TODO: Send these to their respective functions
struct rxCBstruct {
    unsigned int serverPort;
    float bandwidth;
    char * serverAddr;
};

// Defaults for struct that is sent to rxCallBack()
struct rxCBstruct CreaterxCBStruct() {
    struct rxCBstruct rxCB = {};

    rxCB.serverPort = 1402;
    rxCB.bandwidth = 1.0e6;
    rxCB.serverAddr = (char*) "127.0.0.1";

    return rxCB;
} // End CreaterxCBStruct()

int rxCallback(unsigned char *  _header,
                int              _header_valid,
                unsigned char *  _payload,
                unsigned int     _payload_len,
                int              _payload_valid,
                framesyncstats_s _stats,
                void *           _userdata)
{
    struct rxCBstruct * rxCBs_ptr = (struct rxCBstruct*) _userdata;

    // Iterator
    int i = 0;
    float feedback[8];

    // Data that will be sent to server

    // Create a client TCP socket
    int socket_to_server = socket(AF_INET, SOCK_STREAM, 0); 
    if( socket_to_server < 0)
    {   
        printf("Receiver Failed to Create Client Socket\n");
        exit(1);
    }   
    printf("Created client socket to server. socket_to_server: %d\n", socket_to_server);

    // Parameters for connecting to server
    // TODO: Allow selection of IP address and port in command line parameters.
    // TODO: Set to choose any local port
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(rxCBs_ptr->serverPort);
    servAddr.sin_addr.s_addr = inet_addr(rxCBs_ptr->serverAddr);

    // Attempt to connect client socket to server
    int connect_status;
    if((connect_status = connect(socket_to_server, (struct sockaddr*)&servAddr, sizeof(servAddr))))
    {   
        printf("Receive Failed to Connect to server.\n");
        printf("connect_status = %d\n", connect_status);
        exit(1);
    }
   
    framesyncstats_print(&_stats); 

    // Check number of bit errors in packet 
    float headerErrors = 0.0; 
    float payloadErrors = 0.0; 

    for (i=0; i<8; i++) 
    {    
        if (!(_header[i] == (i & 0xff))) {
        headerErrors++;
        }
    }    


    for (i=0; i<(signed int)_payload_len; i++) 
    {    
        if (!(_payload[i] == (i & 0xff))) {
        payloadErrors++;
        }
    }    


    // TODO: Send other useful data through feedback array
    feedback[0] = (float) _header_valid;
    feedback[1] = (float) _payload_valid;
    feedback[2] = (float) _stats.evm;
    feedback[3] = (float) _stats.rssi;   
    if (_header_valid)
    {
        feedback[4] = * (float *) _header;
    }
    else
    {
        feedback[4] = -1;
    }
    //feedback[4] = 0.0; // TODO: Find a way to let crts_rx count frames received but start over with each scenario
    feedback[5] = headerErrors;
    feedback[6] = payloadErrors;
   
    for (i=0; i<7; i++)
    printf("feedback data before transmission: %f\n", feedback[i]);

    // Receiver sends data to server
    //printf("socket_to_server: %d\n", socket_to_server);
    int writeStatus = write(socket_to_server, feedback, 8*sizeof(float));
    printf("Rx writeStatus: %d\n", writeStatus);

    // Receiver closes socket to server
    close(socket_to_server);
    return 0;

} // end rxCallback()

/*
// TODO: Once we are using USRPs, move to an rx.c file that will run independently.
ofdmflexframesync CreateFS(unsigned int numSubcarriers, unsigned int CPLen, unsigned int taperLen)
{
     ofdmflexframesync fs =
             ofdmflexframesync_create(numSubcarriers, CPLen, taperLen, NULL, rxCallback, NULL);

     return fs;
} // End CreateFS();
*/

int rxReceivePacket(unsigned int numSubcarriers, unsigned int CPLen, ofdmflexframesync * _fs, std::complex<float> * frameSamples, unsigned int len)
{
    unsigned int symbolLen = numSubcarriers + CPLen;
    ofdmflexframesync_execute(*_fs, frameSamples, symbolLen);
    //ofdmflexframesync_execute(*_fs, frameSamples, 1);
    //ofdmflexframesync_execute(*_fs, frameSamples, len);
    return 1;
} // End rxReceivePacket()

/*
uhd::usrp::multi_usrp::sptr initializeUSRPs()
{
    uhd::device_addr_t dev_addr;
    // TODO: Allow setting of USRP Address from command line
    //dev_addr["addr0"] = "8b9cadb0";
    dev_addr["addr0"] = "44b6b0e6";
    uhd::usrp::multi_usrp::sptr usrp= uhd::usrp::multi_usrp::make(dev_addr);

    // set the board to use the A RX frontend (RX channel 0)
    //uhd::usrp::subdev_spec_t spec;
    //std::string sd = "A:0";
    //spec.subdev_spec_t("A:0");
    //spec.db_name = "A";
    //spec.sd_name = "0";
    //dev->set_rx_subdev_spec("A:0", 0);
    //dev->set_rx_subdev_spec(sd, 0);

    // Set the antenna port
    //dev->set_rx_antenna("TX/RX");

    // Set Rx Frequency
    // TODO: Allow setting of center frequency from command line
    usrp->set_rx_freq(450e6);
    printf("RX Freq set to %f MHz\n", (usrp->get_rx_freq()/1e6));
    // Wait for USRP to settle at the frequency
    while (not usrp->get_rx_sensor("lo_locked").to_bool()){
        usleep(1000);
            //sleep for a short time 
    }
    //printf("USRP tuned and ready.\n");

    // Set the rf gain (dB)
    // TODO: Allow setting of gain from command line
    usrp->set_rx_gain(0.0);
    printf("RX Gain set to %f dB\n", usrp->get_rx_gain());


    return usrp;
} // end initializeUSRPs()
*/

int main(int argc, char ** argv)
{
    int debug_enabled = 1;
    unsigned int serverPort = 1402;
    char * serverAddr = (char*) "127.0.0.1";

    // Frame Synchronizer parameters
    // TODO: Make these adjustable from command line
    unsigned int numSubcarriers = 64;             // Number of subcarriers for OFDM
    unsigned int CPLen = 16;                      // Cyclic Prefix length
    unsigned int taperLen = 4;                     // Taper length

    // TODO: Make these adjustable from command line
    //float bandwidth = 500.0e3;  // Hz
    float bandwidth = 1.0e6;  // Hz
    float frequency = 450.0e6;  // Hz
    float uhd_rxgain = 20.0;    // dB

    // Check Program options
    int d;
    while ((d = getopt(argc,argv,"uha:p:")) != EOF) {
        switch (d) {
        case 'u':
        case 'h':   usage();                           return 0;
        //case 'q':   verbose = 0;                          break;
        //case 'v':   verbose = 1; verbose_explicit = 1;    break;
        //case 'd':   dataToStdout = 1;
                    //if (!verbose_explicit) verbose = 0;   break;
        //case 'r':   usingUSRPs = 1;                       break;
        //case 's':   usingUSRPs = 0;                       break;
        case 'a':   serverAddr = optarg;               break;
        case 'p':   serverPort = atoi(optarg);            break;
        //case 'f':   frequency = atof(optarg);           break;
        //case 'b':   bandwidth = atof(optarg);           break;
        //case 'G':   uhd_rxgain = atof(optarg);          break;
        //case 't':   num_seconds = atof(optarg);         break;
        default:;
            //verbose = 1;
        }
    }

    // framesynchronizer object used in each test
    //ofdmflexframesync fs;

    struct rxCBstruct rxCBs = CreaterxCBStruct();
    rxCBs.bandwidth = bandwidth;
    rxCBs.serverPort = serverPort;
    rxCBs.serverAddr = serverAddr;
    // Initialize Connection to USRP                                     
    unsigned char * p = NULL;   // default subcarrier allocation
    ofdmtxrx txcvr(numSubcarriers, CPLen, taperLen, p, rxCallback, (void*) &rxCBs);



    //uhd::usrp::multi_usrp::sptr usrp = initializeUSRPs();    

    //// TODO: Move this into initializeUSRPs()
    //    // Set the rx_rate
    //    // TODO: Allow setting of bandwidth from command line
    //    float bandwidth = 500.0e3; // Hz
    //    float rx_rate = 2.0f*bandwidth;
    //    usrp->set_rx_rate(rx_rate);
    //    double usrp_rx_rate = usrp->get_rx_rate();
    //    double rx_resamp_rate = rx_rate/usrp_rx_rate;
    //    msresamp_crcf resamp = msresamp_crcf_create(rx_resamp_rate, 60.0f);
    //    printf("RX rate set to %f MS/s\n", (usrp->get_rx_rate()/1e6));

    //// Create a receive streamer
    //// Linearly map channels (index0 = channel0, index1 = channel1, ...)
    //uhd::stream_args_t stream_args("fc32"); //complex floats
    ////stream_args.channels = 0;
    //uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
    //const size_t samplesPerPacket  = rx_stream->get_max_num_samps();

    ////std::vector<std::complex<float> > frameSamples[samplesPerPacket];
    //std::complex<float> buffer[samplesPerPacket];
    //unsigned int br_len = samplesPerPacket/rx_resamp_rate;
    //std::complex<float> buffer_resampled[br_len];
    //uhd::rx_metadata_t metaData;

    //// Begin streaming data from USRP
    //usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);

    // Initialize Receiver Defaults for current CE and Sc
    //fs = CreateFS(numSubcarriers, CPLen, taperLen);

    // set properties
    txcvr.set_rx_freq(frequency);
    txcvr.set_rx_rate(bandwidth);
    txcvr.set_rx_gain_uhd(uhd_rxgain);
    // This doesn't seem to work
    //char antenna[] = "RX2";
    //txcvr.set_rx_antenna(antenna);

    // enable debugging on request
    if (debug_enabled) {
        txcvr.debug_enable();
        printf("Set Rx freq to %f\n", frequency);
        printf("Set Rx rate to %f\n", bandwidth);
        printf("Set uhd Rx gain to %f\n", uhd_rxgain);
        printf("Set Rx antenna to %s\n", "RX2");
    }

    // run conditions
    int continue_running = 1;

    // Start Receiver
    txcvr.start_rx();


    while (continue_running)
    {
        //size_t num_rx_samps = rx_stream->recv(
        //    buffer, samplesPerPacket, metaData,
        //    3.0,
        //    1 
        //);

        // Resample the data according to difference between nominal and USRP sample rates
        // If this is done one at a time, why is the an entire array used for buffer_resampled?
        // Is the first element of buffer_resampled overwritten each time?
        //unsigned int j;
        //for (j=0; j<num_rx_samps; j++) {
        //    // get sample 
        //    std::complex<float> usrp_sample = buffer[j];

        //    // resample one at a time
        //    unsigned int nw;
        //    msresamp_crcf_execute(resamp, &usrp_sample, 1, buffer_resampled, &nw);

        //    // Rx Receives packet
        //    rxReceivePacket(numSubcarriers, CPLen, &fs, buffer_resampled);
        //}

        // resample 
        //unsigned int nw;
        //msresamp_crcf_execute(resamp, buffer, samplesPerPacket, buffer_resampled, &nw);

        //// Rx Receives packet
        //rxReceivePacket(numSubcarriers, CPLen, &fs, buffer_resampled, br_len);
        ////rxReceivePacket(numSubcarriers, CPLen, &fs, buffer_resampled, nw);

        // TODO: Add error capabilities. 
        // See http://code.ettus.com/redmine/ettus/projects/uhd/repository/revisions/master/entry/host/examples/rx_samples_to_file.cpp
        // lines 85-113

        // TODO: Create this function
        // Store a copy of the packet that was transmitted. For reference.
        // txStoreTransmittedPacket();

    } // End while

    return 0;
} // End main()
