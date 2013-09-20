#ifndef TXJC_H_
#define TXJC_H_

int mycallback(unsigned char *  _header,
                        int              _header_valid,
                        unsigned char *  _payload,
                        unsigned int     _payload_len,
                        int              _payload_valid,
                        framesyncstats_s _stats,
                        void *           _userdata);

void * CreateTCPServerSocket(/*int * sock_listen,*/ void * _read_buffer );

#endif
