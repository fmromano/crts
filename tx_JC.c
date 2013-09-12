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
//#define RCVBUFSIZE 32   /* Size of receive buffer */
#define PORT 1351

#define MAXPENDING 5    /* Maximum outstanding connection requests */
#define BUFF_SIZE 8 	/* Buffer Size*/

// user-defined static callback function
static int mycallback(unsigned char *  _header,
                      int              _header_valid,
                      unsigned char *  _payload,
                      unsigned int     _payload_len,
                      int              _payload_valid,
                      framesyncstats_s _stats,
                      void *           _userdata)
{
    printf("***** callback invoked!\n");
    printf("  header (%s)\n",  _header_valid  ? "valid" : "INVALID");
    printf("  payload (%s)\n", _payload_valid ? "valid" : "INVALID");

    // type-cast, de-reference, and increment frame counter
    //unsigned int * counter = (unsigned int *) _userdata;
    //(*counter)++;
    float * fbarray = (float *) _userdata;
    fbarray[0] = _stats.rssi;
    fbarray[1] = _stats.evm;

    return 0;
}


// Create a TCP socket for the server and bind it to a port
void * CreateTCPServerSocket(int * sock_listen)
{
	
    printf("server thread called\n");
	printf("*sock_listen= %d\n", sock_listen);
    int i;
    struct sockaddr_in ServAddr;   /* Local address */
    int socket_to_client;
    struct sockaddr_in clientServAddr; /*Client address */
    int client_addr_size;			 /* client address size*/
	float buffer[256];	// Buffer for data from client
	int read_status = -1;   // indicates success/failure of read operation.
	
	/* Create socket for incoming connections */
    if ((*sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Failed to Create Server Socket\n");
		exit(1);
	}
	      
    /* Construct local address structure */
    memset(&ServAddr, 0, sizeof(ServAddr));       /* Zero out structure */
    ServAddr.sin_family = AF_INET;                /* Internet address family */
    ServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    ServAddr.sin_port = htons(PORT);              /* Local port */

    /* Bind to the local address */
    if (bind(*sock_listen, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) < 0)
	{
		printf("bind() error\n");
		exit(1);
	}
	
	// listen for connections on socket
    if (listen(*sock_listen, MAXPENDING) < 0)
	{
		printf("Failed to Set Sleeping (listening) Mode\n");
		exit(1);
	}
	printf("Server is now in listening mode\n");

	client_addr_size = sizeof(clientServAddr);

	printf("server is waiting to accept connection from client\n");
	socket_to_client = accept(*sock_listen, (struct sockaddr *)&clientServAddr, &client_addr_size);
        printf("socket_to_client= %d\n", socket_to_client);

	if(socket_to_client< 0)
	{
		printf("Sever Failed to Connect to Client\n");
		exit(1);
	}
	printf("server has accepted connection from client\n");

	// Transmitter receives data from client (receiver)
	// Zero the read buffer. Then read the data into it.
	bzero(buffer, 256);
	read_status = read(socket_to_client, buffer, 255);
	// Print the data received
        printf("read_status= %d\n", read_status);
	printf("Server (transmitter) received:\n" );
	//for (i=0; i<2; i++)
                 //printf("%f\n", buffer[i]);
	printf("rssi= %f\n", buffer[0]);
	printf("evm=  %f\n", buffer[1]);

	// Transmitter (server) closes its sockets 
	
	// Wait so that the client can close the connection first.
	/*sleep(3);

	if  ( close(socket_to_client) < 0)
	{
		printf("Failure to close socket_to_client. errno: %d", errno);
	}

	if  ( close(*sock_listen) < 0)
	{
		printf("Failure to close *sock_listen. errno: %d", errno);
	}*/
    return ;
}


int main() {
    // options
    unsigned int frame_counter = 0; // userdata passed to callback
    float phase_offset=0.3f;        // carrier phase offset
    float frequency_offset=0.02f;   // carrier frequency offset
    float SNRdB = 10.0f;            // signal-to-noise ratio [dB]
    float noise_floor = -40.0f;     // noise floor [dB]

    // allocate memory for arrays
    unsigned char header[8];        // data header
    unsigned char payload[64];      // data payload
    float complex y[1340];          // frame samples
    float feedback[8];      // recevier's feedback data for tx thru TCP

	// For threading
        pthread_t TCPServerThread; // Pointer to thread ID
        int iret2; 			// return value of creating TCPServer thread
	
	int socket_to_client;			// Socket server uses to connect to client

	// defining client's connection to server
	//int server_port = 1302;		// Port for the server to use
        int socket_to_server = 300;		// Socket client will use to connect to server
	int sock_listen; 			// Server's listening socket



    /////////////////
    // TRANSMITTER
    // create frame generator

    framegen64 fg = framegen64_create();
    framegen64_print(fg);

    // initialize header, payload
    unsigned int i;
    for (i=0; i<8; i++)
        header[i] = i;
    for (i=0; i<64; i++)
        payload[i] = rand() & 0xff;

    // EXECUTE generator and assemble the frame
    framegen64_execute(fg, header, payload, y);

/*
    ////////////////////////////
    // NOISE
    // add channel impairments (attenuation, carrier offset, noise)
    float nstd  = powf(10.0f, noise_floor/20.0f);        // noise std. dev.
    float gamma = powf(10.0f, (SNRdB+noise_floor)/20.0f);// channel gain
    for (i=0; i<1340; i++) {
        y[i] *= gamma;
        y[i] *= cexpf(_Complex_I*(phase_offset + i*frequency_offset));
        y[i] += nstd * (randnf() + _Complex_I*randnf())*M_SQRT1_2;
    }
*/

    ///////////////////////
    //  RECEIVER
    // EXECUTE synchronizer and receive the frame one sample at a time
    // create frame synchronizer using default properties


    framesync64 fs = framesync64_create(mycallback,
                                        (void*)feedback);
    framesync64_print(fs);

    for (i=0; i<1340; i++)
        framesync64_execute(fs, &y[i], 1);

    printf("received %u frames\n", frame_counter);

    // DESTROY objects
    framegen64_destroy(fg);
    framesync64_destroy(fs);


    ///////////////////////////
    // TCP/Threading Section
    ///////////////////////////

    // Transmitter listens for receiver's incoming connection
    // Then accepts the connection
	iret2 = pthread_create( &TCPServerThread, NULL, CreateTCPServerSocket, (void*) &sock_listen);

    // Receiver opens a TCP socket to act as a client. Opened as a new thread
	sleep(3);
	//unsigned short port = 1302;
	struct sockaddr_in ServAddr;
	int connect_status;

	socket_to_server = socket(AF_INET, SOCK_STREAM, 0);
	if( socket_to_server < 0)
	{
		printf("Failed to Create Client Socket\n");
		exit(1);
	}
	printf("Created client socket to server. socket_to_server: %d\n", socket_to_server);

	memset(&ServAddr, 0, sizeof(ServAddr));
	ServAddr.sin_family = AF_INET;
	ServAddr.sin_port = htons(PORT);
	ServAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connect_status = connect(socket_to_server, (struct sockaddr*)&ServAddr, sizeof(ServAddr)))
	{
		printf("Failed to Connect to server from client\n");
		printf("connect_status = %d\n", connect_status);
		exit(1);
	}

    // Code for Receiver to determine and encode data that will go over 
    // the TCP socket to the Transmitter will go here.
    // Arbirtary data for now. 
    //for (i=0; i<8; i++)
     //   feedback[i] = (char)((int)'a'+i);

	for (i=0; i<8; i++)
		printf("feedback data before transmission: %f\n", feedback[i]);

    // Receiver sends data to server
    sleep(3);
	printf("socket_to_server: %d\n", socket_to_server);
    int writeStatus = write(socket_to_server, feedback, 8);
        printf("writeStatus: %d\n", writeStatus);

	// Receiver closes socket to server
	close(socket_to_server);
	sleep(1);
	// Transmitter closes master (server) socket
	printf("&sock_listen_main= %d\n", &sock_listen);
	close(sock_listen);


    printf("done.\n");
    return 0;
}
