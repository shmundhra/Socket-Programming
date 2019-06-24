#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN "\033[1;36m"
#define WHITE "\033[1;37m" 
#define RESET "\033[0m" 
#define PORT_X 50000
#define MAX_SIZE 149
#define BUFF_SIZE 100
#define COMM_SIZE 79
#define header_size 3

void set ( char *str ) {
    memset(str, '\0' , MAX_SIZE+1 ) ; 
}
int stoi( char *tmp ) {
    int val ; sscanf( tmp , "%d" , &val) ; return val ;  
}

int main()
{   
    short int read_bytes1 , read_bytes2 , net_read_bytes , net_len , len ;
    short int Response , Error_Code; 
    int  i , header , last ;
    int recv_bytes, enable, status, process , FORK , PORT_Y ; 
    int CC_serv_len , SD_serv_len , CD_cli_len ; 
    int C1_sockfd , C2_sockfd , newC2_sockfd , File_Desc; 
    struct sockaddr_in CC_serv_addr, CD_cli_addr, SD_serv_addr ; 
    char Server_IP[MAX_SIZE+1], net_len_bit[MAX_SIZE+1] ; 
    char buffer[MAX_SIZE+1] , temp[MAX_SIZE+1] ;    
    char Command[MAX_SIZE+1] , FileName[MAX_SIZE+1] , Block[MAX_SIZE+1] ;
    char *Command_Name , *PathName , *tmp ;
    const char* PORT = "port" ; 
    const char* CD   = "cd"   ; 
    const char* GET  = "get"  ; 
    const char* PUT  = "put"  ; 
    const char* QUIT = "quit" ; 
    const char NULL = '\0' ; 

    if ( (C1_sockfd = socket( AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        perror(RED "Control Socket Creation Failure " WHITE ) ; 
        exit(EXIT_FAILURE) ; 
    }    

    printf( BLUE "Please Enter Server IP Address : " WHITE ) ; 
    fgets(Server_IP , MAX_SIZE , stdin ) ; Server_IP[strlen(Server_IP)-1] = '\0' ; 
    if ( strlen(Server_IP) == 0 ) sprintf( Server_IP , "%s" , "127.0.0.1" ) ; 

    memset(&CC_serv_addr, '\0', sizeof(CC_serv_addr));
    CC_serv_addr.sin_family = AF_INET ; 
    inet_aton( Server_IP , &CC_serv_addr.sin_addr) ;          
    CC_serv_addr.sin_port = htons(PORT_X) ; 
    CC_serv_len = sizeof(CC_serv_addr) ; 

    if ( connect( C1_sockfd , (struct sockaddr *) &CC_serv_addr , CC_serv_len ) < 0 )
    {
        perror(RED "Client unable to connect to Server " WHITE ) ;
        close(C1_sockfd) ; 
        exit(EXIT_FAILURE) ;  
    }

    while(1)
    {
        printf(GREEN ">>>" WHITE ) ;
        set(buffer) ; fgets(buffer , MAX_SIZE , stdin) ; buffer[strlen(buffer)-1] = '\0' ; 
        set(Command); sprintf(Command , "%s" , buffer ) ;   //fprintf( stderr , YELLOW "Command Received is : %s\n" , Command) ;

        Command_Name = strtok(buffer , " ") ;               //fprintf( stderr , YELLOW "Command Name is : %s\n" , Command_Name ) ;
        if ( !Command_Name ) continue ; 

        FORK = 0 ; 
        if ( !strcmp(Command_Name,GET) ) 
        {
            FORK = 1 ; 
            PathName = strtok(NULL , " ") ; 
        }
        if ( !strcmp(Command_Name,PUT) ) 
        {
            FORK = 2 ; 
            PathName = strtok(NULL , " ") ; 
        }

        if ( !FORK ) // port , cd , quit 
        {
            if ( send ( C1_sockfd , Command , strlen(Command)+1 , 0 ) < 0 ) 
            {   
                fprintf( stderr , RED "Server Closed Connection - " WHITE "Closing Connections and Quitting...." ) ; 
                close(C1_sockfd) ; 
                exit(EXIT_FAILURE) ;
            }
            if ( recv ( C1_sockfd , &Response , sizeof(Response) , 0 ) < 0 )
            {
                fprintf( stderr , RED "Server Closed Connection - " WHITE "Closing Connections and Quitting...." ) ; 
                kill(process , SIGKILL ) ; 
                close(C1_sockfd) ; 
                exit(EXIT_FAILURE) ;
            }

            Error_Code = ntohs( Response ) ;                    //fprintf( stderr , YELLOW "Error Code is : %d\n" , Error_Code) ; 
            
            switch(Error_Code) 
            {               
                case 200 :  if ( !strcmp(Command_Name , PORT ) ){
                                sscanf( strtok(NULL , " ") , "%d" , &PORT_Y ) ;                               
                                fprintf(stderr, CYAN "%hi : %s\n" WHITE , 200, "Port Number Sent to Server Successfully");                              
                                break ; 
                            }
                            if ( !strcmp(Command_Name , CD ) ){
                                fprintf(stderr, CYAN "%hi : %s\n" WHITE , 200, "Directory Changed Successfully");                               
                                break ; 
                            }
                             
                case 421 :  fprintf(stderr,CYAN "%hi : %s\n" WHITE , 421, "Closing Connections and Quitting...." );
                            close(C1_sockfd) ; 
                            exit(EXIT_SUCCESS) ;    

                case 501 :  fprintf(stderr, RED "%hi : %s\n" WHITE , 501, "Invalid Arguments, Please Try Again");                             
                            break ;          

                case 502 :  fprintf(stderr, RED "%hi : %s\n" WHITE , 502, "Unrecognisable Command not implemented");                                
                            break ; 

                case 503 :  fprintf(stderr, RED "%hi : %s\n" WHITE , 503, "Bad sequence of commands - First Command should be \'PORT ARG\'" );
                            close(C1_sockfd) ; 
                            exit(EXIT_FAILURE) ;

                case 550 :  fprintf(stderr, RED "%hi : %s\n" WHITE , 550, "FATAL ERROR - Incorrect Port Number!" );
                            close(C1_sockfd) ; 
                            exit(EXIT_FAILURE) ;    

                default :   fprintf(stderr, RED "%s" WHITE "%s\n"  , "Some Error Occured on Server Side - " , "Closing Connections and Quitting...." );
                            close(C1_sockfd) ; 
                            exit(EXIT_FAILURE) ;
            }
            continue ; 
        }
        // get , put 
        if ( (process = fork() ) < 0 )
        {
            perror(RED "Forking Error " WHITE ) ; 
            close(C1_sockfd) ;
            exit(EXIT_FAILURE) ; 
        }
        if ( process > 0 ) //Client Control Process
        {
            if ( send ( C1_sockfd , Command , strlen(Command)+1 , 0 ) < 0 )
            {   
                fprintf( stderr , RED "Server Closed Connection - " WHITE "Closing Connections and Quitting...." ) ; 
                kill(process , SIGKILL ) ; 
                close(C1_sockfd) ; 
                exit(EXIT_FAILURE) ;
            }
            if ( recv ( C1_sockfd , &Response , sizeof(Response) , 0 ) < 0 ) 
            {
                fprintf( stderr , RED "Server Closed Connection - " WHITE "Closing Connections and Quitting...." ) ; 
                kill(process , SIGKILL ) ; 
                close(C1_sockfd) ; 
                exit(EXIT_FAILURE) ;
            }
            Error_Code = ntohs( Response ) ;            

            switch(Error_Code) 
            {               
                case 250 :  waitpid(process , &status , 0 ) ; 
                            fprintf(stderr, CYAN "%hi : %s\n" WHITE , 250, "File Transfer Successful");
                            continue ;
                    
                case 501 :  fprintf(stderr, RED "%hi : %s\n" WHITE , 501, "Invalid Arguments, Please Try Again"); 
                            kill(process , SIGKILL ) ; 
                            continue ;

                case 550 :  fprintf(stderr, RED "%hi : %s\n" WHITE , 550, "File Transfer Unsuccessful");
                            kill(process , SIGKILL ) ; 
                            continue ;

                default :   fprintf(stderr, RED "%s" WHITE "%s\n"  , "Some Error Occured on Server Side - " , "Closing Connections and Quitting...." );
                            kill(process , SIGKILL ) ;  
                            close(C1_sockfd) ; 
                            exit(EXIT_FAILURE) ;
            }

            waitpid(process , &status ,0 ) ; 

        }
        else    // Client Data Process
        {
            if ( (C2_sockfd = socket( AF_INET, SOCK_STREAM, 0)) < 0 )
            {
                perror(RED "TCP Data Socket Creation Failure " WHITE ) ; 
                exit(EXIT_FAILURE) ; 
            }
            enable = 1;
            if ( setsockopt(C2_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            {
                perror(RED "setsockopt(SO_REUSEADDR) Failed " WHITE );                        
            }


            memset(&CD_cli_addr, 0, sizeof(CD_cli_addr)) ;
            CD_cli_addr.sin_family      = AF_INET ;
            CD_cli_addr.sin_addr.s_addr = INADDR_ANY ;
            CD_cli_addr.sin_port        = htons(PORT_Y) ;          //fprintf(stderr, YELLOW "Port Y  number is : %d\n", PORT_Y );
            CD_cli_len                  = sizeof(CD_cli_addr) ;

            if ( bind ( C2_sockfd , (struct sockaddr *) &CD_cli_addr , CD_cli_len ) < 0 )
            {
                perror(RED "Unable to Bind Data Socket " WHITE ) ; 
                close(C2_sockfd)   ; 
                exit(EXIT_FAILURE) ; 
            }
            listen( C2_sockfd, 5 ) ; 
            
            SD_serv_len = sizeof(SD_serv_addr) ;        
            if ( (newC2_sockfd = accept( C2_sockfd, (struct sockaddr *) &SD_serv_addr, &SD_serv_len ) ) < 0 )
            {
                perror(RED "Data Process Unable to Accept Incoming Connection " WHITE ) ; 
                close(C2_sockfd)   ;
                exit(EXIT_FAILURE) ; 
            }            
            set(FileName) ; 

            switch(FORK)
            {
                case 1 :    tmp = strtok(PathName,"/") ;
                            sprintf(FileName , "%s" , tmp ) ;
                            while( (tmp = strtok(NULL , "/")) ) {
                                sprintf(FileName , "%s" , tmp ) ;
                            }
                            usleep(100000) ; 
                            if( (File_Desc = creat( FileName , 0666 )) < 0 ) 
                            {
                                fprintf(stderr, RED "%s : " WHITE , FileName ); perror("") ; 
                                close(newC2_sockfd) ; 
                                close(C2_sockfd) ; 
                                exit(EXIT_FAILURE) ; 
                            }
                            
                            header = len = last = 0 ;                           
                            while(1)  
                            {   
                                set(buffer) ; 
                                if ( ( recv_bytes = recv(newC2_sockfd , buffer , MAX_SIZE , 0 )) < 0 )
                                { 
                                    perror(RED "get Failed " WHITE) ; 
                                    close(newC2_sockfd) ; close(C2_sockfd) ; 
                                    close(File_Desc) ; 
                                    exit(EXIT_FAILURE) ;
                                }
                                if ( recv_bytes == 0 )
                                {   
                                    write(File_Desc , &NULL , 1 ) ; 
                                    close(File_Desc) ;
                                    close(newC2_sockfd) ; close(C2_sockfd) ;                                     
                                    exit(EXIT_SUCCESS) ; 
                                }
                                                                            //fprintf(stderr, MAGENTA "Recv -> %d\n", recv_bytes ); 
                                                                            //write( 2 , buffer , recv_bytes ) ; 

                                for ( i = 0 ; i < recv_bytes ; i++ )
                                {   
                                                                            //fprintf( stderr, BLUE );  
                                                                            //write( 2 , buffer+i , 1 ) ; 
                                    if ( len == 0 )
                                    {
                                        if ( last == 1 && header == 0 ){
                                            write(File_Desc , &NULL , 1 ) ; 
                                            //close(File_Desc) ; 
                                            break ; 
                                        }
                                        switch(header)
                                        {
                                            case 0 :    if ( buffer[i] == 'L' ) last = 1 ; 
                                                        break ; 
                                            case 1 :    net_len_bit[0] = buffer[i] ; 
                                                        break ; 
                                            case 2 :    net_len_bit[1] = buffer[i] ; 
                                                        memcpy( &net_len , net_len_bit , 2 ) ; 
                                                        len = ntohs(net_len) ; //fprintf(stderr, MAGENTA "Len -> %d\n", len ); 
                                                        break ; 
                                            default :   
                                                        break ; 
                                        }
                                        header = ( header + 1 ) % 3 ; 
                                    }
                                    else
                                    {   
                                        write( File_Desc , buffer+i , 1 ) ; 
                                        len-- ; 
                                    }
                                }                             

                            }                           
                            break ; 

                case 2 :    sprintf(FileName , "%s" , PathName ) ;  
                            if( (File_Desc = open( FileName , O_RDONLY)) < 0 )  
                            {
                                fprintf(stderr, RED "\'%s\' : " WHITE , FileName ); perror("") ; 
                                close(newC2_sockfd) ; 
                                close(C2_sockfd) ; 
                                exit(EXIT_FAILURE) ; 
                            }    
                            
                            set(buffer) ; 
                            if ( (read_bytes1=read(File_Desc , buffer , BUFF_SIZE )) < 0 )
                            {
                                perror(RED "Read Failure " WHITE ) ; 
                                close(File_Desc) ; 
                                close(newC2_sockfd) ; close(C2_sockfd) ; 
                                exit(EXIT_FAILURE) ; 
                            } 
                            if ( read_bytes1 == 0 )
                            {   
                                set(Block) ; 
                                sprintf( Block , "%c%c%c" , 'L' , 0 , 0 ) ; 
                                send( newC2_sockfd , Block , header_size + read_bytes1 + 1 , 0 ) ; 
                                close(File_Desc) ; 
                                close(newC2_sockfd) ; close(C2_sockfd) ; 
                                exit(EXIT_SUCCESS) ; 
                            }
                            while(1)
                            {   
                                set(Block) ; 
                                net_read_bytes = htons(read_bytes1) ;    
                                Block[0] = 'K' ; 
                                memcpy( Block+1 , &net_read_bytes , 2 ) ; 
                                memcpy( Block+3 , buffer , read_bytes1 ) ; 
                                
                                set(buffer) ; 
                                if ( (read_bytes2=read(File_Desc , buffer , BUFF_SIZE )) < 0 ) 
                                {
                                    perror(RED "Read Failure " WHITE ) ; 
                                    close(File_Desc) ; 
                                    close(newC2_sockfd) ; 
                                    close(C2_sockfd) ; 
                                    exit(EXIT_FAILURE) ; 
                                }
                                if ( read_bytes2 == 0 )
                                {
                                    Block[0] = 'L' ; 
                                    send( newC2_sockfd , Block , header_size + read_bytes1 + 1 , 0 ) ; 
                                    close(File_Desc) ; 
                                    close(newC2_sockfd) ; close(C2_sockfd) ; 
                                    exit(EXIT_SUCCESS) ; 
                                }
                                send( newC2_sockfd , Block , header_size + read_bytes1 , 0 ) ;
                                read_bytes1 = read_bytes2 ; 
                            }
                            break ; 

                default :   break ; 
            }

        }
    }
}