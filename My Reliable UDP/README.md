# MY RELIABLE PROTOCOL (MRP) SOCKETS

This is a Reliable User Datagram Protocol using MyReliableProtocol(MRP) Sockets.  
We created a Static Library with all required functions for our protocol - socket(), send(), recv(), close() and created a Concurrent Thread which managed the Receiving of Messages and placed them into the Receive Buffer. This Thread also managed the Acknowledgements and the Re-transmissions to ensure reliability.