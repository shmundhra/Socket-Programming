#include "rsocket.h"

/*** Structure for every Node of the Receive Buffer Circular Queue ***/
typedef struct __Msg_Info {
	char *Message;
	int Msg_Len;
	struct sockaddr* Source ;
	socklen_t SAddr_Len ;  	
} Msg_Info ;

/*** Wrapper around the different componenets of the Receive Buffer ***/
typedef struct __buffer {
	Msg_Info* buffer ; 
	int start ; 
	int end ;
	int size ;
	int max_size ; 
	pthread_mutex_t Lock ;
	pthread_cond_t Empty ;  
	pthread_cond_t Full ;  	 
} _buffer ; 
_buffer recv_Buffer ; 

/*** Structure for every Entry of the UnACK Message Table ***/
typedef struct __Ack_Data {
	char *Packet;
	int Pack_Len;
	int Msg_flags ; 
	const struct sockaddr* Destination;
	socklen_t DAddr_Len ;  
	struct timeval Msg_time ; 
} ACK_Data ; 

/*** Wrapper around the different componenets of the UnACK Table ***/
typedef struct __UnACK {
	ACK_Data** Table ; 
	int Num ; 
	pthread_mutex_t Lock ;
	pthread_cond_t Exists ;  	 
} _UnACK ; 
_UnACK UnACK ; 

/*** Boolean Hash Array to maintain the Received-Message IDs ***/
short int* recv_ID = NULL;   

short int curr_ID = -1  ; 	// Global UNIQUE_ID Counter
int NumOf_Transm  =  0  ; 	// Global Counter for Number of Packets Received
int NumOf_Packet  =  0  ;	// Global Counter for Number of Unique Packets 
char APP_MSGTYPE  = 'P' ;	// Type distinguisher
char ACK_MSGTYPE  = 'C' ;	// Type distinguisher 

int __i ; 
pthread_t x_id ;

//Definition of Accessors and Getters for the Receive Buffer
void printRecvBuffer() ;
void Recv_Buffer_Enque( Msg_Info ) ;
Msg_Info* Recv_Buffer_Deque( ) ;
char* AppMessagePacket(int*, char*, int ) ;

//Definition of Accessors and Getters for the UnACK Table
void printUnACK() ; 
void UnACK_Table_Attach( int, ACK_Data ) ; 
void UnACK_Table_Detach( int ) ; 
char* ACKMessagePacket(int , int* ) ;

//Assignment of Result to the Value-Result Arguments 
void AssignValues( Msg_Info*, char*, struct sockaddr*, socklen_t* ) ; 

//Socket Constructor and Destroyer Functions
int  CreateSocket(int, int, int);
void CloseSocket( int ) ; 

//Memory Allocation and DeAllocation Functions
int AllocateMemory() ; 
void ReleaseMemory() ;
void ReleaseLockCond() ; 

//Thread Constructor and Destroyer Functions
int  CreateThread(int) ; 
void KillThread_X() ; 

int r_socket( int domain, int type, int protocol )
{	
	/*** Creation of MRP type Socket ***/
	int socket_FD = CreateSocket( domain, type, protocol ) ; 
	if ( socket_FD < 0 ) return -1 ; 

	/*** Dynamic Allocation of Memory to Global Pointers ***/ 
	if ( AllocateMemory() < 0 ) return -1 ; 
	
	/*** Creation of Thread X  ***/ 
	if ( CreateThread(socket_FD) < 0 ) return -1 ; 
	
	return socket_FD ; 
}

int r_bind( int socket_fd, const struct sockaddr *address, socklen_t address_len )
{	
	if ( bind(socket_fd, address, address_len) < 0 ){
		perror("MRP Socket r_bind() Failed ");
		return -1 ; 
	}
	return 0 ;
}

int r_sendto( int socket_fd, const void* message, size_t length, int flags, 
			  const struct sockaddr *dest_address, socklen_t dest_len )
{	
	/** Generate Valid Unique MessageId **/	
	curr_ID++ ; 												// Incrementing from previous value (Unique)
	if ( UnACK.Table[curr_ID] != NULL ){ 						// Already occupied curr_ID in UnACK Table (Invalid) 
		fprintf(stderr, "Message_ID Already used up !!\n" ); 
		return -1 ; 
	}

	/** Generate Application Message Packet **/	
	int Packet_Len = 0 ;	
	char* Packet = AppMessagePacket(&Packet_Len, (char*)message, length ) ; 
	
	/** Sending the created Packet  **/	
	int sent_Packet_Len = -1; 
	if ( (sent_Packet_Len = sendto( socket_fd, Packet, Packet_Len, flags, dest_address, dest_len)) != Packet_Len ){
		perror( "MRP Socket r_sendto() Failed " ) ;	
		return -1 ; 
	}
	
	NumOf_Packet++ ; 
	fprintf(stderr , YELLOW "Unique Packet Count : " CYAN "%2d\tPacket Sent: " , NumOf_Packet ) ; 
	fprintf(stderr, MAGENTA "%c%d%s\n" WHITE , Packet[0], curr_ID, Packet+3 ) ; 

	/**** Successfully Sent, now storing in UnACK Table  ****/

	/** Creating ACK_Data for the Sent Message **/	
	struct timeval curr_time ; 
	gettimeofday( &curr_time, NULL ) ;  	  					// Setting the current time for Msg_Info
	ACK_Data new_msg = { Packet, Packet_Len, flags, dest_address, dest_len, curr_time } ; 	 

	/** Attaching it to UnACK.Table[id] **/
	pthread_mutex_lock(&UnACK.Lock);
		UnACK_Table_Attach(curr_ID, new_msg ) ;
	pthread_mutex_unlock(&UnACK.Lock);

	return length ;
}

int r_recvfrom(int socket_fd, void* message, size_t length, int flags, 
			   struct sockaddr *source_address, socklen_t *source_len )
{	
	pthread_mutex_lock(&recv_Buffer.Lock);
		// Blocking :: Wait till Buffer is Empty
		while(!recv_Buffer.size){	
			pthread_cond_wait(&recv_Buffer.Empty, &recv_Buffer.Lock);
		}

		/*** Deque the First element from the FIFO Buffer ***/
		Msg_Info* msg_Info = Recv_Buffer_Deque() ; 
		pthread_cond_signal(&recv_Buffer.Full); 			// Signal :: Buffer may no be Full anymore ***/

		/*** Assigning Values to the Value-Result Arguments  ***/
		AssignValues(msg_Info, (char*)message, source_address, source_len ) ; 

	pthread_mutex_unlock(&recv_Buffer.Lock);	

	return (msg_Info->Msg_Len) ; 		 					// Returns the Length of the Message Received  
	
}

int r_close ( int socket_fd )
{		
	signal(SIGUSR1, Signal_Handler) ; 
	fprintf(stderr, RED"%s\n"WHITE, "Waiting for ACKs..." );

	pthread_mutex_lock(&UnACK.Lock);
		// Blocking :: wait till UnACK Messages Exist
		while(UnACK.Num){					
			pthread_cond_wait(&UnACK.Exists, &UnACK.Lock) ;
		}
		fprintf(stderr, CYAN"%s\n"WHITE, "All ACKs Received." );

		ReleaseMemory() ; 
		KillThread_X() ; 
		CloseSocket(socket_fd);
		
	pthread_mutex_unlock(&UnACK.Lock);	
	ReleaseLockCond() ;
	return 0 ; 
}
	

void* Thread_X ( void* param )
{		
	int socket_FD = *((int*)param) ; 
	fd_set wait_FD ; 
	int nfds = socket_FD + 1 ;
	struct timeval timeout = { T , 0 }  ; 	
	int consec_timeouts = 0 ; 

	while(consec_timeouts<MAX_CONSECUTIVE_TIMEOUTS) 
	{	
		FD_ZERO(&wait_FD);
		FD_SET(socket_FD, &wait_FD);
		int ready_fd ; 

		if ( (ready_fd = select( nfds, &wait_FD, NULL, NULL, &timeout)) < 0){
			perror("select() ") ; 
			pthread_exit(0) ; 
		}	
		int res ; 			
		if ( ready_fd == 0 )   				 	 // Comes out on a Timeout 
		{	
			fprintf(stderr, RED"%s\n"WHITE, "Timeout Occured" );
			if ( (res=HandleRetransmit(socket_FD)) < 0 ) {				
				perror("Retransmit Failed ") ; 
				exit(EXIT_FAILURE) ; 
			}
			if ( res == 0 )	consec_timeouts++ ; 
			else consec_timeouts = 0 ; 
			
			timeout.tv_sec = T ;			  	// Reinitialise the Timeout 
			timeout.tv_usec = 0 ;												
		}	
		else								  	// Comes out on a Receive
		{
			fprintf(stderr, GREEN"%s\n"WHITE, "\nIncoming Receive" );
			if ( (res=HandleReceive(socket_FD)) < 0 ) {
				perror("Receive Failed ") ; 
				exit(EXIT_FAILURE) ; 
			}					     		  	// Continue with same Timeout 
			consec_timeouts = 0 ; 
		}			
	}	
	exit(EXIT_SUCCESS) ;
}

int HandleRetransmit( int socket_fd )			// Handles Retranmissions 
{												// {-1,Num} :: {Error, Num of UnACKed Messages Resent}
	struct timeval curr_time ; 
	gettimeofday(&curr_time,NULL);
	
	int NumOf_ReTransm = 0 ; 
	for ( __i=0 ; __i<MAXSIZE_TABLE; __i++ )  	 	// Check for UnACKed Messag in the UnACK.Table
	{
		if ( UnACK.Table[__i] == NULL ){			// NULL implies this 'id' is not UnACKed
			continue ;
		}
		pthread_mutex_lock(&UnACK.Lock);
			/*** Reaching here means Message with ID "__i" is UnACKed  ***/
			if ( (curr_time.tv_sec - (UnACK.Table[__i]->Msg_time).tv_sec) > T )   // UnACKed and Timed-out !!
			{	
				int sent_Packet_Len = -1 ; 				
				if ( (sent_Packet_Len = sendto( socket_fd, UnACK.Table[__i]->Packet,  UnACK.Table[__i]->Pack_Len, 
											 	UnACK.Table[__i]->Msg_flags, UnACK.Table[__i]->Destination, 
												UnACK.Table[__i]->DAddr_Len) ) != UnACK.Table[__i]->Pack_Len )
				{
					perror( "MRP Socket Retransmission Failed " ) ; 
					return -1 ; 
				}
				gettimeofday( &(UnACK.Table[__i]->Msg_time) , NULL ) ;  			// Updating Time 
				
				fprintf(stderr, BLUE"Retransmitted Packet %d\n"WHITE, __i );
				NumOf_ReTransm++ ; 
			}
		pthread_mutex_unlock(&UnACK.Lock);		
	}
	return NumOf_ReTransm ;
}

int HandleReceive( int socket_fd )
{	
	/*** Receive Packet using recvfrom() ***/	
	char* Packet = (char*)calloc(sizeof(char), MAXSIZE_PACKET ) ; 
	size_t recv_Packet_Len = MAXSIZE_PACKET; 
	struct sockaddr source ;
	socklen_t sAddr_Len = sizeof(source);

	if ( (recv_Packet_Len = recvfrom( socket_fd, Packet, recv_Packet_Len, 0, &source, &sAddr_Len )) < 0 )	{
		perror("Receive Error " );
		return -1 ; 
	}

	/*** Here Packet is of form :: |TYPE|ID|Message|  ***/	
	fprintf(stderr, WHITE "Packet Received is : %c%2d%s\t", Packet[0], *(Packet+TYPE_SIZE), Packet+TYPE_SIZE+ID_SIZE );

	int res ; 	
	if ( *Packet == APP_MSGTYPE )	// Handle 'Application Type' Message 
	{	
		NumOf_Transm++ ; fprintf(stderr, YELLOW "Tranmission Count : " BLUE "%d\n" WHITE , NumOf_Transm) ; 

		if ( dropMessage(P) ) {
			fprintf(stderr, RED"Dropping APP\n"WHITE);
			return 1 ; 
		}
		if( (res = HandleAppMsg(socket_fd, Packet+TYPE_SIZE, recv_Packet_Len-TYPE_SIZE, &source, sAddr_Len)) < 0 ){
			return res ; 
		}
		//printRecvBuffer() ; 
	}
	if ( *Packet == ACK_MSGTYPE )	// Handle 'Acknowledegement Type' Message
	{	
		if ( dropMessage(P) ) {
			fprintf(stderr, RED"Dropping ACK\n"WHITE);
			return 1 ; 
		}
		if( (res = HandleAckMsg(socket_fd, Packet+TYPE_SIZE, recv_Packet_Len-TYPE_SIZE, &source, sAddr_Len)) < 0 ){
			return res ;
		}
		//printUnACK() ;	
	}
}

int HandleAppMsg( int socket_fd, char* Packet, size_t Packet_Len, 	 
				  struct sockaddr* source, socklen_t sAddr_Len )	// Handles Receive of AppMsg
{																	// {-1,1} : {Error, Success}
	/*** Here Packet is of form :: |ID|Message|  ***/
	
	//Extraction of the ID and converting to HBO
	short int nbo_ID = *Packet 		   ;  
	short int ID = nbo_ID 			   ; 
	
	fprintf(stderr, CYAN"App Packet %d%s received.  "WHITE , ID, Packet+ID_SIZE );																				
	
	// Checking if the Message with this ID has not been received yet
	if ( recv_ID[ID] == 1 ) {
		fprintf(stderr, RED"\tRejecting Duplicate" );		 
	}
	if ( recv_ID[ID] == 0 ) 
	{
		recv_ID[ID] = 1 ; 							 // Marked Receive of Packet with MessageID 'ID' 
		
		// Creation of a Buffer Node :: { char*, size_t, sockaddr*, socklen_t }
		Msg_Info msg_Info = { (Packet+ID_SIZE), (Packet_Len-ID_SIZE), source, sAddr_Len };

		pthread_mutex_lock(&recv_Buffer.Lock);
			//Blocking :: Waiting till Buffer is Full 
			while(recv_Buffer.size == recv_Buffer.max_size){
				pthread_cond_wait(&recv_Buffer.Full, &recv_Buffer.Lock );
			}	
			/*** Enqueuing New Message into End of Buffer ***/
			Recv_Buffer_Enque(msg_Info) ; 			
			pthread_cond_signal(&recv_Buffer.Empty); // Signaling Buffer is no Longer Empty
		pthread_mutex_unlock(&recv_Buffer.Lock); 
	}
	
	/*** Creating and Sending out the ACK ***/	

	// Creating the ACK_Packet :: |TYPE|ID|
	int ACKPacket_Len = 0 ; 
	char* ACK_Packet = ACKMessagePacket( nbo_ID, &ACKPacket_Len) ; 

	int sent_Packet_Len = -1 ;  
	if ( (sent_Packet_Len = sendto( socket_fd, ACK_Packet, ACKPacket_Len, 0, source, sAddr_Len)) != ACKPacket_Len ){
		perror( "MRP Socket ACK Sending Failed " ) ; 
		return -1 ; 
	}
	fprintf(stderr, GREEN"\tACK %d Sent \n"WHITE, ID  );

	return 1 ; 
}

int HandleAckMsg( int socket_fd, char* Packet, size_t Packet_Len, 
				  struct sockaddr* source, socklen_t sAddr_Len )
{	
	/*** Here Packet is of form :: |ID|  ***/
		
	// Extraction of the ID and converting to HBO	
	short int nbo_ID = *Packet         ;  	
	short int ID 	 = nbo_ID 		   ; 
	
	fprintf(stderr, CYAN"ACK %d%s received. "WHITE , ID, Packet+ID_SIZE );
	
	// Checking if the 'ID' value is within the bounds 
	if ( ID < 0 || MAXSIZE_TABLE <= ID ){
		fprintf(stderr, RED"Invalid Message-ID : Out of Bounds\n"WHITE );
		return -1 ; 
	}

	/*** UnACK.Table Data Shared Memory Access ***/	
	pthread_mutex_lock(&UnACK.Lock);
		if ( UnACK.Table[ID] == NULL ){
			fprintf(stderr, RED"\tRejecting Duplicate\n" );
			return 1 ;
		}			
		if ( UnACK.Table[ID] != NULL ) 			// Packet ACKed, removed from UnACK.Table 
		{	
			UnACK_Table_Detach( ID ) ; 
			pthread_cond_signal(&UnACK.Exists); // Signaling Outstanding Messages MAYBE 0 
		}  
	pthread_mutex_unlock(&UnACK.Lock);

	fprintf(stderr, GREEN"\tACK %d Processed \n"WHITE, ID  );
	return 0 ; 
}

int dropMessage( float p ){	
	struct timeval seed ; 
	gettimeofday(&seed, NULL);
	srand(seed.tv_usec) ; 
	double uniform = (double)((double)rand()/RAND_MAX) ; 
	if ( uniform <= p )
		return 1 ; 
	else 
		return 0 ; 
}

void Signal_Handler( int Signal_Number ){
	if ( Signal_Number == SIGUSR1 ){
		pthread_exit(NULL) ; 	
	}
	return ;
}

//Definition of Accessors and Getters for the Receive Buffer
void printRecvBuffer(){	
	fprintf(stderr, "#_" );
	int index ; 
	for( __i=0; __i<recv_Buffer.size; __i++ ){
		index = (__i + recv_Buffer.start)%recv_Buffer.max_size ; 
		fprintf(stderr, "%s(%d)_", recv_Buffer.buffer[index].Message, recv_Buffer.buffer[index].Msg_Len ) ; 
	}
	fprintf(stderr, "#\n" );
	return ; 
}
Msg_Info* Recv_Buffer_Deque(){	
	Msg_Info* msg_Info = (Msg_Info*)calloc( sizeof(Msg_Info), 1 ) ;
	*msg_Info = recv_Buffer.buffer[recv_Buffer.start] ;					// Store First AppMsg of buffer
	recv_Buffer.start = (recv_Buffer.start+1) % recv_Buffer.max_size ; 	// Increment Start and Wrap Around
	recv_Buffer.size-- ; 												// Decrement Buffer Count
	return msg_Info ; 													// Return the Msg_Info	
}
void Recv_Buffer_Enque( Msg_Info msg_Info ){
	recv_Buffer.buffer[recv_Buffer.end] = msg_Info ; 			// Add new AppMsg to the END of the buffer
	recv_Buffer.end = (recv_Buffer.end+1)%recv_Buffer.max_size; // Increment End Variable and Wrap Around 
	recv_Buffer.size ++ ;										// Inrement Buffer Count
	return ; 
}
char* AppMessagePacket(int *Packet_Len, char* Message, int Length ){
	*Packet_Len = 0 ;	
	short int nbo_id = curr_ID ; 
	char* Packet = (char*)calloc(sizeof(char) , MAXSIZE_PACKET ) ; 
	memcpy(Packet + *Packet_Len, &APP_MSGTYPE , TYPE_SIZE) ; *Packet_Len += TYPE_SIZE ; //Type
	memcpy(Packet + *Packet_Len, &nbo_id 	  , ID_SIZE  ) ; *Packet_Len += ID_SIZE   ; //ID
	memcpy(Packet + *Packet_Len, Message 	  , Length   ) ; *Packet_Len += Length    ; //Message
	return Packet ; 
}

//Definition of Accessors and Getters for the UnACK Table
void printUnACK(){	
	for ( __i=0; __i<MAXSIZE_TABLE; __i++){
		if ( UnACK.Table[__i] ) fprintf(stderr, "(%s) ", (UnACK.Table[__i]->Packet)+3 );
	}
	fprintf(stderr, "\n");
	return ; 
}
void UnACK_Table_Attach( int id, ACK_Data new_msg ){
	UnACK.Table[id] = (ACK_Data*)malloc( sizeof(ACK_Data) ) ;	// Allocating Memory to new UnACK.Table Entry 		
	*UnACK.Table[id] = new_msg ;  								// UnACK.Table[id] points to value of new_msg  
	UnACK.Num++ ;  												// Increment Count of Outstanding Messages
	return ;
}
void UnACK_Table_Detach( int id ){
	free(UnACK.Table[id]) ; 				// Free Memory Associated at that index
	UnACK.Table[id] = NULL ; 				// Point to NULL 
	UnACK.Num-- ;							// Decrement Outstanding Count 
	return ; 
}
char* ACKMessagePacket(int nbo_ID , int *ACKPacket_Len )
{	
	char* ACK_Packet = (char*)calloc(sizeof(char),MAXSIZE_PACKET) ; 	
	memcpy( ACK_Packet + *ACKPacket_Len, &ACK_MSGTYPE,  TYPE_SIZE ) ; *ACKPacket_Len += TYPE_SIZE ; // Type
	memcpy( ACK_Packet + *ACKPacket_Len, &nbo_ID,		 ID_SIZE  ) ; *ACKPacket_Len += ID_SIZE   ; // ID		
	return ACK_Packet ; 
}

//Assignment of Result to the Value-Result Arguments 
void AssignValues( Msg_Info* msg_Info, char* message, struct sockaddr* source_address, socklen_t* source_len )
{
	char* temp = (char*)message ; 
	for ( __i=0 ; __i<(msg_Info->Msg_Len) ; __i++ ){
		temp[__i] = msg_Info->Message[__i] ;
	} 
	*source_address = *(msg_Info->Source) ; 
	*source_len = (msg_Info->SAddr_Len)   ; 
	return ; 
}

//Socket Constructor and Destroyer Functions
int CreateSocket( int domain, int type, int protocol ){
	int socket_FD ;
	if ( type != SOCK_MRP )	{
		fprintf(stderr, "MRP Socket Creation Failed : Invalid Type Argument\n" ) ;
		return -1 ; 
	}
	if ( (socket_FD = socket(domain, SOCK_DGRAM, protocol)) < 0 ){
		perror("MRP Socket Creation Failed ");
		return -1 ; 
	}
	fprintf(stderr, GREEN"Socket Created\n"WHITE );
	return socket_FD ; 
}
void CloseSocket(int socket_fd){
	close(socket_fd);
	fprintf(stderr, GREEN "Closed the MRP Socket.\n" WHITE );
	return ; 
}

//Memory Allocation and DeAllocation Functions 
int AllocateMemory(){	
	//Received Message Buffer
	recv_Buffer = (_buffer){.buffer=(Msg_Info*)calloc( sizeof(Msg_Info), MAXSIZE_TABLE), 
							.start=0, .end=0, .size=0, .max_size=MAXSIZE_TABLE, .Lock=PTHREAD_MUTEX_INITIALIZER, 
							.Empty=PTHREAD_COND_INITIALIZER, .Full=PTHREAD_COND_INITIALIZER }; 
	if ( recv_Buffer.buffer == NULL ){
		fprintf(stderr, "recv_Buffer.buffer : Memory Allocation Failed\n" ) ;
		return -1 ;   
	}
	//Received Message-ID Table indexed by MessageIDs - All initialised to False (0)
	recv_ID = (short int *)calloc( sizeof(short int) , MAXSIZE_TABLE ) ;
	if( recv_ID == NULL ){
		fprintf(stderr, "recv_ID : Memory Allocation Failed\n" ) ;
		return -1 ; 
	}
	for ( __i=0 ; __i<MAXSIZE_TABLE; __i++ ) recv_ID[__i] = 0 ;				
	
	//UnACK Message Table indexed by MessageIDs - All initialised to NULL
	UnACK = (_UnACK) { .Table=(ACK_Data**)malloc( sizeof(ACK_Data*) * MAXSIZE_TABLE), 
					   .Num=0, .Lock=PTHREAD_MUTEX_INITIALIZER, .Exists=PTHREAD_COND_INITIALIZER } ;
	if ( UnACK.Table == NULL ){
		fprintf(stderr, "UnACK.Table : Memory Allocation Failed\n" ) ;
		return -1 ; 
	}	
	for ( __i=0 ; __i<MAXSIZE_TABLE; __i++ ) UnACK.Table[__i] = NULL ;		
	
	fprintf(stderr, GREEN"Memory Allocated\n"WHITE );
	return 1 ; 	
}
void ReleaseMemory(){
	free(recv_Buffer.buffer);					// Release Receive Buffer Space by discarding any data if present 
	free(UnACK.Table);							// Releases the UnACK.Table Space after ensuring that no UnACK Packets are left
	free(recv_ID);								// Releasing the Received Message-ID Hash Table
	fprintf(stderr, GREEN "Memory Released.\n" WHITE );		
	return ; 	
}
void ReleaseLockCond(){	
	//Destroying Mutex Variables
	pthread_mutex_destroy(&recv_Buffer.Lock) ;  
	pthread_mutex_destroy(&UnACK.Lock);
	//Destroying Condition Variables
	pthread_cond_destroy(&recv_Buffer.Empty);
	pthread_cond_destroy(&recv_Buffer.Full);
	pthread_cond_destroy(&UnACK.Exists);

	fprintf(stderr, GREEN "Locks and Conditions Released.\n" WHITE );	
	return ; 
}

//Thread Constructor and Destroyer Functions 
int CreateThread( int socket_fd ){	
	int* socket_fdp = (int*)calloc(sizeof(int),1) ; 
	*socket_fdp = socket_fd ; 
	if ( pthread_create( &x_id, NULL, Thread_X, socket_fdp ) < 0 ){
		perror("Thread X Creation Failed ") ; 
		return -1 ; 
	}
	fprintf(stderr, GREEN"Thread Created\n"WHITE ); 
	return 1 ; 
}
void KillThread_X(){
	pthread_kill( x_id, SIGUSR1 );				// Sending Signal to Thread X for Safe Termination 
	pthread_join( x_id, NULL 	); 				// Waiting for Thread Termination
	fprintf(stderr, GREEN "Thread X Terminated Safely.\n" WHITE );	
	return ; 
}



