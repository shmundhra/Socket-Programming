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

#define SERVER_PORT 10000
#define BUFF_SIZE 99

typedef struct cclient_info
{
    struct in_addr IP;
    unsigned short int Port;
} client_info;

int max(int a, int b)
{
    return ((a > b) ? a : b);
}

int ret;
int cfd[] = {-1, -1, -1, -1, -1};
client_info cid[5];
int numc = 0;

void handleAccept(int lsock)
{
    struct sockaddr_in client;
    int cli_len = sizeof(client);

    cfd[numc] = accept(lsock, (struct sockaddr *)&client, &cli_len);
    //Check ;
    cid[numc].IP = client.sin_addr;
    cid[numc].Port = client.sin_port;
    numc++;

    printf("Server: Received a New Connection from Client ");
    printf("%s:%d\n", inet_ntoa(cid[numc-1].IP), ntohs(cid[numc-1].Port));
    return;
}

void handleReceive(int i)
{
    char buffer[BUFF_SIZE + 1];
    int msg;
    int total_b, recv_b;
    total_b = recv_b = 0;

    while (1)
    {
        recv_b = recv(cfd[i], buffer + total_b, sizeof(char), 0);
        //Check
        total_b += recv_b;
        if (buffer[total_b - 1] == '\0')
        {
            break;
        }
    }
    recv_b = recv(cfd[i], &msg, sizeof(msg), MSG_WAITALL);
    //Check
    msg = ntohl(msg);

    printf("Server: Received Message \"");
    printf("%s%d\" ", buffer, msg);
    printf("from Client ");
    printf("%s:%d\n", inet_ntoa(cid[i].IP), ntohs(cid[i].Port));

    if (numc == 1)
    {
        printf("Server: Insufficient clients, \"");
        printf("%s%d\" ", buffer, msg);
        printf("from Client ");
        printf("%s:%d ", inet_ntoa(cid[i].IP), ntohs(cid[i].Port));
        printf("dropped\n");
        return;
    }

    int j;
    for (j = 0; j < numc; j++)
    {
        if (i == j)
            continue;
        ret = send(cfd[j], &cid[i].IP, sizeof(cid[i].IP), 0);
        //Check
        ret = send(cfd[j], &cid[i].Port, sizeof(cid[i].Port), 0);
        //Check
        ret = send(cfd[j], buffer, total_b, 0);
        //Check
        int nbo_msg = ntohl(msg);
        ret = send(cfd[j], &nbo_msg, sizeof(nbo_msg), 0);
        //Check

        printf("Server: Sent Message \"");
        printf("%s%d\" ", buffer, msg);
        printf("from Client ");
        printf("%s:%d ", inet_ntoa(cid[i].IP), ntohs(cid[i].Port));
        printf("to Client ");
        printf("%s:%d\n", inet_ntoa(cid[j].IP), ntohs(cid[j].Port));
    }
    return;
}

int main()
{
    int lsock;
    lsock = socket(AF_INET, SOCK_STREAM, 0);
    //Check

    int enable = 1;
    ret = setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    //Check

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(SERVER_PORT);

    ret = bind(lsock, (struct sockaddr *)&server, sizeof(server));
    //Check

    listen(lsock, 5);
    //Check
    printf("Server is Running... \n");

    int i;
    while (1)
    {
        fd_set fds;

        FD_ZERO(&fds);

        FD_SET(lsock, &fds);
        for (i = 0; i < numc; i++)
        {
            if (cfd[i] != -1)
                FD_SET(cfd[i], &fds);
        }

        int nfds = lsock;
        for (i = 0; i < numc; i++)
        {
            if (cfd[i] != -1)
                nfds = max(nfds, cfd[i]);
        }

        ret = select(nfds + 1, &fds, NULL, NULL, NULL);
        //Check

        if (FD_ISSET(lsock, &fds))
        {
            printf("Accept Connection Received !! \n\n");
            handleAccept(lsock);
        }
        for (i = 0; i < numc; i++)
        {
            if (cfd[i] != -1 && FD_ISSET(cfd[i], &fds))
            {
                printf("Message Received from %d !! \n\n", i);
                handleReceive(i);
            }
        }
    }
    return 0;
}
