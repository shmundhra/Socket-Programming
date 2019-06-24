#include "rsocket.h"
#define ROLL 57 
#define MAX_SIZE 100

 int main() 
 {
 	int socket_fd = r_socket(AF_INET, SOCK_MRP, 0) ; 
 	if ( socket_fd < 0 ){
 		perror(RED"UDP Socket Creation Failure "WHITE) ; 
 		exit(EXIT_FAILURE) ; 
 	} 
 	int enable = 1 ; 
	if ( setsockopt( socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable) ) < 0 )	{
		perror("setsockopt(SO_REUSEADDR) ") ; 				
	}	
 	
 	struct sockaddr_in user2_addr ; 
 	socklen_t user2_len = sizeof(user2_addr) ; 
 	memset( &user2_addr , '\0' , user2_len) ;
 	user2_addr.sin_family = AF_INET ; 
 	user2_addr.sin_addr.s_addr = INADDR_ANY ; 
 	user2_addr.sin_port = htons(50000 + 2*ROLL + 1 ) ;  

 	if ( r_bind(socket_fd, (struct sockaddr*)&user2_addr, user2_len ) < 0 ){
 		perror(RED"UDP Socket Bind Failed "WHITE) ; 
 		exit(EXIT_FAILURE) ; 
 	}	

	fprintf(stderr, GREEN"Socket Bound to Port and Waiting for a Receive...\n"WHITE );

 	while(1)
 	{	
 		char c[MAX_SIZE] ;  	
 		int recv_bytes = 0 ; 
 		struct sockaddr_in user1_addr ; 
 		socklen_t user1_len = sizeof(user1_addr) ; 

 		if ( (recv_bytes = r_recvfrom( socket_fd, c, MAX_SIZE , 0 , (struct sockaddr*)&user1_addr, &user1_len )) < 0 ){
 			perror(RED"recvfrom() Failed "WHITE) ; 
 			exit(EXIT_FAILURE) ; 
 		}

 		fprintf( stdout , MAGENTA"Character Received is : " BLUE ) ; 
 		for ( int i = 0 ; i < recv_bytes ; i++ ) fprintf(stdout, "%c", c[i] );  fprintf(stdout, "\t"); 
 	}

 	r_close(socket_fd) ; 
}