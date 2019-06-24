#ifndef _rsocket_h_
#define _rsocket_h_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/*
	POSIX.1-2001 does not require the inclusion of certain header files.
	However Some historical (BSD) implementations required these header files, 
	and portable applications are probably wise to include them.
*/
#include <sys/types.h> // Used in tandem with sys/socket.h 
#include <sys/socket.h>
/*
	socket(), setsockopt(), bind(), sendto(), recvfrom() 
*/
#include <sys/time.h>
/*
	struct timeval
*/
#include <arpa/inet.h>    //Inclusion of <arpa/inet.h> makes visible all symbols in <netinet/in.h> 
#include <netinet/in.h>
/*
	htons(), ntohs(), inet_aton(), inet_ntoa(), sockaddr_in
*/
#include <sys/select.h>
/*
	select(), FD_SET(), FD_CLR(), FD_ISSET(), FD_ZERO()
*/
#include <fcntl.h>
/*
	for various control flags: O_CREAT, O_RDONLY, O_WRONLY, O_NONBLOCK 
*/
#include <pthread.h>
/*
	for POSIX THREADS 
*/
#include <signal.h>
/*
	for Signals 
*/

#define T 2
#define P 0.2
#define MAX_CONSECUTIVE_TIMEOUTS 120/T
#define SOCK_MRP -2
#define MAXSIZE_MSG 100
#define MAXSIZE_TABLE 100
#define TYPE_SIZE sizeof(char)
#define ID_SIZE sizeof(short int)
#define MAXSIZE_PACKET (MAXSIZE_MSG + TYPE_SIZE + ID_SIZE)

#define BLACK "\033[1;30m"
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE  "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN "\033[1;36m"
#define WHITE "\033[1;37m" 
#define RESET "\033[0m"

int r_socket	( int domain, int type, int protocol ) ; 
int r_bind 		( int socket_fd, const struct sockaddr *address, socklen_t address_len ) ; 
int r_sendto	( int socket_fd, const void* message, size_t length, int flags, const struct sockaddr *dest_address, socklen_t dest_len ) ;
int r_recvfrom	( int socket_fd, void* message, size_t length, int flags, struct sockaddr *source_address, socklen_t *source_len ) ; 
int r_close		( int socket_fd ) ; 

int HandleRetransmit ( int ) ; 
int HandleReceive	 ( int ) ; 
int HandleAppMsg	 ( int, char *, size_t, struct sockaddr*, socklen_t ) ; 
int HandleAckMsg	 ( int, char *, size_t, struct sockaddr*, socklen_t ) ; 
int dropMessage 	 ( float p ) ; 
void* Thread_X		 (void*) ; 
void Signal_Handler  ( int ) ; 

#endif 

