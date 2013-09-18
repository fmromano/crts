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
//#define RCVBUFSIZE 32   /* Size of receive buffer */
#define PORT 1321

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
    unsigned int * counter = (unsigned int *) _userdata;
    (*counter)++;

    return 0;
}


// Create a TCP socket for the server and bind it to a port
void * CreateTCPServerSocket(/*int * sock_listen*/)
{
    printf("server thread called\n");
    int i;
    int sock_listen;
    unsigned short port = 1302;

    struct sockaddr_in ServAddr;   /* Local address */
    int socket_to_client;

    struct sockaddr_in clientServAddr; /*Client address */
    int client_addr_size;			 /* client address size*/
	 
    // defining buffer size for receive and send
	int buff_rcv[BUFF_SIZE+MAXPENDING]; // buffer size of recevier Side
	
	/* Create socket for incoming connections */
    if ((sock_listen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
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
    if (bind(sock_listen, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) < 0)
	{
		printf("bind() error\n");
		exit(1);
	}
	

    if (listen(sock_listen, MAXPENDING) < 0)
	{
		printf("Failed to Set Sleeping (listening) Mode\n");
		exit(1);
	}

	client_addr_size = sizeof(clientServAddr);

    while (1) 
    {
	socket_to_client = accept(sock_listen, (struct sockaddr *)&clientServAddr, &client_addr_size);
        printf("socket_to_client= %d\n", socket_to_client);

/*	if(socket_to_client< 0)
	{
		printf("Sever Failed to Connect to Client\n");
		exit(1);
	}
*/

	// Transmitter receives data from client (receiver)
	int recv_status = recv(socket_to_client, buff_rcv, BUFF_SIZE,0);
        printf("recv_status= %d", recv_status);
	printf("Server (transmitter) received:\n" );
	for (i=0; i<8; i++)
		printf("%d\n", buff_rcv[i]);
	//sprintf( buff_snd, "%d : %s", strlen(buff_rcv), buff_rcv);
	//send(socket_to_client, buff_snd, strlen(buff_snd)+1,0);
	// Transmitter (server) closes its socket to client
	close(socket_to_client);
        
        sleep(1);
    }


    return ;
}


void * CreateAndConnectTCPClientSocket(/* unsigned short port,*/ void * cl_socket )
{
	printf("client thread called\n");
        int * client_socket = &cl_socket;
	unsigned short port = 1302;
	struct sockaddr_in ServAddr;
	char buff[BUFF_SIZE+MAXPENDING];
	int conn;

	* client_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(* client_socket < 0)
	{
		printf("Failed to Create Client Socket\n");
		exit(1);
	}

	memset(&ServAddr, 0, sizeof(ServAddr));
	ServAddr.sin_family = AF_INET;
	ServAddr.sin_port = htons(PORT);
	ServAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(conn = connect(* client_socket, (struct sockaddr*)&ServAddr, sizeof(ServAddr)))
	{
		printf("Failed to Connect to server from client\n");
		printf("conn = %d\n", conn);
		exit(1);
	}

	return;
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
   // float complex y[1340];          // frame samples
    float complex y[10];
    int feedback[8];      // recevier's feedback data for tx thru TCP

	// For threading
	pthread_t TCPClientThread;
        pthread_t TCPServerThread; // Pointer to thread ID
	int iret1; 			// return value of creating TCPclient thread
        int iret2; 			// return value of creating TCPServer thread
	
	int socket_to_client;			// Socket server uses to connect to client

	// defining client's connection to server
	//int server_port = 1302;		// Port for the server to use
        int socket_to_server;		// Socket client will use to connect to server
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
//        payload[i] = rand() & 0xff;
        payload[i] = rand() % 1 & 0xff;
    // EXECUTE generator and assemble the frame
    framegen64_execute(fg, header, payload, y);
    printf("%d/n",y);

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
                                        (void*)&frame_counter);
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
	iret2 = pthread_create( &TCPServerThread, NULL, CreateTCPServerSocket, NULL);

    // Receiver opens a TCP socket to act as a client. Opened as a new thread
	iret1 = pthread_create( &TCPClientThread, NULL, CreateAndConnectTCPClientSocket, &socket_to_server );
    //socket_to_server = CreateAndConnectTCPClientSocket( server_port);


    // Code for Receiver to determine and encode data that will go over 
    // the TCP socket to the Transmitter will go here.
    // Arbirtary data for now. 
    for (i=0; i<8; i++)
        feedback[i] = i;

	for (i=0; i<8; i++)
		printf("feedback data before transmission: %d\n", feedback[i]);

    // Receiver sends data to server
    sleep(3);
    int sendStatus = send(socket_to_server, feedback, 8, 0);
        printf("sendStatus: %d\n", sendStatus);

    sleep(3);

	// Receiver closes socket to server
	close(socket_to_server);

	// Transmitter closes master (server) socket
	close(sock_listen);


    printf("done.\n");
    return 0;
}
