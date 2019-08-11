#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//#include <sys/type.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>

#define SERVER_PORT 10000
#define BUFF_SIZE 99

int ret;

typedef struct cclient_info
{
    struct in_addr IP;
    unsigned short int Port;
} client_info;

int max(int a, int b)
{
    return ((a > b) ? a : b);
}

int main()
{
    int tcp;
    tcp = socket(AF_INET, SOCK_STREAM, 0);

    ret = fcntl(tcp, F_SETFL, O_NONBLOCK);
    //Check ;

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    inet_aton("127.0.0.1", &server.sin_addr);
    server.sin_port = htons(SERVER_PORT);

    ret = connect(tcp, (struct sockaddr *)&server, sizeof(server));
    //Check
    int n = 1;
    const char *msg = "Message";
    client_info ci;

    while (1)
    {
        srand(time(NULL));
        int num = rand() % 3 + 1;
        sleep(num);

        if (n < 6)
        {
            ret = send(tcp, msg, strlen(msg) + 1, 0);
            //Check
            int nbo = htonl(n);
            ret = send(tcp, &nbo, sizeof(nbo), 0);
            //Check

            printf("%s%d sent\n", msg, n);
            n++;
        }

        ret = recv(tcp, &ci.IP, sizeof(ci.IP), MSG_WAITALL);
        if (ret < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            else
                exit(EXIT_FAILURE);
        }
        ret = recv(tcp, &ci.Port, sizeof(ci.Port), MSG_WAITALL);
        //Check

        char buffer[BUFF_SIZE + 1];
        int msg;
        int total_b, recv_b;
        total_b = recv_b = 0;

        while (1)
        {
            recv_b = recv(tcp, buffer + total_b, sizeof(char), 0);
            //Check
            total_b += recv_b;
            if (buffer[total_b - 1] == '\0')
            {
                break;
            }
        }
        recv_b = recv(tcp, &msg, sizeof(msg), MSG_WAITALL);
        //Check
        msg = ntohl(msg);
        printf("Client: Received Message \"");
        printf("%s%d\" ", buffer, msg);
        printf("from Client ");
        printf("%s:%d\n", inet_ntoa(ci.IP), ntohs(ci.Port));
    }

    return 0;
}
