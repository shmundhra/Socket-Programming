# mytraceroute

This is a self implementation of the Linux Command **traceroute**.  
Here we use Raw Sockets and create the IP and UDP Headers Manually, incrementing the TTL Field progressively so as to detect and record each HOP.

### Execution
    make exec - Compiles traceroute into a binary and adds it to the bin with +x rights so that it can be run as a command
