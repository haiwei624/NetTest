#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define BUFFLENGTH 512

int init_sock_s(int *sock, struct sockaddr_in *addr,  char s_port[])
{
    
    if((*sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("init_socket");
        exit(1);
    }
    addr->sin_family = AF_INET;
    addr->sin_port = htons(atoi(s_port));
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    
    if(bind(*sock, (struct sockaddr *)addr, sizeof(*addr)) < 0)
    {
        perror("init_socket_bind");
        exit(1);
    }
    return 0;
}

int sendto_s(int sock, struct sockaddr_in *addr, char msg[])
{
    int n;
    n = sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)addr, sizeof(*addr));
    if (n < 0)
    {
        perror("sendto");
        close(sock);
        return -1;
    }
    return 0;
}

int recvfrom_s(int sock, struct sockaddr_in *addr, char msg[])
{
    int addr_len = sizeof(*addr);
    int n;
    //n = recvfrom(sock, msg, BUFFLENGTH, 0, (struct sockaddr *)&addr, &addr_len);
    n = recvfrom(sock, msg, BUFFLENGTH, 0, (struct sockaddr *)addr, &addr_len);
    if (n>0)
        {
            msg[n] = 0;
        }
    else
        {
            //printf("n==-1 here\n");
            perror("recvfrom");
            //close(sock);
            //exit(1);
            msg[0] = 0;
            return -1;
        }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Usage: %s port\n", argv[0]);
        exit(1);
    }
    printf("This is a UDP server, I will received message from client and reply with same message\n");

    int sock[3];
    struct sockaddr_in addr[3];
    int pi;
    for(pi = 0; pi < 3; pi++)
    {
        init_sock_s(&sock[pi], &addr[pi], argv[1+pi]);//init the sock and addr
    }
    

    printf("sock init....finish\n");

    srand(time(NULL));
    printf("%d\n",(int)time(NULL));

    char buff[BUFFLENGTH];
    struct sockaddr_in addr_c;
    while (1)
    {
        

        recvfrom_s(sock, &addr_c, buff);//recv msg
        printf("%s %u says: %s\n", inet_ntoa(addr_c.sin_addr), ntohs(addr_c.sin_port), buff);
        //printf("%s\n", buff);
        int WaitTime = rand()%1000;
        usleep(WaitTime * 1000);

        sendto_s(sock, &addr_c, buff);//send msg
        printf("sent: %s\n", buff);

        

        //usleep(300000);
    }
    
    return 0;
}
