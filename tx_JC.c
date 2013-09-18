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

    // Iterator
    int i = 0;

    // Create a client TCP socket
	int socket_to_server = socket(AF_INET, SOCK_STREAM, 0);
	if( socket_to_server < 0)
	{
		printf("Failed to Create Client Socket\n");
		exit(1);
	}
	printf("Created client socket to server. socket_to_server: %d\n", socket_to_server);

    // Parameters for connecting to server
	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(PORT);
	servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Attempt connect client socket to server
	int connect_status;
	if((connect_status = connect(socket_to_server, (struct sockaddr*)&servAddr, sizeof(servAddr))))
	{
		printf("Failed to Connect to server from client\n");
		printf("connect_status = %d\n", connect_status);
		exit(1);
	}

    // Data that will be sent to server
    // TODO: Send useful data through feedback array
    float feedback[8];
    feedback[0] = 0;
    feedback[1] = 1;
    feedback[2] = 2;
    feedback[3] = 3;

	for (i=0; i<8; i++)
		printf("feedback data before transmission: %f\n", feedback[i]);

    // Receiver sends data to server
	printf("socket_to_server: %d\n", socket_to_server);
    int writeStatus = write(socket_to_server, feedback, 8);
        printf("writeStatus: %d\n", writeStatus);

	// Receiver closes socket to server
	close(socket_to_server);
	sleep(1);

    return 0;
}


// Create a TCP socket for the server and bind it to a port
void * CreateTCPServerSocket(/*int * sock_listen,*/ void * _read_buffer )
{
    printf("Server thread called.\n");

    // Iterator
    int i;

    // Buffer for data sent by client. This address is also given to CE
    float * read_buffer = (float *) _read_buffer;	

    struct sockaddr_in servAddr;   //  Local (server) address
	
	// Create socket for incoming connections 
    int sock_listen;
    if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Failed to Create Server Socket\n");
		exit(1);
	}
	printf("sock_listen= %d\n", sock_listen);
	      
    // Construct local (server) address structure 
    memset(&servAddr, 0, sizeof(servAddr));       /* Zero out structure */
    servAddr.sin_family = AF_INET;                /* Internet address family */
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    servAddr.sin_port = htons(PORT);              /* Local port */

    // Bind to the local address 
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
        printf("Server is now in listening mode\n");

        // Parameters of client connection
        struct sockaddr_in clientAddr; // Client address 
        int client_addr_size;			 // client address size
        client_addr_size = sizeof(clientAddr);

        // Accept a connection from client
        printf("server is waiting to accept connection from client\n");
        int socket_to_client;
        socket_to_client = accept(sock_listen, (struct sockaddr *)&clientAddr, &client_addr_size);
        printf("socket_to_client= %d\n", socket_to_client);

        if(socket_to_client< 0)
        {
            printf("Sever Failed to Connect to Client\n");
            exit(1);
        }
        printf("server has accepted connection from client\n");

        // Transmitter receives data from client (receiver)
        // Zero the read buffer. Then read the data into it.
        bzero(read_buffer, 256);
        int read_status = -1;   // indicates success/failure of read operation.
        read_status = read(socket_to_client, read_buffer, 255);

        // Print the data received
            printf("read_status= %d\n", read_status);
        printf("Server (transmitter) received:\n" );
        printf("readbuffer[0]= %f\n", read_buffer[0]);
        printf("readbuffer[1]= %f\n", read_buffer[1]);
    }

	// Transmitter (server) closes its sockets 
	
	// Wait so that the client can close the connection first.
	/*sleep(3);

	if  ( close(socket_to_client) < 0)
	{
		printf("Failure to close socket_to_client. errno: %d", errno);
	}

	if  ( close(sock_listen) < 0)
	{
		printf("Failure to close sock_listen. errno: %d", errno);
	}*/
    //return ;
}

int ce1(float * fb_buffer);

int main() {
    // data arrays for TCP
    float feedback[8];      // recevier's feedback data to be sent thru TCP
    float fb_buffer[256];   // buffer for feedback data after received through TCP

	// For threading
    pthread_t TCPServerThread; // Pointer to thread ID
    int iret2;                  // return value of creating TCPServer thread
	
    // Initialize server
    // Transmitter listens for receiver's incoming connection
    // Then accepts the connection
	//iret2 = pthread_create( &TCPServerThread, NULL, CreateTCPServerSocket, (void*) &sock_listen);
	iret2 = pthread_create( &TCPServerThread, NULL, CreateTCPServerSocket, (void*) fb_buffer);
    sleep(2); // Delay to be sure server has finished initialization

    // Initialize Cognitive Engine
    // Pass it the array which will be filled with the frame samples
    // Also pass the array which will contain the feedback received through TCP connection.
    // TODO: Implement as own pthread?
    ce1(/*y,*/ fb_buffer);

    /*
	// Transmitter closes master (server) socket
	printf("&sock_listen_main= %d\n", &sock_listen);
	close(sock_listen);
    */

    printf("done.\n");
    return 0;
}
