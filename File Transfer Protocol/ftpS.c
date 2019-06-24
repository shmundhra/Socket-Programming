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

#include <netdb.h> // defines the hostent structure 
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

unsigned short PORT_Y ; 
const char* PORT = "port" ; 
const char* CD   = "cd"   ; 
const char* GET  = "get"  ; 
const char* PUT  = "put"  ; 
const char* QUIT = "quit" ;
const char NULL = '\0' ; 

void set ( char *str ) {
    memset(str, '\0' , MAX_SIZE+1 ) ; 
}

int parse( char *Command_Name ){
         if ( !strcmp( Command_Name , PORT ) ) return 1 ; 
    else if ( !strcmp( Command_Name , CD   ) ) return 2 ; 
    else if ( !strcmp( Command_Name , GET  ) ) return 3 ; 
    else if ( !strcmp( Command_Name , PUT  ) ) return 4 ;
    else if ( !strcmp( Command_Name , QUIT ) ) return 5 ;
    else                                       return 0 ; 
}

short int parse_first ( char *Command )
{   
    int code , Port_No  ; 
    char *Command_Name , *Argument ; 

    Command_Name = strtok(Command , " ") ; 
    code = parse(Command_Name) ; 
    if ( code == 0 )                                            // Unrecognisable Command 
                    return (short int)502 ;   
    if ( code == 5 )                                            // Quit Command 
                    return (short int)421 ;       
    if ( code != 1 )                                            // Non 'port arg' command 
                    return (short int)503 ; 
    
    Argument = strtok(NULL, " ") ; 
    if ( !Argument || strtok(NULL, " ") )                       // No Argument or More than one Argument 
                    return (short int)501 ; 
    
    if ( ! sscanf( Argument , "%d" , &Port_No ) )               // Port Number in String Format is not numeric
                    return (short int)501 ; 
    
    if ( !(1024 <= Port_No && Port_No < 65536) )                // Invalid Port Number 
                    return (short int)550 ; 
    
    PORT_Y = (short)Port_No ;                          // Store Valid Port Number in Global Variable 
                    return (short int)200 ;            // Return 'ALL GOOD'         
}

int main () 
{   
    short int read_bytes1 , read_bytes2 , net_read_bytes , net_len , len ; 
    short int Error_Code, Response ; 
    int i , header, last, recv_bytes, total, code, stop, next, process, status, enable ;     
    int S1_sockfd, newS1_sockfd, S2_sockfd, File_Desc ; 
    int SC_serv_len, CC_cli_len, CD_cli_len;
    char net_len_bit[MAX_SIZE+1] ; 
    char buffer[MAX_SIZE+1] , buffer1[MAX_SIZE+1] ; 
    char Command[MAX_SIZE+1] , FileName[MAX_SIZE+1] ; 
    char Header[MAX_SIZE+1] , Block[MAX_SIZE+1] ; 
    char *Command_Name, *PathName, *DirName , *Argument , *tmp; 
    struct sockaddr_in SC_serv_addr , CC_cli_addr , CD_cli_addr ; 

    if ( (S1_sockfd = socket( AF_INET, SOCK_STREAM, 0)) < 0 )
    {   
        perror( RED "Control Socket Creation Failure " WHITE ) ; 
        exit(EXIT_FAILURE) ; 
    }
    enable = 1;
    if ( setsockopt(S1_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        perror( RED "setsockopt(SO_REUSEADDR) Failed " WHITE); 
    }

    memset(&SC_serv_addr, 0, sizeof(SC_serv_addr));
    SC_serv_addr.sin_family      = AF_INET ; 
    SC_serv_addr.sin_addr.s_addr = INADDR_ANY ; 
    SC_serv_addr.sin_port        = htons(PORT_X) ; 
    SC_serv_len                  = sizeof(SC_serv_addr) ; 

    if ( bind ( S1_sockfd , (struct sockaddr *) &SC_serv_addr , SC_serv_len ) < 0 )
    {
        perror( RED "Unable to Bind Control Socket to Local Address " WHITE ) ; 
        //close(S1_sockfd)   ; 
        exit(EXIT_FAILURE) ; 
    }
    listen ( S1_sockfd, 10 ) ;  

    while(1)
    {   
        printf( GREEN "\nServer Running.... \n\n" WHITE ) ; 

        CC_cli_len = sizeof(CC_cli_addr) ;        
        if ( (newS1_sockfd = accept( S1_sockfd, (struct sockaddr *) &CC_cli_addr, &CC_cli_len ) ) < 0 )
        {
            perror( RED "Server Control Unable to Accept Incoming Connection " WHITE ) ; 
            continue ;
        }
       
        while(1)                             // This is waiting for the completion of the first command sequence. 
        {            
            if ( (recv_bytes = recv( newS1_sockfd , buffer , MAX_SIZE , 0 )) < 0 ) 
            {
                perror(RED "Command Receive Error " WHITE) ; 
                continue ;
            }
            if ( recv_bytes == 0 )
            {
                fprintf(stderr, CYAN "%hi : %s\n" WHITE, 200, "SUCCESS - Closing Connection.." );
                stop = 1 ; 
                break ; 
            }
            sprintf( Command , "%s" , buffer ) ;                        //fprintf( stderr , YELLOW "Command Received is : %s\n" , Command) ; 
            
            Error_Code = parse_first(buffer) ;                          //fprintf( stderr , YELLOW "Error Code is : %d\n" , Error_Code) ; 
            Response = htons(Error_Code) ; 
            send( newS1_sockfd , &Response , sizeof(Response) , 0 ) ;    

            if ( Error_Code == 200 )
            {   
                fprintf(stderr, CYAN "%hi : %s\n" WHITE, Error_Code, "Command Executed Successfully" );
                stop = 0 ; 
                break ; 
            }
            if ( Error_Code == 421 )
            {
                fprintf(stderr, CYAN "%hi : %s\n" WHITE, Error_Code, "SUCCESS - Closing Connection.." );
                stop = 1 ; 
                break ; 
            }
            if ( Error_Code == 501 || Error_Code == 502 )
            {
                fprintf(stderr, RED "%hi : %s%s%s\n" , 501 , "Invalid Command - " , WHITE , "Waiting for next command." );
                continue ;  
            }
            if ( Error_Code == 503 || Error_Code == 550 )
            {
                fprintf(stderr, RED "%hi : %s\n" WHITE, Error_Code, "FAILURE - Closing Connection." ); 
                stop = 1 ;
                break ; 
            }    
        }       
        if (stop)
        {   
            close(newS1_sockfd) ; 
            continue ;
        }

        while(1)                            // For the commands post the first command 
        {
            if ( (recv_bytes = recv( newS1_sockfd , buffer , MAX_SIZE , 0 )) < 0 ) 
            {
                perror(RED "Command Receive Error " WHITE) ; 
                continue ;
            }
            if ( recv_bytes == 0 )
            {
                fprintf(stderr, CYAN "%hi : %s\n" WHITE, 200, "SUCCESS - Closing Connection.." );
                stop = 1 ; 
                break ;  
            } 
            sprintf( Command , "%s" , buffer ) ;        //fprintf( stderr , YELLOW "Command Received is : %s\n" , Command) ;  

            Command_Name = strtok(buffer , " ") ;       //fprintf( stderr , YELLOW "Command Name is : %s\n" , Command_Name ) ;
            code = parse(Command_Name) ;     

            if ( code == 0 || code == 1 )
            {
                Error_Code = 502 ; 
                if(code == 0) fprintf(stderr, RED "%hi : %s%s%s\n" , 502 , "Invalid Command - " , WHITE , "Waiting for next command." );
                if(code == 1) fprintf(stderr, RED "%hi : %s%s%s\n" , 502 , "Port Command Received Again - " , WHITE , "Waiting for next command." );
                Response = htons(Error_Code) ; 
                send( newS1_sockfd , &Response , sizeof(Response) , 0 ) ;          
                continue ;  
            }
            if ( code == 2 )
            {
                DirName = strtok(NULL , " " ) ; 
                if ( !DirName || strtok(NULL," ") ) 
                {
                    Error_Code = 501 ;
                    fprintf(stderr, RED "%hi : %s%s%s\n" , 501 , "Invalid Argument - " , WHITE , "Waiting for next command." );
                    Response = htons(Error_Code) ; 
                    send( newS1_sockfd , &Response , sizeof(Response) , 0 ) ;                   
                }
                else
                {   
                    //sprintf(Argument , "/%s" , DirName ) ; 
                    if ( chdir(DirName) < 0 )
                    {
                        perror( RED "501 : Could not change directory " WHITE ) ; 
                        Error_Code = 501 ;                         
                    }
                    else
                    {   
                        fprintf(stderr, CYAN "%hi : %s\n" WHITE, Error_Code, "Directory Change Successful." );
                        Error_Code = 200 ; 
                    }
                    Response = htons(Error_Code) ; 
                    send( newS1_sockfd , &Response , sizeof(Response) , 0 ) ; 
                }
                continue ;
            }
            if ( code == 3 )
            {
                PathName = strtok(NULL , " ") ; 
                if ( !PathName || strtok(NULL , " ") )
                {
                    Error_Code = 501 ;
                    fprintf(stderr, RED "%hi : %s%s%s\n" , 501 , "Invalid Argument - " , WHITE , "Waiting for next command." );
                    Response = htons(Error_Code) ; 
                    send( newS1_sockfd , &Response , sizeof(Response) , 0 ) ;  
                    continue ;                                   
                }
                
                if ( (File_Desc = open( PathName , O_RDONLY )) < 0 )
                {   
                    Error_Code = 550 ; 
                    fprintf(stderr, RED "%hi : \'%s\' " WHITE, Error_Code, PathName ); perror("") ; 
                    Response = htons(Error_Code) ; 
                    send( newS1_sockfd , &Response , sizeof(Response) , 0 ) ;  
                    continue ;                                                  
                }
                
                if ( (process = fork()) < 0 )
                {
                    perror( RED "Forking Error " WHITE ) ; 
                    Error_Code = 550 ; 
                    Response = htons(Error_Code) ; 
                    send( newS1_sockfd , &Response , sizeof(Response) , 0 ) ;    
                    continue ;    
                }
                if ( process == 0 )
                {
                    if ( (S2_sockfd = socket( AF_INET, SOCK_STREAM, 0)) < 0 ) 
                    {
                        perror( RED "Data Socket Creation Failure " WHITE) ;                             
                        exit(EXIT_FAILURE) ; 
                    }
                    
                    memset( &CD_cli_addr , '\0' , sizeof(CD_cli_addr) ) ; 
                    memcpy( &CD_cli_addr , &CC_cli_addr , sizeof(CC_cli_addr) ) ;                    
                    CD_cli_addr.sin_port    = htons(PORT_Y)         ;
                    CD_cli_len = sizeof(CD_cli_addr) ; 

                    usleep(100000) ; 
                    if ( connect( S2_sockfd , (struct sockaddr *) &CD_cli_addr , CD_cli_len ) < 0 )
                    {
                        perror( RED "Unable to Connect to Client Data Socket " WHITE) ; 
                        close(S2_sockfd)  ; 
                        exit(EXIT_FAILURE) ; 
                    }
                    
                    set(buffer) ;                     
                    if ( (read_bytes1=read(File_Desc , buffer , BUFF_SIZE )) < 0 )
                    {
                        perror( RED "Read Failure " WHITE ) ; 
                        close(File_Desc) ; 
                        close(S2_sockfd) ; 
                        exit(EXIT_FAILURE) ; 
                    } 
                    if ( read_bytes1 == 0 )
                    {   
                        set(Block) ; 
                        sprintf( Block , "%c%c%c" , 'L' , 0 , 0 ) ; //fprintf(stderr, BLUE ) ; write( 2 , buffer , 3 + strlen(buffer+3) ) ; 
                        send( S2_sockfd , Block , header_size + read_bytes1 + 1 , 0 ) ; 
                        close(File_Desc) ; 
                        close(S2_sockfd) ; 
                        exit(EXIT_SUCCESS) ; 
                    }
                                                                    //fprintf(stderr, MAGENTA "Buffer is- %s\n", buffer );
                    
                    while(1)
                    {   
                        set(Block) ; 
                        net_read_bytes = htons(read_bytes1) ; 
                        Block[0] = 'K' ; 
                        memcpy( Block+1 , &net_read_bytes , 2 ) ; 
                        memcpy( Block+3 , buffer , read_bytes1 ) ;
                        
                        set(buffer) ; 
                        if ( (read_bytes2 =read(File_Desc , buffer , BUFF_SIZE )) < 0 ) 
                        {
                            perror(RED "Read Failure " WHITE ) ; 
                            close(File_Desc) ; 
                            close(S2_sockfd) ; 
                            exit(EXIT_FAILURE) ; 
                        }                        
                        if ( read_bytes2 == 0 )
                        {   
                                                            //fprintf(stderr, BLUE ) ; write( 2 , Block , 3 + strlen(Block+3) ) ;                             
                            Block[0] = 'L' ; 
                                                            //fprintf(stderr, BLUE ) ; write( 2 , Block , 3 + strlen(Block+3) ) ; 
                            send( S2_sockfd , Block , header_size + read_bytes1 + 1, 0 ) ; 
                            close(File_Desc) ; 
                            close(S2_sockfd) ; 
                            exit(EXIT_SUCCESS) ; 
                        }
                                                            //fprintf(stderr, BLUE ) ; write( 2 , Block , 3 + strlen(Block+3) ) ; 
                                                            //fprintf(stderr, MAGENTA "Buffer is- %s\n", buffer );                    
                        send( S2_sockfd , Block , header_size + read_bytes1 , 0 ) ; 
                        read_bytes1 = read_bytes2 ; 
                    }
                }
                else
                {
                    waitpid ( process , &status , 0 ) ;
                    if ( WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS )
                    {
                        Error_Code = 250 ; 
                        fprintf(stderr, CYAN "%hi : %s\n" WHITE , 250, "File Transfer Successful");
                    }                     
                    else
                    {
                        Error_Code = 550 ; 
                        fprintf(stderr, RED "%hi : %s\n" WHITE , 550, "File Transfer Unsuccessful");
                    }
                    Response = htons(Error_Code) ; 
                    send( newS1_sockfd , &Response , sizeof(Response) , 0 ) ;    
                    close(File_Desc) ; 
                    continue ; 
                }
            }
            if ( code == 4 )
            {   
                PathName = strtok(NULL , " ") ; 
                if ( !PathName || strtok(NULL , " ") )
                {
                    Error_Code = 501 ;
                    fprintf(stderr, RED "%hi : %s%s%s\n" , 501 , "Invalid Argument - " , WHITE , "Waiting for next command." );
                    Response = htons(Error_Code) ; 
                    send( newS1_sockfd , &Response , sizeof(Response) , 0 ) ;  
                    continue ;                                 
                }

                tmp = strtok(PathName,"/") ; 
                set(FileName); sprintf(FileName , "%s" , tmp ) ; 
                if ( (tmp = strtok(NULL,"/")) ) 
                {
                    Error_Code = 501 ;
                    fprintf(stderr, RED "%hi : %s%s%s\n" , 501 , "Invalid Argument, no Absolute Paths Allowed - " , WHITE , "Waiting for next command." );
                    Response = htons(Error_Code) ; 
                    send( newS1_sockfd , &Response , sizeof(Response) , 0 ) ; 
                    continue ; 
                }    

                total = 0 ; 
                if ( (process = fork()) < 0 )
                {
                    perror(RED "Forking Error " WHITE ) ; 
                    Error_Code = 550 ; 
                    Response = htons(Error_Code) ; 
                    send( newS1_sockfd , &Response , sizeof(Response) , 0 ) ;    
                    continue ;    
                }
                if ( process == 0 )
                {
                    if ( (S2_sockfd = socket( AF_INET, SOCK_STREAM, 0)) < 0 ) 
                    {
                        perror(RED "Data Socket Creation Failure " WHITE ) ;                             
                        exit(EXIT_FAILURE) ; 
                    }

                    memset( &CD_cli_addr , '\0' , sizeof(CD_cli_addr) ) ; 
                    memcpy( &CD_cli_addr , &CC_cli_addr , sizeof(CC_cli_addr) ) ;                    
                    CD_cli_addr.sin_port    = htons(PORT_Y)         ;
                    CD_cli_len = sizeof(CD_cli_addr) ; 

                    usleep(100000) ; 
                    if ( connect( S2_sockfd , (struct sockaddr *) &CD_cli_addr , CD_cli_len ) < 0 )
                    {
                        perror(RED "Unable to Connect to Client Data Socket " WHITE ) ; 
                        close(S2_sockfd) ; 
                        exit(EXIT_FAILURE) ; 
                    }               
                    
                    header = len = last = 0 ;                           
                    while(1)  
                    {	
                    	set(buffer) ; 
                        if ( ( recv_bytes = recv(S2_sockfd , buffer , MAX_SIZE, 0 ) ) < 0 )
                        { 
                            perror(RED "get Failed ") ; 
                            close(S2_sockfd) ;                             
                            close(File_Desc) ; 
                            exit(EXIT_FAILURE) ;
                        }
                        if ( recv_bytes == 0 )
                        {   
                            if ( total == 0 )
                            {
                                close(S2_sockfd) ; 
                                exit(EXIT_FAILURE) ; 
                            }            
                            write(File_Desc , &NULL , 1 ) ; 
                            close(File_Desc) ;            
                            close(S2_sockfd) ;                                     
                            exit(EXIT_SUCCESS) ; 
                        }
                        if ( total == 0 )
                        {
                            if ( (File_Desc = creat( FileName , 0666 ) ) < 0 )   
                            {
                                perror(RED "Unable to Create Copy File " WHITE ) ; 
                                close(S2_sockfd) ; 
                                exit(EXIT_FAILURE) ; 
                            }
                        }
                        total += recv_bytes ; 
                        for ( i = 0 ; i < recv_bytes ; i++ )
                        {   
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
                                                len = ntohs(net_len) ;      //fprintf(stderr, MAGENTA "Len -> %d\n", len ); 
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
                }
                else
                {
                    waitpid ( process , &status , 0 ) ;
                    if ( WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS )
                    {
                        Error_Code = 250 ; 
                        fprintf(stderr, CYAN "%hi : %s\n" WHITE , 250, "File Transfer Successful");
                    }                     
                    else
                    {
                        Error_Code = 550 ; 
                        fprintf(stderr, RED "%hi : %s\n" WHITE , 550, "File Transfer Unsuccessful");
                    }
                    Response = htons(Error_Code) ; 
                    send( newS1_sockfd , &Response , sizeof(Response) , 0 ) ;    
                    close(File_Desc) ; 
                    continue ; 
                }                
            }
            if ( code == 5 )
            {
                Error_Code = 421 ; 
                fprintf(stderr, CYAN "%hi : %s\n" WHITE , Error_Code , "SUCCESS - Closing Connection.." );
                Response = htons(Error_Code) ; 
                send( newS1_sockfd , &Response , sizeof(Response) , 0 ) ;
                close( newS1_sockfd ) ; 
                break ; 
            }

            
        }
    }
}