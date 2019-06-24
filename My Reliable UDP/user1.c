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

 	struct sockaddr_in user1_addr ; 
 	socklen_t user1_len = sizeof(user1_addr) ;  	
 	memset( &user1_addr, '\0' , user1_len ) ; 
 	user1_addr.sin_family = AF_INET ; 
 	user1_addr.sin_addr.s_addr = INADDR_ANY ; 
 	user1_addr.sin_port = htons(50000 + 2*ROLL ) ; 

 	if ( r_bind(socket_fd, (struct sockaddr*)&user1_addr, user1_len ) < 0 ){
 		perror(RED"UDP Socket Bind Failed "WHITE) ; 
 		exit(EXIT_FAILURE) ; 
 	}

 	char* buffer = (char*)calloc(sizeof(char), MAX_SIZE+5) ; 

 	fprintf(stdout , CYAN"Please Enter a String ( 25 < Length < 100 ) : "WHITE ) ; 
 	fgets( buffer , MAX_SIZE , stdin ) ; 	buffer[strlen(buffer)-1] = '\0' ; 
 	
 	struct sockaddr_in user2_addr ; 
 	socklen_t user2_len = sizeof(user2_addr) ; 
 	memset( &user2_addr , '\0' , user2_len) ;
 	user2_addr.sin_family = AF_INET ; 
 	user2_addr.sin_addr.s_addr = INADDR_ANY ; 
 	user2_addr.sin_port = htons(50000 + 2*ROLL + 1 ) ;  

 	int i ; 
 	for ( i = 0 ; i < strlen(buffer) ; i++ ){
 		if ( r_sendto( socket_fd , buffer+i , sizeof(char), 0, (struct sockaddr*)&user2_addr, user2_len ) < 0 ){
 			perror(RED"sendto() Failed "WHITE) ; 
 			exit(EXIT_FAILURE) ; 
 		}
 	}

 	r_close(socket_fd) ;

 	fprintf(stderr, "Program Exit Success\n" );
 	exit(EXIT_SUCCESS) ; 

 }
