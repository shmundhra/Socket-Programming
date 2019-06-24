/* Including necessary Header Files */
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/select.h>
    #include <sys/wait.h>
    #include <signal.h>
    #include <sys/time.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <linux/ip.h> 	/* for ipv4 header */
    #include <linux/udp.h>  /* for upd header */
    #include <linux/icmp.h>  /* for icmp header */

/* Defining necessary Colours */
    #define BLACK "\033[1;30m"
    #define RED "\033[1;31m"
    #define GREEN "\033[1;32m"
    #define YELLOW "\033[1;33m"
    #define BLUE "\033[1;34m"
    #define MAGENTA "\033[1;35m"
    #define CYAN "\033[1;36m"
    #define WHITE "\033[1;37m"
    #define RESET "\033[0m"

#define MAX_SIZE 101
#define PAYLOAD_SIZE 52
#define Dest_Port 32164
#define PORT_NO 8080
#define T 1

void print( char* buf , int len ){
    for ( int i = 0 ; i < len ; i++){
        fprintf(stderr, "%c", buf[i] );
    }
    fprintf(stderr, "\n");
}

void Print( int TTL_Value, struct sockaddr_in* Addr , struct timeval* Send, struct timeval* Recv )
{
    if ( Addr == NULL ){
        fprintf(stdout, GREEN "HOP VALUE" BLUE "(%d) " MAGENTA "\t*" YELLOW "\t*" WHITE "\n" , TTL_Value);
        return ;
    }
    fprintf(stdout, GREEN "HOP VALUE" BLUE "(%d) " , TTL_Value );
    fprintf(stdout, MAGENTA "%s", inet_ntoa(Addr->sin_addr) );
    fprintf(stdout, YELLOW " %ldus", ( (Recv->tv_sec)*1000000+(Recv->tv_usec) ) - ( (Send->tv_sec)*1000000+(Send->tv_usec) ) );
    fprintf(stdout, WHITE "\n" );

}

int main( int argc, char* argv[] )
{
/* Extracting Domain Information from Domain Name Given as a Command Line Arguement 1*/
    char Dest_Domain[MAX_SIZE] ;
    if ( argc == 1 ) {
        sprintf( Dest_Domain, "localhost" );
    }
    else {
        sprintf( Dest_Domain, "%s", argv[1] );
    }
/* Extracting Printing Flag (an integer) Given as a Command Line Arguement 2*/
    int p = 0 ;
    if ( argc >= 3 ){
        if ( sscanf(argv[2], "%d", &p) < 0 ){
            fprintf(stderr, RED"Invalid Second Argument"WHITE) ;
            exit(EXIT_FAILURE) ;
        }
    }
/* Getting IP Address of Requested Domain */
    struct hostent* HOSTENT = gethostbyname(Dest_Domain);
    if( !HOSTENT ){
        fprintf(stderr, RED"%s"WHITE " : Domain Not Available\n", Dest_Domain ) ;
        exit(EXIT_FAILURE) ;
    }
    struct in_addr Dest_IPAddr = *((struct in_addr* )(HOSTENT->h_addr)) ;
    if(p) fprintf(stderr, WHITE"IP Address of Requested Domain : %s\n\n", inet_ntoa(Dest_IPAddr)) ;

/* Setting up Local Address sockaddr structure for Binding */
    int port = PORT_NO ; 										// ((getpid() & 0xffff)|(0x8000))
    struct sockaddr_in Server ;
    Server.sin_family 		= AF_INET ;
    Server.sin_port 		= htons(port) ;
    Server.sin_addr.s_addr 	= INADDR_ANY ;
    socklen_t server_len 	= sizeof(Server);

/* Create Destination sockaddr */
    struct sockaddr_in Destination ;
    Destination.sin_family 	= AF_INET ;
    Destination.sin_port 	= htons(Dest_Port) ;
    Destination.sin_addr 	= Dest_IPAddr ;
    socklen_t Dest_Len 		= sizeof(Destination) ;

/* Create RAW Socket S1 with UDP Protocol, set socket options IP_HDRINCL , SO_REUSEADDR and Bind to Local Address */
    int udp_socket = socket( AF_INET, SOCK_RAW, IPPROTO_UDP );
    if ( udp_socket < 0 ){
        perror( RED "UDP SOCKET S1 Creation" WHITE ) ;
        exit(EXIT_FAILURE) ;
    }
    const int enable = 1 ;
    if ( setsockopt( udp_socket, IPPROTO_IP, IP_HDRINCL, &enable, sizeof(enable) ) < 0 ){
        perror( RED "setsockopt(IP_HDRINCL)" WHITE ) ;
        close(udp_socket) ;
        exit(EXIT_FAILURE) ;
    }
    if ( setsockopt( udp_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable) ) < 0 ){
        perror( RED "setsockopt(SO_REUSEADDR)" WHITE ) ;
    }
    if ( bind( udp_socket, (struct sockaddr*)&Server, server_len ) < 0 ){
        perror( RED "UDP SOCKET BIND" WHITE ) ;
        close(udp_socket) ;
        exit(EXIT_FAILURE) ;
    }

/* Create RAW Socket S2 with ICMP Protocol and Bind to Local Address */
    int icmp_socket = socket( AF_INET, SOCK_RAW, IPPROTO_ICMP ) ;
    if ( icmp_socket < 0 ){
        perror( RED "ICMP SOCKET S2 Creation" WHITE ) ;
        close(udp_socket) ;
        exit(EXIT_FAILURE) ;
    }
    if ( bind( icmp_socket, (struct sockaddr*)&Server, server_len ) < 0 ){
        perror( RED "ICMP SOCKET BIND" WHITE ) ;
        close(udp_socket) ;
        close(icmp_socket) ;
        exit(EXIT_FAILURE) ;
    }

/* Iterating on Incremental Max Hop Values to check the Route of the Packet */
    for ( int TTL = 1 ; TTL < 31 ; TTL++ )
    {
        int Done = 0 ;
        for( int try = 0 ; try < 3 ; try++ )
        {
        /* Create UDP DATAGRAM */

            /* Create UDP DATAGRAM PAYLOAD*/
                struct timeval seed ; gettimeofday(&seed, NULL) ; srand(seed.tv_usec) ;
                char PAYLOAD[MAX_SIZE] ;
                for ( int i = 0 ; i < PAYLOAD_SIZE; i++ ){
                    PAYLOAD[i] = rand()%(26) + 'a' ;
                }
                PAYLOAD[PAYLOAD_SIZE] = '\0' ;

            /* Create UDP DATAGRAM UDP Header */
                struct udphdr UDP_HDR ;
                int UDP_HDRLEN 		= sizeof(UDP_HDR);
                int DATAGRAM_Len 	= UDP_HDRLEN + PAYLOAD_SIZE ;
                UDP_HDR.source 		= Server.sin_port ;
                UDP_HDR.dest 		= Destination.sin_port ;
                UDP_HDR.len 		= htons(DATAGRAM_Len) ;
                UDP_HDR.check 		= 0 ;

            /* Create complete UDP DATAGRAM */
                char UDP_DATAGRAM[MAX_SIZE] ;
                memcpy(UDP_DATAGRAM , &UDP_HDR, UDP_HDRLEN ) ;
                memcpy(UDP_DATAGRAM+UDP_HDRLEN, PAYLOAD, PAYLOAD_SIZE ) ;
                UDP_DATAGRAM[DATAGRAM_Len] = '\0' ;

        /* Create IP Header */
            struct iphdr IP_HDR ;
            int IP_HDRLEN 		= sizeof(IP_HDR);
            int PACKET_Len		= IP_HDRLEN + DATAGRAM_Len ;
            IP_HDR.ihl 			= (IP_HDRLEN >> 2) ;    	// 5
            IP_HDR.version 		= 4 ;  						// IPv4
            IP_HDR.tos 			= 0 ;                       // IP Takes Care
            IP_HDR.tot_len 		= htons(PACKET_Len) ;
            IP_HDR.id 			= 0 ; 						// IP Takes Care
            IP_HDR.frag_off 	= 0 ;
            IP_HDR.ttl 			= TTL ;
            IP_HDR.protocol 	= IPPROTO_UDP ;
            IP_HDR.check 		= 0 ; 						// IP Takes Care
            IP_HDR.saddr 		= Server.sin_addr.s_addr ;
            IP_HDR.daddr 		= Destination.sin_addr.s_addr ;

        /* Create Complete IP Packet sendto() Destination */

            /* Create Packet */
                 char PACKET[MAX_SIZE] ;
                 memcpy( PACKET, &IP_HDR, IP_HDRLEN ) ;
                 memcpy( PACKET+IP_HDRLEN, &UDP_DATAGRAM, DATAGRAM_Len )	;
                 PACKET[PACKET_Len] = '\0' ;
                //print(PACKET, PACKET_Len ) ;

            /* Sending to Destination over S1 RAW UDP Socket */
                 if ( sendto( udp_socket, PACKET, PACKET_Len, 0 , (struct sockaddr*)&Destination, Dest_Len ) < 0 )
                 {
                     perror( RED "UDP Packet sendto()" WHITE ) ;
                     close(udp_socket) ;
                    close(icmp_socket) ;
                     exit(EXIT_FAILURE) ;
                 }
                 struct timeval SendTime ; gettimeofday(&SendTime, NULL) ;

        /* select() on the RAW ICMP Socket and wait for a ICMP Message Receive or a Timeout of 1 second */
            fd_set Recv_FDS ;
            int Max_FD = icmp_socket + 1 ;
            struct timeval Timeout = { T , 0 } ;

            int ValidPacket = 0 ;
            while( !ValidPacket )
            {
                FD_ZERO( &Recv_FDS ) ;
                FD_SET( icmp_socket, &Recv_FDS ) ;

                if(p) fprintf(stderr, RED "Waiting on a select() call..." WHITE ) ;

                int Ready_FD = 0 ;
                if ( (Ready_FD = select(Max_FD, &Recv_FDS, NULL, NULL, &Timeout)) < 0 ){
                    perror( RED "select() on ICMP Socket Timeout " WHITE ) ;
                    close(udp_socket) ;
                    close(icmp_socket) ;
                    exit(EXIT_FAILURE) ;
                }

            /* Check for a Timeout */
                if ( Ready_FD == 0 ){
                    if(p) fprintf(stderr, "Timeout Occured\n") ; fflush(stdout) ;
                    Timeout = (struct timeval ){ T, 0 } ;
                    Done = 0 ;
                    break ;
                }

            /* Check for a Receive */
                if ( FD_ISSET( icmp_socket, &Recv_FDS) ){

                    if(p) fprintf(stderr, "ICMP Packet Received ") ; fflush(stdout) ;
                /* Receive the Packet */
                    char PACKET[MAX_SIZE];
                    struct sockaddr_in Packet_Source ;
                    socklen_t Source_Len = sizeof(Packet_Source) ;

                    int Recv_Len = 0 ;
                    if ( (Recv_Len = recvfrom( icmp_socket, PACKET, MAX_SIZE, 0,
                                               (struct sockaddr*)&Packet_Source, &Source_Len ) ) < 0 )
                    {
                        perror( RED "ICMP Packet recvfrom() " WHITE ) ;
                        close(udp_socket) ;
                        close(icmp_socket) ;
                        exit(EXIT_FAILURE) ;
                    }
                    struct timeval RecvTime ; gettimeofday(&RecvTime, NULL) ;

                /* Check IP Header for IPPROTO_ICMP */
                    struct iphdr* IP_HDR = (struct iphdr*)PACKET ;
                    if ( IP_HDR->protocol != IPPROTO_ICMP ){
                        continue ;
                    }
                    int IP_HDRLEN = sizeof(*IP_HDR) ;

                /* Check ICMP Header for Type */
                    struct icmphdr* ICMP_HDR = (struct icmphdr*)(PACKET+IP_HDRLEN);

                    if ( ICMP_HDR->type == 3 )
                    {
                        ValidPacket = 1 ;

                        if ( Packet_Source.sin_addr.s_addr != Destination.sin_addr.s_addr ){
                            fprintf(stderr, RED"%s\n"WHITE, "Destination Source and Target Source Unequal " ) ;
                            close(udp_socket) ;
                            close(icmp_socket) ;
                            exit(EXIT_FAILURE) ;
                        }

                        fprintf(stdout, CYAN "Destination Unreachable\t" WHITE ) ;
                        Print( TTL, &Packet_Source, &SendTime, &RecvTime ) ;
                        Done = 1 ;
                        close(udp_socket) ;
                        close(icmp_socket) ;
                         exit(EXIT_SUCCESS) ;
                    }
                    else if ( ICMP_HDR->type == 11 )
                    {
                        ValidPacket = 1 ;
                        fprintf(stdout, CYAN "Time Exceeded\t" WHITE ) ;
                        Print( TTL, &Packet_Source, &SendTime, &RecvTime ) ;
                        Done = 1 ;
                    }
                    else
                    {
                        ValidPacket = 0 ;
                        fprintf(stdout, RED "Invalid " WHITE ) ;
                    }

                }

            }
        /* Checking if Destination Address Sucessfully Reached*/
            if ( Done == 1 ){
                break ;
            }

        }
        /* Checking if Three Sucessive Timeouts were encountered */
            if ( Done == 0 ){
                Print( TTL, NULL, NULL, NULL ) ;
            }
    }

/* Close and Exit */
    close(udp_socket) ;
    close(icmp_socket) ;
    exit(EXIT_SUCCESS) ;
}