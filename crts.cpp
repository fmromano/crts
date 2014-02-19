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
#include <getopt.h>     // For command line options
#define PORT 1400
#define MAXPENDING 5


void usage() {
    printf("crts -- test cognitive engines\n");
    printf("  u,h   :   usage/help\n");
    printf("  q/v   :   quiet/verbose (not yet implemented)\n");
    printf("  d     :   print rx data to Stdout rather than to file (not yet implemented)\n");
    //printf("  f     :   center frequency [Hz], default: 462 MHz\n");
    //printf("  b     :   bandwidth [Hz], default: 250 kHz\n");
    //printf("  G     :   uhd rx gain [dB] (default: 20dB)\n");
    //printf("  t     :   run time [seconds]\n");
    //printf("  z     :   number of subcarriers to notch in the center band, default: 0\n");
}


struct CognitiveEngine {
    char modScheme[30];
    char crcScheme[30];
    char innerFEC[30];
    char outerFEC[30];
    char adjustOn[30];
    float default_tx_power;
    char option_to_adapt[30];
    char goal[30];
    float threshold;
    float latestGoalValue;
    float weightedAvg; 
    float PER;
    float BERLastPacket;
    float BERTotal;
    float framesReceived;
    float validPayloads;
    float errorFreePayloads;
    float frequency;
    float bandwidth;
    float txgain_dB;
    float uhd_txgain;
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
    ce.weightedAvg = 0.0;
    ce.PER = 0.0;
    ce.BERLastPacket = 0.0;
    ce.BERTotal = 0.0;
    ce.framesReceived = 0.0;
    ce.validPayloads = 0.0;
    ce.errorFreePayloads = 0.0;
    ce.frequency = 450.0e6;
    ce.bandwidth = 1.0e6;
    ce.txgain_dB = -12.0;
    ce.uhd_txgain = 40.0;
    strcpy(ce.modScheme, "QPSK");
    strcpy(ce.option_to_adapt, "mod_scheme->BPSK");
    strcpy(ce.goal, "payload_valid");
    strcpy(ce.crcScheme, "none");
    strcpy(ce.innerFEC, "none");
    strcpy(ce.outerFEC, "none");
    //strcpy(ce.adjustOn, "weighted_avg_payload_valid"); 
    strcpy(ce.adjustOn, "packet_error_rate"); 
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

int readScMasterFile(char scenario_list[30][60])
{
    config_t cfg;                   // Returns all parameters in this structure 
    config_setting_t *setting;
    const char *str;                // Stores the value of the String Parameters in Config file
    int tmpI;                       // Stores the value of Integer Parameters from Config file                

    char current_sc[30];
    int no_of_scenarios=1;
    int i;
    char tmpS[30];
    //Initialization
    config_init(&cfg);
   
   
    // Read the file. If there is an error, report it and exit. 
    if (!config_read_file(&cfg,"master_scenario_file.txt"))
    {
        printf("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        printf("\nCould not find master file\n");
        config_destroy(&cfg);
        return -1;
    }
    else
        printf("\nFound master Scenario config file\n");

  
    // Get the configuration file name. 
    if (config_lookup_string(&cfg, "filename", &str))
        //printf("File Type: %s\n", str)
        ;
    else
        printf("No 'filename' setting in configuration file.\n");

    // Read the parameter group
    setting = config_lookup(&cfg, "params");
    if (setting != NULL)
    {
        
        if (config_setting_lookup_int(setting, "NumberofScenarios", &tmpI))
        {
            no_of_scenarios=tmpI;
            printf ("Number of Scenarios: %d\n",tmpI);
        }
        
       for (i=1;i<=no_of_scenarios;i++)
       //while (strcmp (status,"end"!=0))
       {
         strcpy (current_sc,"scenario_");
         sprintf (tmpS,"%d",i);
         //printf ("\n Scenario Number =%s", tmpS);
         strcat (current_sc,tmpS);
         //printf ("\n CURRENT SCENARIO =%s", current_sc);
         if (config_setting_lookup_string(setting, current_sc, &str))
          {
              strcpy(*((scenario_list)+i-1),str);          
              //printf ("\nSTR=%s\n",str);
          }
        /*else
            printf("\nNo 'param2' setting in configuration file.");
          */
        printf ("Scenario File: %s\n", *((scenario_list)+i-1) );
      } 
    }
    config_destroy(&cfg);
    return no_of_scenarios;
} // End readScMasterFile()

int readCEMasterFile(char cogengine_list[30][60])
{
    config_t cfg;               // Returns all parameters in this structure 
    config_setting_t *setting;
    const char *str;            // Stores the value of the String Parameters in Config file
    int tmpI;                   // Stores the value of Integer Parameters from Config file             

    char current_ce[30];
    int no_of_cogengines=1;
    int i;
    char tmpS[30];
    //Initialization
    config_init(&cfg);
   
    //printf ("\nInside readCEMasterFile function\n");
    //printf ("%sCogEngine List[0]:\n",cogengine_list[0] );

    // Read the file. If there is an error, report it and exit. 
    if (!config_read_file(&cfg,"master_cogengine_file.txt"))
    {
        printf("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        printf("\nCould not find master file\n");
        config_destroy(&cfg);
        return -1;
    }
    else
        printf("Found master Cognitive Engine config file.\n");
  
    // Get the configuration file name. 
    if (config_lookup_string(&cfg, "filename", &str))
        //printf("File Type: %s\n", str)
        ;
    else
        printf("No 'filename' setting in master CE configuration file.\n");

    // Read the parameter group
    setting = config_lookup(&cfg, "params");
    if (setting != NULL)
    {
        if (config_setting_lookup_int(setting, "NumberofCogEngines", &tmpI))
        {
            no_of_cogengines=tmpI;
            printf ("Number of Congnitive Engines: %d\n",tmpI);
        }
        
        for (i=1;i<=no_of_cogengines;i++)
        {
            strcpy (current_ce,"cogengine_");
            sprintf (tmpS,"%d",i);
            //printf ("\n Scenario Number =%s", tmpS);
            strcat (current_ce,tmpS);
            //printf ("\n CURRENT SCENARIO =%s", current_sc);
            if (config_setting_lookup_string(setting, current_ce, &str))
            {
                strcpy(*((cogengine_list)+i-1),str);          
                //printf ("\nSTR=%s\n",str);
            }
            printf ("Cognitive Engine File: %s\n", *((cogengine_list)+i-1) );
        } 
    }
    config_destroy(&cfg);
    return no_of_cogengines;
} // End readCEMasterFile()

///////////////////Cognitive Engine///////////////////////////////////////////////////////////
////////Reading the cognitive radio parameters from the configuration file////////////////////
int readCEConfigFile(struct CognitiveEngine * ce,char *current_cogengine_file)
{
    config_t cfg;               // Returns all parameters in this structure 
    config_setting_t *setting;
    const char * str;           // Stores the value of the String Parameters in Config file
    int tmpI;                   // Stores the value of Integer Parameters from Config file
    double tmpD;                
    char ceFileLocation[60];

    strcpy(ceFileLocation, "ceconfigs/");
    strcat(ceFileLocation, current_cogengine_file);

    //Initialization
    config_init(&cfg);

    // Read the file. If there is an error, report it and exit. 
    if (!config_read_file(&cfg,ceFileLocation))
    {
        printf("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return -1;
    }

    // Get the configuration file name. 
    if (config_lookup_string(&cfg, "filename", &str))
        printf("Cognitive Engine Configuration File Name: %s\n", str);
    else
        printf("No 'filename' setting in configuration file.\n");

    // Read the parameter group
    setting = config_lookup(&cfg, "params");
    if (setting != NULL)
    {
        // Read the string
        if (config_setting_lookup_string(setting, "option_to_adapt", &str))
        {
            strcpy(ce->option_to_adapt,str);
            printf ("Option to adapt: %s\n",str);
        }
       
        if (config_setting_lookup_string(setting, "goal", &str))
        {
            strcpy(ce->goal,str);
            printf ("Goal: %s\n",str);
        }
        if (config_setting_lookup_string(setting, "adjustOn", &str))
        {
            strcpy(ce->adjustOn,str);
            printf ("adjustOn: %s\n",str);
        }
        if (config_setting_lookup_string(setting, "modScheme", &str))
        {
            strcpy(ce->modScheme,str);
            printf ("Modulation Scheme:%s\n",str);
        }
        if (config_setting_lookup_string(setting, "crcScheme", &str))
        {
            strcpy(ce->crcScheme,str);
            printf ("CRC Scheme:%s\n",str);
        }
        if (config_setting_lookup_string(setting, "innerFEC", &str))
        {
            strcpy(ce->innerFEC,str);
            printf ("Inner FEC Scheme:%s\n",str);
        }
        if (config_setting_lookup_string(setting, "outerFEC", &str))
        {
            strcpy(ce->outerFEC,str);
            printf ("Outer FEC Scheme:%s\n",str);
        }

        // Read the integers
        if (config_setting_lookup_int(setting, "iterations", &tmpI))
        {
           ce->iterations=tmpI;
           printf("Iterations: %d\n", tmpI);
        }
        if (config_setting_lookup_int(setting, "payloadLen", &tmpI))
        {
           ce->payloadLen=tmpI; 
           printf("PayloadLen: %d\n", tmpI);
        }
        if (config_setting_lookup_int(setting, "numSubcarriers", &tmpI))
        {
           ce->numSubcarriers=tmpI; 
           printf("Number of Subcarriers: %d\n", tmpI);
        }
        if (config_setting_lookup_int(setting, "CPLen", &tmpI))
        {
           ce->CPLen=tmpI; 
           printf("CPLen: %d\n", tmpI);
        }
        if (config_setting_lookup_int(setting, "taperLen", &tmpI))
        {
           ce->taperLen=tmpI; 
           printf("taperLen: %d\n", tmpI);
        }
        // Read the floats
        if (config_setting_lookup_float(setting, "default_tx_power", &tmpD))
        {
           ce->default_tx_power=tmpD; 
           printf("Default Tx Power: %f\n", tmpD);
        }
        if (config_setting_lookup_float(setting, "latestGoalValue", &tmpD))
        {
           ce->latestGoalValue=tmpD; 
           printf("Latest Goal Value: %f\n", tmpD);
        }
        if (config_setting_lookup_float(setting, "threshold", &tmpD))
        {
           ce->threshold=tmpD; 
           printf("Threshold: %f\n", tmpD);
        }
     }
    config_destroy(&cfg);
     return 1;
} // End readCEConfigFile()

int readScConfigFile(struct Scenario * sc, char *current_scenario_file)
{
    config_t cfg;               // Returns all parameters in this structure 
    config_setting_t *setting;
    const char * str;
    int tmpI;
    double tmpD;
    char scFileLocation[60];

    //printf("In readScConfigFile(): string current_scenario_file: \n%s\n", current_scenario_file);

    // Because the file is in the folder 'scconfigs'
    strcpy(scFileLocation, "scconfigs/");
    strcat(scFileLocation, current_scenario_file);
    //printf("In readScConfigFile(): string scFileLocation: \n%s\n", scFileLocation);

    // Initialization 
    config_init(&cfg);

    // Read the file. If there is an error, report it and exit. 
    if (!config_read_file(&cfg, scFileLocation))
    {
        printf("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return -1;
    }

    // Get the configuration file name. 
    if (config_lookup_string(&cfg, "filename", &str))
        printf("\nFile Name: %s", str);
    else
        printf("\nNo 'filename' setting in configuration file.");

    // Read the parameter group.
    setting = config_lookup(&cfg, "params");
    if (setting != NULL)
    {
        // Read the integer
        if (config_setting_lookup_int(setting, "addNoise", &tmpI))
        {
            printf("\nAddnoise: %d", tmpI);
            sc->addNoise=tmpI;
        }
        else
            printf("\nNo AddNoise setting in configuration file.");
        
        // Read the double
        if (config_setting_lookup_float(setting, "noiseSNR", &tmpD))
        {
            printf("\nNoise SNR: %f", tmpD);
            sc->noiseSNR=(float) tmpD;
        }
        else
            printf("\nNo Noise SNR setting in configuration file.");
       
        // Read the double
        if (config_setting_lookup_float(setting, "noiseDPhi", &tmpD))
        {
            sc->noiseDPhi=(float) tmpD;
            printf("\nNoiseDPhi: %f", tmpD);
        }
        else
            printf("\nNo NoiseDPhi setting in configuration file.");

        // Read the integer
        if (config_setting_lookup_int(setting, "addInterference", &tmpI))
        {
            sc->addInterference=tmpI;
            printf("\naddInterference: %d", tmpI);
        }
        else
            printf("\nNo addInterference setting in configuration file.");

        // Read the integer
        if (config_setting_lookup_int(setting, "addFading", &tmpI))
        {
            sc->addFading=tmpI;
            printf("\naddFading: %d", tmpI);
        }
        else
            printf("\nNo addFading setting in configuration file.");

        // Read the double
        if (config_setting_lookup_float(setting, "fadeK", &tmpD))
        {
            sc->addFading=(float)tmpD;
            printf("\naddFading: %f", tmpD);
        }
        else
            printf("\nNo addFading setting in configuration file.");

        // Read the double
        if (config_setting_lookup_float(setting, "fadeFd", &tmpD))
        {
            sc->addFading=(float)tmpD;
            printf("\naddFading: %f", tmpD);
        }
        else
            printf("\nNo addFading setting in configuration file.");

        // Read the double
        if (config_setting_lookup_float(setting, "fadeDPhi", &tmpD))
        {
            sc->addFading=(float)tmpD;
            printf("\naddFading: %f", tmpD);
        }
        else
            printf("\nNo addFading setting in configuration file.");
    printf("\n");
    }

    config_destroy(&cfg);

    //printf("End of readScConfigFile() Function\n");
    return 1;
} // End readScConfigFile()

// Add AWGN
void addAWGN(std::complex<float> * transmit_buffer, struct CognitiveEngine ce, struct Scenario sc)
{
    //options
    float dphi  = sc.noiseDPhi;                              // carrier frequency offset
    float SNRdB = sc.noiseSNR;                               // signal-to-noise ratio [dB]
    unsigned int symbol_len = ce.numSubcarriers + ce.CPLen;  // defining symbol length
    
    //printf("In addAWGN: SNRdB=%f\n", SNRdB);

    // noise parameters
    float nstd = powf(10.0f, -SNRdB/20.0f); // noise standard deviation
    float phi = 0.0f;                       // channel phase
   
    std::complex<float> tmp (0, 1); 
    unsigned int i;

    // noise mixing
    for (i=0; i<symbol_len; i++) {
        transmit_buffer[i] = std::exp(tmp*phi) * transmit_buffer[i]; // apply carrier offset
        //transmit_buffer[i] *= cexpf(_Complex_I*phi); // apply carrier offset
        phi += dphi;                                 // update carrier phase
        cawgn(&transmit_buffer[i], nstd);            // add noise
    }
} // End addAWGN()

// Add Rice-K Fading
void addRiceFading(std::complex<float> * transmit_buffer, struct CognitiveEngine ce, struct Scenario sc)
{
    // options
    unsigned int symbol_len = ce.numSubcarriers + ce.CPLen; // defining symbol length
    unsigned int h_len;                                     // doppler filter length
    if (symbol_len > 94){
        h_len = 0.0425*symbol_len;
    }
    else {
        h_len = 4;
    }
    float fd           = sc.fadeFd; // maximum doppler frequency
    float K            = sc.fadeK;  // Rice fading factor
    float omega        = 1.0f;      // mean power
    float theta        = 0.0f;      // angle of arrival
    float dphi = sc.fadeDPhi;       // carrier frequency offset
    float phi = 0.0f;               // channel phase

    // validate input
    if (K < 1.5f) {
        fprintf(stderr,"error: fading factor K must be greater than 1.5\n");
        exit(1);
    } else if (omega < 0.0f) {
        fprintf(stderr,"error: signal power Omega must be greater than zero\n");
        exit(1);
    } else if (fd <= 0.0f || fd >= 0.5f) {
        fprintf(stderr,"error: Doppler frequency must be in (0,0.5)\n");
        exit(1);
    } else if (symbol_len== 0) {
        fprintf(stderr,"error: number of samples must be greater than zero\n");
        exit(1);
    }
 
    unsigned int i;

    // allocate array for output samples
    std::complex<float> * y = (std::complex<float> *) malloc(symbol_len*sizeof(std::complex<float>));
    // generate Doppler filter coefficients
    float h[h_len];
    liquid_firdes_doppler(h_len, fd, K, theta, h);

    // normalize filter coefficients such that output Gauss random
    // variables have unity variance
    float std = 0.0f;
    for (i=0; i<h_len; i++)
        std += h[i]*h[i];
    std = sqrtf(std);
    for (i=0; i<h_len; i++)
        h[i] /= std;

    // create Doppler filter from coefficients
    firfilt_crcf fdoppler = firfilt_crcf_create(h,h_len);

    // generate complex circular Gauss random variables
    std::complex<float> v;    // circular Gauss random variable (uncorrelated)
    std::complex<float> x;    // circular Gauss random variable (correlated w/ Doppler filter)
    float s   = sqrtf((omega*K)/(K+1.0));
    float sig = sqrtf(0.5f*omega/(K+1.0));
        
    std::complex<float> tmp(0, 1);
    for (i=0; i<symbol_len; i++) {
        // generate complex Gauss random variable
        crandnf(&v);

        // push through Doppler filter
        firfilt_crcf_push(fdoppler, v);
        firfilt_crcf_execute(fdoppler, &x);

        // convert result to random variable with Rice-K distribution
        y[i] = tmp*( std::imag(x)*sig + s ) +
                          ( std::real(x)*sig     );
    }
    for (i=0; i<symbol_len; i++) {
        transmit_buffer[i] *= std::exp(tmp*phi);  // apply carrier offset
        phi += dphi;                                  // update carrier phase
        transmit_buffer[i] *= y[i];                   // apply Rice-K distribution
    }

    // destroy filter object
    firfilt_crcf_destroy(fdoppler);

    // clean up allocated arrays
    free(y);
} // End addRiceFading()

// Enact Scenario
// TODO: Alter code for when usingUSRPs
void enactScenario(std::complex<float> * transmit_buffer, struct CognitiveEngine ce, struct Scenario sc, int usingUSRPs)
{
    // Check AWGN
    if (sc.addNoise == 1){
       addAWGN(transmit_buffer, ce, sc);
    }
    if (sc.addInterference == 1){
       printf("WARNING: There is currently no interference scenario functionality!\n");
       // Interference function
    }
    if (sc.addFading == 1){
       addRiceFading(transmit_buffer, ce, sc);
    }
    if ( (sc.addNoise == 0) && (sc.addInterference == 0) && (sc.addFading == 0) ){
       printf("Nothing Added by Scenario!\n");
    }
} // End enactScenario()

modulation_scheme convertModScheme(char * modScheme)
{
    modulation_scheme ms;
    // TODO: add other liquid-supported mod schemes
    if (strcmp(modScheme, "QPSK") == 0) {
        ms = LIQUID_MODEM_QPSK;
    }
    else if ( strcmp(modScheme, "BPSK") ==0) {
        ms = LIQUID_MODEM_BPSK;
    }
    else if ( strcmp(modScheme, "OOK") ==0) {
        ms = LIQUID_MODEM_OOK;
    }
    else if ( strcmp(modScheme, "8PSK") ==0) {
        ms = LIQUID_MODEM_PSK8;
    }
    else if ( strcmp(modScheme, "16PSK") ==0) {
        ms = LIQUID_MODEM_PSK16;
    }
    else if ( strcmp(modScheme, "32PSK") ==0) {
        ms = LIQUID_MODEM_PSK32;
    }
    else if ( strcmp(modScheme, "64PSK") ==0) {
        ms = LIQUID_MODEM_PSK64;
    }
    else if ( strcmp(modScheme, "128PSK") ==0) {
        ms = LIQUID_MODEM_PSK128;
    }
    else if ( strcmp(modScheme, "8QAM") ==0) {
        ms = LIQUID_MODEM_QAM8;
    }
    else if ( strcmp(modScheme, "16QAM") ==0) {
        ms = LIQUID_MODEM_QAM16;
    }
    else if ( strcmp(modScheme, "32QAM") ==0) {
        ms = LIQUID_MODEM_QAM32;
    }
    else if ( strcmp(modScheme, "64QAM") ==0) {
        ms = LIQUID_MODEM_QAM64;
    }
    else if ( strcmp(modScheme, "BASK") ==0) {
        ms = LIQUID_MODEM_ASK2;
    }
    else if ( strcmp(modScheme, "4ASK") ==0) {
        ms = LIQUID_MODEM_ASK4;
    }
    else if ( strcmp(modScheme, "8ASK") ==0) {
        ms = LIQUID_MODEM_ASK8;
    }
    else if ( strcmp(modScheme, "16ASK") ==0) {
        ms = LIQUID_MODEM_ASK16;
    }
    else if ( strcmp(modScheme, "32ASK") ==0) {
        ms = LIQUID_MODEM_ASK32;
    }
    else if ( strcmp(modScheme, "64ASK") ==0) {
        ms = LIQUID_MODEM_ASK64;
    }
    else if ( strcmp(modScheme, "128ASK") ==0) {
        ms = LIQUID_MODEM_ASK128;
    }
    else {
        printf("ERROR: Unknown Modulation Scheme");
        //TODO: Skip current test if given an unknown parameter.
    }

    return ms;
} // End convertModScheme()

crc_scheme convertCRCScheme(char * crcScheme)
{
    crc_scheme check;
    if (strcmp(crcScheme, "none") == 0) {
        check = LIQUID_CRC_NONE;
        printf("check = LIQUID_CRC_NONE\n");
    }
    else if (strcmp(crcScheme, "checksum") == 0) {
        check = LIQUID_CRC_CHECKSUM;
        printf("check = LIQUID_CRC_CHECKSUM\n");
    }
    else if (strcmp(crcScheme, "8") == 0) {
        check = LIQUID_CRC_8;
        printf("check = LIQUID_CRC_8\n");
    }
    else if (strcmp(crcScheme, "16") == 0) {
        check = LIQUID_CRC_16;
        printf("check = LIQUID_CRC_16\n");
    }
    else if (strcmp(crcScheme, "24") == 0) {
        check = LIQUID_CRC_24;
        printf("check = LIQUID_CRC_24\n");
    }
    else if (strcmp(crcScheme, "32") == 0) {
        check = LIQUID_CRC_32;
        printf("check = LIQUID_CRC_32\n");
    }
    else {
        printf("ERROR: unknown CRC\n");
        //TODO: Skip current test if given an unknown parameter.
    }

    return check;
} // End convertCRCScheme()

fec_scheme convertFECScheme(char * FEC)
{
    // TODO: add other liquid-supported FEC schemes
    fec_scheme fec;
    if (strcmp(FEC, "none") == 0) {
        fec = LIQUID_FEC_NONE;
        printf("fec = LIQUID_FEC_NONE\n");
    }
    else if (strcmp(FEC, "Hamming74") == 0) {
        fec = LIQUID_FEC_HAMMING74;
        printf("fec = LIQUID_FEC_HAMMING74\n");
    }
    else if (strcmp(FEC, "Hamming128") == 0) {
        fec = LIQUID_FEC_HAMMING128;
        printf("fec = LIQUID_FEC_HAMMING128\n");
    }
    else if (strcmp(FEC, "Golay2412") == 0) {
        fec = LIQUID_FEC_GOLAY2412;
        printf("fec = LIQUID_FEC_GOLAY2412\n");
    }
    else if (strcmp(FEC, "SEC-DED2216") == 0) {
        fec = LIQUID_FEC_SECDED2216;
        printf("fec = LIQUID_FEC_SECDED2216\n");
    }
    else if (strcmp(FEC, "SEC-DED3932") == 0) {
        fec = LIQUID_FEC_SECDED3932;
        printf("fec = LIQUID_FEC_SECDED3932\n");
    }
    else if (strcmp(FEC, "SEC-DED7264") == 0) {
        fec = LIQUID_FEC_SECDED7264;
        printf("fec = LIQUID_FEC_SECDED7264\n");
    }
    else {
        printf("ERROR: unknown FEC\n");
        //TODO: Skip current test if given an unknown parameter.
    }
    return fec;
} // End convertFECScheme()

// Create Frame generator with CE and Scenario parameters
ofdmflexframegen CreateFG(struct CognitiveEngine ce, struct Scenario sc) {

    printf("Setting inital ofdmflexframegen options:\n");
    // Set Modulation Scheme
    modulation_scheme ms = convertModScheme(ce.modScheme);

    // Set Cyclic Redundency Check Scheme
    crc_scheme check = convertCRCScheme(ce.crcScheme);

    // Set inner forward error correction scheme
    printf("Inner FEC: ");
    fec_scheme fec0 = convertFECScheme(ce.innerFEC);

    // Set outer forward error correction scheme
    // TODO: add other liquid-supported FEC schemes
    printf("Outer FEC: ");
    fec_scheme fec1 = convertFECScheme(ce.outerFEC);

    // Frame generation parameters
    ofdmflexframegenprops_s fgprops;

    // Initialize Frame generator and Frame Synchronizer Objects
    ofdmflexframegenprops_init_default(&fgprops);
    fgprops.mod_scheme      = ms;
    fgprops.check           = check;
    fgprops.fec0            = fec0;
    fgprops.fec1            = fec1;
    printf("About to create fg...\n");
    ofdmflexframegen fg = ofdmflexframegen_create(ce.numSubcarriers, ce.CPLen, ce.taperLen, NULL, &fgprops);
    printf("fg created\n");

    return fg;
} // End CreateFG()

int rxCallback(unsigned char *  _header,
               int              _header_valid,
               unsigned char *  _payload,
               unsigned int     _payload_len,
               int              _payload_valid,
               framesyncstats_s _stats,
               void *           _userdata)
{
    // 
    //float * frameNumber = (float *) _userdata;
    //*frameNumber = *frameNumber +1;

    // Access header when it contains floats
    float * _header_f = (float*) _header;

    // Iterator
    int i = 0;

    // Create a client TCP socket
    int socket_to_server = socket(AF_INET, SOCK_STREAM, 0); 
    if( socket_to_server < 0)
    {   
        printf("Receiver Failed to Create Client Socket. error: %s\n", strerror(errno));
        exit(1);
    }   
    //printf("Created client socket to server. socket_to_server: %d\n", socket_to_server);

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
        printf("Receiver Failed to Connect to server.\n");
        printf("connect_status = %d\n", connect_status);
        exit(1);
    }
   
    //framesyncstats_print(&_stats); 

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
          
    // Data that will be sent to server
    // TODO: Send other useful data through feedback array
    float feedback[8];
    feedback[0] = (float) _header_valid;
    feedback[1] = (float) _payload_valid;
    feedback[2] = (float) _stats.evm;
    feedback[3] = (float) _stats.rssi;   
    //feedback[4] = *frameNumber;
    feedback[4] = *_header_f;
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

// TODO: Once we are using USRPs, move to an rx.c file that will run independently.
ofdmflexframesync CreateFS(struct CognitiveEngine ce, struct Scenario sc, float * frameNumber)
{
     ofdmflexframesync fs =
             ofdmflexframesync_create(ce.numSubcarriers, ce.CPLen, ce.taperLen, NULL, rxCallback, (void *) frameNumber);

     return fs;
} // End CreateFS();

// Transmit a packet of data.
// This will need to be modified once we implement the USRPs.
//int txGeneratePacket(struct CognitiveEngine ce, ofdmflexframegen * _fg, unsigned char * header, unsigned char * payload)
//{
//    return 1;
//} // End txGeneratePacket()

//int txTransmitPacket(struct CognitiveEngine ce, ofdmflexframegen * _fg, std::complex<float> * frameSamples, 
//                        uhd::tx_metadata_t md, uhd::tx_streamer::sptr txStream, int usingUSRPs)
//{
//    int isLastSymbol = ofdmflexframegen_writesymbol(*_fg, frameSamples);
//
//    return isLastSymbol;
//} // End txTransmitPacket()

// TODO: Alter code for when usingUSRPs
//int rxReceivePacket(struct CognitiveEngine ce, ofdmflexframesync * _fs, std::complex<float> * frameSamples, int usingUSRPs)
//{
//    unsigned int symbolLen = ce.numSubcarriers + ce.CPLen;
//    ofdmflexframesync_execute(*_fs, frameSamples, symbolLen);
//    return 1;
//} // End rxReceivePacket()

// Create a TCP socket for the server and bind it to a port
// Then sit and listen/accept all connections and write the data
// to an array that is accessible to the CE
void * startTCPServer(void * _read_buffer )
{
    printf("(Server thread called.)\n");
    // Buffer for data sent by client. This memory address is also given to CE
    float * read_buffer = (float *) _read_buffer;
    //  Local (server) address
    struct sockaddr_in servAddr;   
    // Parameters of client connection
    struct sockaddr_in clientAddr;              // Client address 
    socklen_t client_addr_size;  // Client address size
    int socket_to_client = -1;

    int reusePortOption = 1;
        
    // Create socket for incoming connections 
    int sock_listen;
    if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Transmitter Failed to Create Server Socket.\n");
        exit(1);
    }

    // TODO: Allow reuse of a port. See http://stackoverflow.com/questions/14388706/socket-options-so-reuseaddr-and-so-reuseport-how-do-they-differ-do-they-mean-t
    if (setsockopt(sock_listen, SOL_SOCKET, SO_REUSEPORT, (void*) &reusePortOption, sizeof(reusePortOption)) < 0 )
    {
        printf(" setsockopt() failed\n");
        exit(1);
    }

    // Construct local (server) address structure 
    memset(&servAddr, 0, sizeof(servAddr));       // Zero out structure 
    servAddr.sin_family = AF_INET;                // Internet address family 
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface 
    servAddr.sin_port = htons(PORT);              // Local port 
    // Bind to the local address to a port
    if (bind(sock_listen, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
    {
        printf("bind() error\n");
        exit(1);
    }
    // Listen and accept connections indefinitely
    while (1)
    {
        // listen for connections on socket
        if (listen(sock_listen, MAXPENDING) < 0)
        {
            printf("Failed to Set Sleeping (listening) Mode\n");
            exit(1);
        }
        printf("\n(Server is now in listening mode)\n");

        // Accept a connection from client
        //printf("Server is waiting to accept connection from client\n");
        socket_to_client = accept(sock_listen, (struct sockaddr *)&clientAddr, &client_addr_size);
        //printf("socket_to_client= %d\n", socket_to_client);
        if(socket_to_client< 0)
        {
            printf("Sever Failed to Connect to Client\n");
            exit(1);
        }
        printf("Server has accepted connection from client\n");
        // Transmitter receives data from client (receiver)
            // Zero the read buffer. Then read the data into it.
            bzero(read_buffer, 256);
            //int read_status = -1;   // indicates success/failure of read operation.
            //read_status = read(socket_to_client, read_buffer, 255);
            read(socket_to_client, read_buffer, 255);
        close(socket_to_client);

    } // End listening While loop
} // End startTCPServer()

int ceProcessData(struct CognitiveEngine * ce, float * feedback)
{
    int i = 0;
    printf("In ceProcessData():\nfeedback=\n");
    for (i = 0; i<7;i++) {
        printf("feedback[%d]= %f\n", i, feedback[i]);
    }

    ce->framesReceived = feedback[4];
    ce->validPayloads += feedback[1];
    if ( feedback[6] == 0)
    {
        printf("Error Free payload!\n");
        ce->errorFreePayloads++;
    }
    ce->PER = (ce->errorFreePayloads)/(ce->framesReceived);
    ce->BERLastPacket = feedback[6]/ce->payloadLen;

    //printf("ce->goal=%s\n", ce->goal);

    // Copy the data from the server
    if (strcmp(ce->goal, "payload_valid") == 0)
    {
        printf("Goal is payload_valid. Setting latestGoalValue to %f\n", feedback[1]);
        ce->latestGoalValue = feedback[1];
    }
    if (strcmp(ce->goal, "X_valid_payloads") == 0)
    {
        printf("Goal is X_valid_payloads. Setting latestGoalValue to %f\n", ce->latestGoalValue+feedback[1]);
        ce->latestGoalValue += feedback[1];
    }
    if(strcmp(ce->goal, "X_errorFreePayloads") == 0)
    {
        printf("Goal is X_errorFreePayloads. Setting latestGoalValue to %f\n", ce->errorFreePayloads);
        ce->latestGoalValue  = ce->errorFreePayloads;
    }
    else
    {
        printf("ERROR: Unknown Goal!\n");
    }
    // TODO: implement if statements for other possible goals.

    return 1;
} // End ceProcessData()

int ceOptimized(struct CognitiveEngine ce)
{
   printf("Checking if goal value has been reached.\n");
   printf("ce.latestGoalValue= %f\n", ce.latestGoalValue);
   printf("ce.threshold= %f\n", ce.threshold);
   if (ce.latestGoalValue >= ce.threshold)
   {
       printf("Goal is reached\n");
       return 1;
   }
   printf("Goal not reached yet.\n");
   return 0;
} // end ceOptimized()

int ceModifyTxParams(struct CognitiveEngine * ce, float * feedback)
{
    int modify = 0;
    printf("ce->adjustOn = %s\n", ce->adjustOn);
    // Check what values determine if parameters should be modified
    if(strcmp(ce->adjustOn, "last_payload_valid") == 0) {
        // Check if parameters should be modified
        if(feedback[1]<1)
        {
            modify = 1;
            printf("lpv. Modifying...\n");
        }
    }
    if(strcmp(ce->adjustOn, "weighted_avg_payload_valid") == 0) {
        // Check if parameters should be modified
        // TODO: Move to ceAnalyzeData
        ce->weightedAvg += feedback[1];
        // TODO: Allow this value to be changed
        if (ce->weightedAvg < 0.5)
        {
            modify = 1;
            printf("wapv. Modifying...\n");
        }
    }
    if(strcmp(ce->adjustOn, "packet_error_rate") == 0) {
        // Check if parameters should be modified
        // TODO: Allow this value to be changed
        printf("PER = %f\n", ce->PER);
        if(ce->PER <0.5)
        {
            modify = 1;
            printf("per. Modifying...\n" );
        }
    }
    if(strcmp(ce->adjustOn, "last_packet_error_free") == 0) {
        // Check if parameters should be modified
        if(feedback[6]<1){
            modify = 1;
            printf("lpef. Modifying...\n");
        }
    }

    // If so, modify the specified parameter
    if (modify) 
    {
        printf("Modifying Tx parameters...\n");
        // TODO: Implement a similar if statement for each possible option
        // that can be adapted.

        // Doesn't work.
        if (strcmp(ce->option_to_adapt, "decrease_numSubcarriers") == 0) {
            if (ce->numSubcarriers > 2)
                ce->numSubcarriers -= 2;
        }

        if (strcmp(ce->option_to_adapt, "decrease_mod_scheme_PSK") == 0) {
            if (strcmp(ce->modScheme, "QPSK") == 0) {
                strcpy(ce->modScheme, "BPSK");
            }
            if (strcmp(ce->modScheme, "8PSK") == 0) {
                strcpy(ce->modScheme, "QPSK");
            }
            if (strcmp(ce->modScheme, "16PSK") == 0) {
                strcpy(ce->modScheme, "8PSK");
            }
            if (strcmp(ce->modScheme, "32PSK") == 0) {
                strcpy(ce->modScheme, "16PSK");
            }
            if (strcmp(ce->modScheme, "64PSK") == 0) {
                strcpy(ce->modScheme, "32PSK");
            }
            if (strcmp(ce->modScheme, "128PSK") == 0) {
                strcpy(ce->modScheme, "64PSK");
                printf("New modscheme: 64PSK\n");
            }
        }
        // TODO: DOens't work. Fix.
        if (strcmp(ce->option_to_adapt, "decrease_mod_scheme_ASK") == 0) {
            if (strcmp(ce->modScheme, "128ASK") == 0) {
                strcpy(ce->modScheme, "64ASK");
            }
            if (strcmp(ce->modScheme, "64ASK") == 0) {
                strcpy(ce->modScheme, "32ASK");
            }
            if (strcmp(ce->modScheme, "32ASK") == 0) {
                strcpy(ce->modScheme, "16ASK");
            }
            if (strcmp(ce->modScheme, "16ASK") == 0) {
                strcpy(ce->modScheme, "8ASK");
            }
            if (strcmp(ce->modScheme, "8ASK") == 0) {
                strcpy(ce->modScheme, "4ASK");
            }
            if (strcmp(ce->modScheme, "4ASK") == 0) {
                strcpy(ce->modScheme, "BASK");
            }
        }
        // Not use FEC
        if (strcmp(ce->option_to_adapt, "no_fec") == 0) {
           if (strcmp(ce->outerFEC, "none") == 0) {
               strcpy(ce->outerFEC, "none");
           }
           if (strcmp(ce->outerFEC, "Hamming74") == 0) {
               strcpy(ce->outerFEC, "none");
           }
           if (strcmp(ce->outerFEC, "Hamming128") == 0) {
               strcpy(ce->outerFEC, "none");
           }
           if (strcmp(ce->outerFEC, "Golay2412") == 0) {
               strcpy(ce->outerFEC, "none");
           }
           if (strcmp(ce->outerFEC, "SEC-DED2216") == 0) {
               strcpy(ce->outerFEC, "none");
           }
           if (strcmp(ce->outerFEC, "SEC-DED3932") == 0) {
               strcpy(ce->outerFEC, "none");
           }
        }
        // FEC modifying (change to higher)
        if (strcmp(ce->option_to_adapt, "use_change_higher_fec") == 0) {
           if (strcmp(ce->outerFEC, "none") == 0) {
               strcpy(ce->outerFEC, "Hamming74");
           }
           if (strcmp(ce->outerFEC, "Hamming74") == 0) {
               strcpy(ce->outerFEC, "Hamming128");
           }
           if (strcmp(ce->outerFEC, "Hamming128") == 0) {
               strcpy(ce->outerFEC, "Golay2412");
           }
           if (strcmp(ce->outerFEC, "Golay2412") == 0) {
               strcpy(ce->outerFEC, "SEC-DED2216");
           }
           if (strcmp(ce->outerFEC, "SEC-DED2216") == 0) {
               strcpy(ce->outerFEC, "SEC-DED3932");
           }
           if (strcmp(ce->outerFEC, "SEC-DED3932") == 0) {
               strcpy(ce->outerFEC, "SEC-DED7264");
           } 
        }
        // FEC modifying (change to lower)
        if (strcmp(ce->option_to_adapt, "use_change_lower_fec") == 0) {
           if (strcmp(ce->outerFEC, "SEC-DED7264") == 0) {
               strcpy(ce->outerFEC, "SEC-DED3932");
           }
           if (strcmp(ce->outerFEC, "SEC-DED3932") == 0) {
               strcpy(ce->outerFEC, "SEC-DED2216");
           }
           if (strcmp(ce->outerFEC, "SEC-DED2216") == 0) {
               strcpy(ce->outerFEC, "Golay2412");
           }
           if (strcmp(ce->outerFEC, "Golay2412") == 0) {
               strcpy(ce->outerFEC, "Hamming128");
           }
           if (strcmp(ce->outerFEC, "Hamming128") == 0) {
               strcpy(ce->outerFEC, "Hamming74");
           }
        }

        if (strcmp(ce->option_to_adapt, "mod_scheme->BPSK") == 0) {
            strcpy(ce->modScheme, "BPSK");
        }
        if (strcmp(ce->option_to_adapt, "mod_scheme->QPSK") == 0) {
            strcpy(ce->modScheme, "QPSK");
        }
        if (strcmp(ce->option_to_adapt, "mod_scheme->8PSK") == 0) {
            strcpy(ce->modScheme, "8PSK");
        }
        if (strcmp(ce->option_to_adapt, "mod_scheme->16PSK") == 0) {
            strcpy(ce->modScheme, "16PSK");
        }
        if (strcmp(ce->option_to_adapt, "mod_scheme->328PSK") == 0) {
            strcpy(ce->modScheme, "32PSK");
        }
        if (strcmp(ce->option_to_adapt, "mod_scheme->64PSK") == 0) {
            strcpy(ce->modScheme, "64PSK");
        }
        if (strcmp(ce->option_to_adapt, "mod_scheme->8QAM") == 0) {
            strcpy(ce->modScheme, "8QAM");
        }
        if (strcmp(ce->option_to_adapt, "mod_scheme->16QAM") == 0) {
            strcpy(ce->modScheme, "16QAM");
        }
        if (strcmp(ce->option_to_adapt, "mod_scheme->32QAM") == 0) {
            strcpy(ce->modScheme, "32QAM");
        }
        if (strcmp(ce->option_to_adapt, "mod_scheme->64QAM") == 0) {
            strcpy(ce->modScheme, "64QAM");
        }
        if (strcmp(ce->option_to_adapt, "mod_scheme->OOK") == 0) {
            strcpy(ce->modScheme, "OOK");
        }
        if (strcmp(ce->option_to_adapt, "mod_scheme->4ASK") == 0) {
            strcpy(ce->modScheme, "4ASK");
        }
        if (strcmp(ce->option_to_adapt, "mod_scheme->8ASK") == 0) {
            strcpy(ce->modScheme, "8ASK");
        }
        if (strcmp(ce->option_to_adapt, "mod_scheme->16ASK") == 0) {
            strcpy(ce->modScheme, "16ASK");
        }
        if (strcmp(ce->option_to_adapt, "mod_scheme->32ASK") == 0) {
            strcpy(ce->modScheme, "32ASK");
        }
        if (strcmp(ce->option_to_adapt, "mod_scheme->64ASK") == 0) {
            strcpy(ce->modScheme, "64ASK");
        }
    }
    return 1;
}   // End ceModifyTxParams()

uhd::usrp::multi_usrp::sptr initializeUSRPs()
{
    //uhd::device_addr_t hint; //an empty hint discovers all devices
    //uhd::device_addrs_t dev_addrs = uhd::device::find(hint);
    //std::string str = dev_addrs[0].to_string();
    //const char * c = str.c_str();
    //printf("First UHD Device found: %s\n", c ); 

    //std::string str2 = dev_addrs[1].to_string();
    //const char * c2 = str2.c_str();
    //printf("Second UHD Device found: %s\n", c2 ); 

    uhd::device_addr_t dev_addr;
    // TODO: Allow setting of USRP Address from command line
    dev_addr["addr0"] = "type=usrp1,serial=8b9cadb0";
    //uhd::usrp::multi_usrp::sptr usrp= uhd::usrp::multi_usrp::make(dev_addr);
    //dev_addr["addr0"] = "8b9cadb0";
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(dev_addr);

    //Lock mboard clocks
    //usrp->set_clock_source(ref);

    // Set the TX freq (in Hz)
    usrp->set_tx_freq(450e6);
    printf("TX Freq set to %f MHz\n", (usrp->get_tx_freq()/1e6));
    // Wait for USRP to settle at the frequency
    while (not usrp->get_tx_sensor("lo_locked").to_bool()){
        usleep(1000);
        //sleep for a short time 
    }
    //printf("USRP tuned and ready.\n");
 
    // Set the rf gain (dB)
    // TODO: Allow setting of gain from command line
    usrp->set_tx_gain(0.0);
    printf("TX Gain set to %f dB\n", usrp->get_tx_gain());
 
    // Set the rx_rate (Samples/s)
    // TODO: Allow setting of tx_rate from command line
    usrp->set_tx_rate(1e6);
    printf("TX rate set to %f MS/s\n", (usrp->get_tx_rate()/1e6));

    // Set the IF BW (in Hz)
    usrp->set_tx_bandwidth(500e3);
    printf("TX bandwidth set to %f kHz\n", (usrp->get_tx_bandwidth()/1e3));

    return usrp;
} // end initializeUSRPs()


int postTxTasks(struct CognitiveEngine * cePtr, float * feedback, int debug)
{
    // TODO: Find another way to fix this
    usleep(1000000.0);

    int DoneTransmitting = 0;

    // Process data from rx
    ceProcessData(cePtr, feedback);
    // Modify transmission parameters (in fg and in USRP) accordingly
    if (!ceOptimized(*cePtr)) 
    {
        printf("ceOptimized() returned false\n");
        ceModifyTxParams(cePtr, feedback);
    }
    else
    {
        printf("ceOptimized() returned true\n");
        DoneTransmitting = 1;
        //printf("else: DoneTransmitting= %d\n", DoneTransmitting);
    }

    // For debugging
    if (debug)
    {
        printf("ce.numSubcarriers= %u\n", cePtr->numSubcarriers);
        printf("ce.CPLen= %u\n", cePtr->CPLen);
        printf("ce.numSubcarriers= %u\n", cePtr->numSubcarriers);
    }

    return DoneTransmitting;
} // End postTxTasks()

int main(int argc, char ** argv)
{
    // Seed the PRNG
    srand( time(NULL));

    // TEMPORARY VARIABLE
    int usingUSRPs = 1;

    int debug = 1;
    int verbose = 1;
    int dataToStdout = 0;

    // Check Program options
    int d;
    while ((d = getopt(argc,argv,"uhqvd")) != EOF) {
        switch (d) {
        case 'u':
        case 'h':   usage();                       return 0;
        case 'q':   verbose = 0;                      break;
        case 'v':   verbose = 1;                      break;
        case 'd':   dataToStdout = 1;                 break;
        //case 'f':   frequency = atof(optarg);       break;
        //case 'b':   bandwidth = atof(optarg);       break;
        //case 'G':   uhd_rxgain = atof(optarg);      break;
        //case 't':   num_seconds = atof(optarg);     break;
        default:
            verbose = 1;
        }   
    }   

    // Threading parameters (to open Server in its own thread)
    pthread_t TCPServerThread;   // Pointer to thread ID
    //int serverThreadReturn = 0;  // return value of creating TCPServer thread

    // Array that will be accessible to both Server and CE.
    // Server uses it to pass data to CE.
    float feedback[100];

    // For creating appropriate symbol length from 
    // number of subcarriers and CP Length
    unsigned int symbolLen;

    // Iterators
    int i_CE = 0;
    int i_Sc = 0;
    int DoneTransmitting = 0;
    //int N = 0;                 // Iterations of transmission before receiving.
    //int i_N = 0;
    int isLastSymbol = 0;
    char scenario_list [30][60];
    //printf ("Declared scenario array\n");
    char cogengine_list [30][60];
    
    //printf("variables declared.\n");

    int NumCE=readCEMasterFile(cogengine_list);  
    int NumSc=readScMasterFile(scenario_list);  
    //printf ("\nCalled readScMasterFile function\n");

    // Cognitive engine struct used in each test
    struct CognitiveEngine ce = CreateCognitiveEngine();
    // Scenario struct used in each test
    struct Scenario sc = CreateScenario();

    //printf("structs declared\n");
    // framegenerator object used in each test
    ofdmflexframegen fg;

    // framesynchronizer object used in each test
    // TODO: Once we are using USRPs, move to an rx.c file that will run independently.
    ofdmflexframesync fs;

    //printf("frame objects declared\n");

    // Buffers for packet/frame data
    unsigned char header[8];                       // Must always be 8 bytes for ofdmflexframe
    unsigned char payload[1000];                   // Large enough to accomodate any (reasonable) payload that
                                                   // the CE wants to use.

    // pointer for accessing header array when it has float values
    float * header_f = (float*) header;

    std::complex<float> frameSamples[10000];      // Buffer of frame samples for each symbol.
                                                   // Large enough to accomodate any (reasonable) payload that 
                                                   // the CE wants to use.
    // USRP objects
    uhd::tx_metadata_t metaData;
    uhd::usrp::multi_usrp::sptr usrp;
    uhd::tx_streamer::sptr txStream;
                                                   
    ////////////////////// End variable initializations.



    // Begin TCP Server Thread
    //serverThreadReturn = pthread_create( &TCPServerThread, NULL, startTCPServer, (void*) feedback);
    pthread_create( &TCPServerThread, NULL, startTCPServer, (void*) feedback);

    // Allow server time to finish initialization
    usleep(0.1e6);
    
    // Initialize Data File
    FILE * dataFile = fopen("data", "w");

    // Begin running tests

    // For each Cognitive Engine
    for (i_CE=0; i_CE<NumCE; i_CE++)
    {
        printf("\nStarting Tests on Cognitive Engine %d\n", i_CE+1);

        
        // Run each CE through each scenario
        for (i_Sc= 0; i_Sc<NumSc; i_Sc++)
        {
            // Initialize current CE
            ce = CreateCognitiveEngine();
            readCEConfigFile(&ce,cogengine_list[i_CE]);

            printf("\n\nStarting Scenario %d\n", i_Sc+1);
            // Initialize current Scenario
            sc = CreateScenario();
            readScConfigFile(&sc,scenario_list[i_Sc]);

            fprintf(dataFile, "Cognitive Engine %d\nScenario %d\n", i_CE+1, i_Sc+1);
            fprintf(dataFile, "frameNum\theader_valid\tpayload_valid\tevm\trssi\tPER\theaderBitErrors\tpayloadBitErrors\tBER:LastPacket\n");

            // Initialize Receiver Defaults for current CE and Sc
            // TODO: Once we are using USRPs, move to an rx.c file that will run independently.
            float frameNum = 0.0;
            fs = CreateFS(ce, sc, NULL);

            std::clock_t begin = std::clock();
            // Begin Testing Scenario
            DoneTransmitting = 0;

            //while(!DoneTransmitting)
            //{
                if (usingUSRPs) 
                {
                    //usrp = initializeUSRPs();    
                    // create transceiver object
                    unsigned char * p = NULL;   // default subcarrier allocation
                    if (debug) 
                        printf("Using ofdmtxrx\n");
                    ofdmtxrx txcvr(ce.numSubcarriers, ce.CPLen, ce.taperLen, p, NULL, NULL);

                    while(!DoneTransmitting)
                    {

                        //printf("DoneTransmitting= %d\n", DoneTransmitting);
                        //metaData.start_of_burst = false;
                        //metaData.end_of_burst   = false;  
                        //metaData.has_time_spec  = false; 

                        //uhd::stream_args_t stream_args("fc32"); // Sending complex floats to USRP
                        //txStream = usrp->get_tx_stream(stream_args);

                        // set properties
                        txcvr.set_tx_freq(ce.frequency);
                        txcvr.set_tx_rate(ce.bandwidth);
                        txcvr.set_tx_gain_soft(ce.txgain_dB);
                        txcvr.set_tx_gain_uhd(ce.uhd_txgain);
                        txcvr.set_tx_antenna("TX/RX");

                        if (debug) {
                            txcvr.debug_enable();
                            printf("Set frequency to %f\n", ce.frequency);
                            printf("Set bandwidth to %f\n", ce.bandwidth);
                            printf("Set txgain_dB to %f\n", ce.txgain_dB);
                            printf("Set uhd_txgain_dB to %f\n", ce.uhd_txgain);
                            printf("Set Tx antenna to %s\n", "TX/RX");
                        }

                        int i = 0;
                        // Generate data
                        printf("\n\nGenerating data that will go in frame...\n");
                        for (i=0; i<8; i++)
                            header[i] = i & 0xff;
                        for (i=0; i<ce.payloadLen; i++)
                            payload[i] = i & 0xff;
                        // Include frame number in header information
                        * header_f = frameNum;

                        // Set Modulation Scheme
                        modulation_scheme ms = convertModScheme(ce.modScheme);

                        // Set Cyclic Redundency Check Scheme
                        //crc_scheme check = convertCRCScheme(ce.crcSchere);

                        // Set inner forward error correction scheme
                        printf("Inner FEC: ");
                        fec_scheme fec0 = convertFECScheme(ce.innerFEC);

                        // Set outer forward error correction scheme
                        // TODO: add other liquid-supported FEC schemes
                        printf("Outer FEC: ");
                        fec_scheme fec1 = convertFECScheme(ce.outerFEC);

                        txcvr.transmit_packet(header, payload, ce.payloadLen, ms, fec0, fec1);

                        DoneTransmitting = postTxTasks(&ce, feedback, debug);
                        // Record the feedback data received
                        fprintf(dataFile, "%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", feedback[4], feedback[0], 
                                feedback[1], feedback[2], feedback[3], ce.PER, feedback[5], feedback[6], ce.BERLastPacket);

                        // Increment the frame counter
                        frameNum++;
                    } // End If while loop
                }
                else // If not using USRPs
                {
                    while(!DoneTransmitting)
                    {
                        // Initialize Transmitter Defaults for current CE and Sc
                        fg = CreateFG(ce, sc);  // Create ofdmflexframegen object with given parameters
                        ofdmflexframegen_print(fg);

                        // Generate data to go into frame (packet)
                        //txGeneratePacket(ce, &fg, header, payload);

                        // Iterator
                        int i = 0;
                        //printf("ce.payloadLen= %u", ce.payloadLen);

                        // Generate data
                        printf("\n\nGenerating data that will go in frame...\n");
                        for (i=0; i<8; i++)
                            header[i] = i & 0xff;
                        for (i=0; i<ce.payloadLen; i++)
                            payload[i] = i & 0xff;
                        // Include frame number in header information
                        * header_f = frameNum;

                        // Assemble frame
                        ofdmflexframegen_assemble(fg, header, payload, ce.payloadLen);

                        //printf("DoneTransmitting= %d\n", DoneTransmitting);

                        // i.e. Need to transmit each symbol in frame.
                        isLastSymbol = 0;
                        while (!isLastSymbol) 
                        {
                            //isLastSymbol = txTransmitPacket(ce, &fg, frameSamples, metaData, txStream, usingUSRPs);
                            isLastSymbol = ofdmflexframegen_writesymbol(fg, frameSamples);

                            enactScenario(frameSamples, ce, sc, usingUSRPs);

                            // TODO: Create this function
                            // Store a copy of the packet that was transmitted. For reference.
                            // txStoreTransmittedPacket();
                        
                            // Rx Receives packet
                            //rxReceivePacket(ce, &fs, frameSamples, usingUSRPs);
                            symbolLen = ce.numSubcarriers + ce.CPLen;
                            ofdmflexframesync_execute(fs, frameSamples, symbolLen);
                        } // End Transmition For loop

                        // posttransmittasks
                        DoneTransmitting = postTxTasks(&ce, feedback, debug);
                        // Record the feedback data received
                        fprintf(dataFile, "%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", feedback[4], feedback[0], 
                                feedback[1], feedback[2], feedback[3], ce.PER, feedback[5], feedback[6], ce.BERLastPacket);

                        // Increment the frame counter
                        frameNum++;
                    } // End else While loop
                }


            //} // End Test While loop
            clock_t end = clock();
            double time = double(end-begin)/CLOCKS_PER_SEC;
            fprintf(dataFile, "Elasped Time: %f (s)", time);

            // Reset the goal
            ce.latestGoalValue = 0.0;
            ce.errorFreePayloads = 0.0;
            printf("Scenario %i completed for CE %i.\n", i_Sc+1, i_CE+1);
            fprintf(dataFile, "\n\n");
            
        } // End Scenario For loop

        printf("Tests on Cognitive Engine %i completed.\n", i_CE+1);

    } // End CE for loop

    return 0;
}


