#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex>
#include <liquid/liquid.h>
// Definition of liquid_float_complex changes depending on
// whether <complex> is included before or after liquid.h
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
#define PORT 1353
#define MAXPENDING 5

struct CognitiveEngine {
    char modScheme[20];
    char crcScheme[20];
    char innerFEC[20];
    char outerFEC[20];
    float default_tx_power;
    char option_to_adapt[20];
    char goal[20];
    float threshold;
    float latestGoalValue;
    int iterations;
    int payloadLen;
    unsigned int numSubcarriers;
    unsigned int CPLen;
    unsigned int taperLen;
};

struct Scenario {
    int addNoise; //Does the Scenario have noise?
    float noiseSNR;
    float noiseDPhi;

    int addInterference; // Does the Scenario have interference?

    int addFading; // Does the Secenario have fading?
    float fadeK;
    float fadeFd;
    float fadeDPhi;
};

// Default parameters for a Cognitive Engine
struct CognitiveEngine CreateCognitiveEngine() {
    struct CognitiveEngine ce = {};
    ce.default_tx_power = 10.0;
    ce.threshold = 1.0;                 // Desired value for goal
    ce.latestGoalValue = 0.0;           // Value of goal to be compared to threshold
    ce.iterations = 100;                // Number of transmissions made before attemting to receive them.
    ce.payloadLen = 120;                // Length of payload in frame generation
    ce.numSubcarriers = 64;             // Number of subcarriers for OFDM
    ce.CPLen = 16;                      // Cyclic Prefix length
    ce.taperLen = 4;                     // Taper length
    strcpy(ce.modScheme, "QPSK");
    strcpy(ce.option_to_adapt, "mod_scheme");
    strcpy(ce.goal, "payload_valid");
    strcpy(ce.crcScheme, "none");
    strcpy(ce.innerFEC, "Hamming128");
    strcpy(ce.outerFEC, "none");
    return ce;
} // End CreateCognitiveEngine()

// Default parameter for Scenario
struct Scenario CreateScenario() {
    struct Scenario sc = {};
    sc.addNoise = 1,
    sc.noiseSNR = 7.0f, // in dB
    sc.noiseDPhi = 0.001f,

    sc.addInterference = 0,

    sc.addFading = 0,
    sc.fadeK = 30.0f,
    sc.fadeFd = 0.2f,
    sc.fadeDPhi = 0.001f;
    return sc;
} // End CreateScenario()

int rxCallback(unsigned char *  _header,
                int              _header_valid,
                unsigned char *  _payload,
                unsigned int     _payload_len,
                int              _payload_valid,
                framesyncstats_s _stats,
                void *           _userdata)
{
    // Iterator
    int i = 0;

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
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(PORT);
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Attempt to connect client socket to server
    int connect_status;
    if((connect_status = connect(socket_to_server, (struct sockaddr*)&servAddr, sizeof(servAddr))))
    {   
        printf("Receive Failed to Connect to server.\n");
        printf("connect_status = %d\n", connect_status);
        exit(1);
    }
   
    framesyncstats_print(&_stats); 
      
    // Data that will be sent to server
    // TODO: Send other useful data through feedback array
    float feedback[8];
    feedback[0] = (float) _header_valid;
    feedback[1] = (float) _payload_valid;
    feedback[2] = (float) _stats.evm;
    feedback[3] = (float) _stats.rssi;   
   
    for (i=0; i<4; i++)
    printf("feedback data before transmission: %f\n", feedback[i]);

    // Receiver sends data to server
    //printf("socket_to_server: %d\n", socket_to_server);
    int writeStatus = write(socket_to_server, feedback, 8*sizeof(float));
    printf("Rx writeStatus: %d\n", writeStatus);

    // Receiver closes socket to server
    close(socket_to_server);
    return 0;

} // end rxCallback()

// TODO: Once we are using USRPs, move to an rx.c file that will run independently.
ofdmflexframesync CreateFS(unsigned int numSubcarriers, unsigned int CPLen, unsigned int taperLen)
{
     ofdmflexframesync fs =
             ofdmflexframesync_create(numSubcarriers, CPLen, taperLen, NULL, rxCallback, NULL);

     return fs;
} // End CreateFS();

int rxReceivePacket(unsigned int numSubcarriers, unsigned int CPLen, ofdmflexframesync * _fs, std::complex<float> * frameSamples)
{
    unsigned int symbolLen = numSubcarriers + CPLen;
    ofdmflexframesync_execute(*_fs, frameSamples, symbolLen);
    return 1;
} // End rxReceivePacket()

uhd::usrp::multi_usrp::sptr initializeUSRPs()
{
    uhd::device_addr_t dev_addr;
    // TODO: Allow setting of USRP Address from command line
    dev_addr["addr0"] = "8b9cadb0";
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
    usrp->set_rx_freq(400e6);
    // Wait for USRP to settle at the frequency
    while (not usrp->get_rx_sensor("lo_locked").to_bool()){
        usleep(1000);
            //sleep for a short time 
    }
    //printf("USRP tuned and ready.\n");

    // Set the rf gain (dB)
    // TODO: Allow setting of gain from command line
    usrp->set_rx_gain(0.0);

    // Set the rx_rate
    // TODO: Allow setting of rx_rate from command line
    usrp->set_rx_rate(1e6);

    return usrp;
} // end initializeUSRPs()

int main()
{

    // Frame Synchronizer parameters
    // TODO: Make these adjustable from command line
    unsigned int numSubcarriers = 64;             // Number of subcarriers for OFDM
    unsigned int CPLen = 16;                      // Cyclic Prefix length
    unsigned int taperLen = 4;                     // Taper length

    // framesynchronizer object used in each test
    ofdmflexframesync fs;

    // Initialize Connection to USRP                                     
    uhd::usrp::multi_usrp::sptr usrp = initializeUSRPs();    

    // Create a receive streamer
    // Linearly map channels (index0 = channel0, index1 = channel1, ...)
    uhd::stream_args_t stream_args("fc32"); //complex floats
    //stream_args.channels = 0;
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
    const size_t samplesPerPacket  = rx_stream->get_max_num_samps();

    //std::vector<std::complex<float> > frameSamples[samplesPerPacket];
    std::complex<float> frameSamples[samplesPerPacket];
    uhd::rx_metadata_t metaData;

    // Begin streaming data from USRP
    usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);

    // Initialize Receiver Defaults for current CE and Sc
    fs = CreateFS(numSubcarriers, CPLen, taperLen);

    while (1)
    {
        size_t num_rx_samps = rx_stream->recv(
            frameSamples, samplesPerPacket, metaData,
            3.0,
            1 
        );

        // TODO: Add error capabilities. 
        // See http://code.ettus.com/redmine/ettus/projects/uhd/repository/revisions/master/entry/host/examples/rx_samples_to_file.cpp
        // lines 85-113

        // TODO: Create this function
        // Store a copy of the packet that was transmitted. For reference.
        // txStoreTransmittedPacket();

        // Rx Receives packet
        rxReceivePacket(numSubcarriers, CPLen, &fs, frameSamples);
    } // End while

    return 0;
} // End main()
