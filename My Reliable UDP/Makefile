CC = gcc
server = user2.c 
client = user1.c
.DEFAULT_GOAL := Menu

Menu	:	
    @echo "Welcome to MRP Protocol Makefile :: Default CC=$(CC) server=$(server) client=$(client) "
    @echo ""
    @echo "CHOOSE APPROPRIATE COMMAND : "
    @echo "Header  [CC=<CompilerVersion>] - Creates Header Executable \"rsocket.o\""
    @echo "Library [CC=<CompilerVersion>] - Creates Static Library \"librsocket.a\""
    @echo "Server  [CC=<CompilerVersion>] [server=<ServerSource.c>] - Creates \"server.o\""
    @echo "Client  [CC=<CompilerVersion>] [client=<ClientSource.c>] - Creates \"client.o\""
    @echo "clean   - Removes All files created in this Makefile and Clears Screen"
    @echo "remove  - Removes All files created in this Makefile"
    @echo ""

Server 		: MRP_Server
    @echo ""
    @echo ">>\"./server.o\" TO START MRP Server"
    @echo ""

Client 		: MRP_Client 
    @echo ""
    @echo ">>\"./client.o\" TO START MRP Client"
    @echo ""

MRP_Server 	:	librsocket.a
    $(CC) -o server.o $(server) -L. -lrsocket -lpthread

MRP_Client 	:	librsocket.a
    $(CC) -o client.o $(client) -L. -lrsocket -lpthread

Library :	librsocket.a
    @echo ""
    @echo ">>\"CC -o [Obj.o] [Source.c] -L. -lrsocket -lpthread\" TO COMPILE A PROGRAM"
    @echo ""

librsocket.a	:	rsocket.o
    ar rcs librsocket.a rsocket.o

Header	:	rsocket.o
    @echo ""
    @echo ">>\"ar rcs [LibraryName.a] rsocket.o\" TO CREATE STATIC LIBRARY"
    @echo ""

rsocket.o	:	rsocket.c	rsocket.h
    $(CC) rsocket.h 
    $(CC) -c -o rsocket.o rsocket.c

clean	:	
    rm -f server.o client.o librsocket.a rsocket.o rsocket.h.gch
    clear

remove	:	
    rm -f server.o client.o librsocket.a rsocket.o rsocket.h.gch
    