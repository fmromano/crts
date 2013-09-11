#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <liquid/liquid.h>
//TCP Header Files
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <sys/types.h>
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
//#define RCVBUFSIZE 32   /* Size of receive buffer */

#define MAXPENDING 5    /* Maximum outstanding connection requests */
#define BUFF_SIZE 1024  /* Buffer Size*/

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

int CreateTCPServerSocket(unsigned short port)
{
    int sock;                      /* socket to create */

    struct sockaddr_in ServAddr;   /* Local address */
	
	/* Create socket for incoming connections */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		printf("Fail to Create Server Socket\n");
		exit(1);
	}
	      
    /* Construct local address structure */
    memset(&ServAddr, 0, sizeof(ServAddr));       /* Zero out structure */
    ServAddr.sin_family = AF_INET;                /* Internet address family */
    ServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    ServAddr.sin_port = htons(port);              /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) < 0)
	{
		printf("bind() error\n");
		exit(1);
	}
    return sock;
}

int ClientProgram(unsigned short port, int argc, char **argv)
{
	int client_socket;
	struct sockaddr_in ServAddr;
	char buff[BUFF_SIZE+MAXPENDING];

	client_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(client_socket < 0)
	{
		printf("Fail to Build Socket\n");
		exit(1);
	}

	memset(&ServAddr, 0, sizeof(ServAddr));
	ServAddr.sin_family = AF_INET;
	ServAddr.sin_port = htons(port);
	ServAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connet(client_socket, (struct sockaddr*)&ServAddr, sizeof(ServAddr)))
	{
		printf("Fail to Connet\n");
		exit(1);
	}
	send(client_socket, argv[1], strlen(argv[1]+1, 0));
	recv(client_socket, buff, BUFF_SIZE, 0);
	printf("%s\n", buff);
	close(client_socket);
	return 0;
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
	
	// defining client
	int client_socket;				 /* client socket */
	int client_addr_size;			 /* client address size*/
	struct sockaddr_in clientServAddr; /*Client address */


	// defining buffer size for receive and send
	char buff_rcv[BUFF_SIZE+MAXPENDING]; // buffer size of recevier Side
	char buff_snd[BUFF_SIZE+MAXPENDING]; // buffer size of transmitting side

    /////////////////
    // TRANSMITTER
    // create frame generator

    // Open socket to receive feed back from RF Rx
    int sock = CreateTCPServerSocket(28367); 
    printf("sock= %d", sock);
    
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

	While(1)
	{
		if (listen(sock, MAXPENDING) < 0)
		{
			printf("Fail to Set Sleeping Mode\n");
			exit(1);
		}

		client_addr_size = sizeof(clientServAddr);
		client_soket = accept(sock, struct sockaddr *)&clientSerAddr, client_addr_size);

		if(client_socket < 0)
		{
			printf("Fail to Connet Client\n");
			exit(1);
		}

		recv (client_socket, buff_rcv, BUFF_SIZE,0);
		printf("receive: %s\n", buff_rcv);
		sprintf( buff_snd, "%d : %s", strlen(buff_rcv), buff_rcv);
		send(client_socket, buff_snd, strlen(buff_snd)+1,0);
		close(client_socket);
	}
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

    // DESTROY objects
    framegen64_destroy(fg);
    framesync64_destroy(fs);

	// Client
	//ClientProgram(28367,    ,    );

    printf("received %u frames\n", frame_counter);
    printf("done.\n");
    return 0;
}
