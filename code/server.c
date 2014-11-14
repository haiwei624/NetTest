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

#define BUFFLENGTH 65536
#define MAXMINUS 1000

int PortNow, Maxtot; 

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
    n = recvfrom(sock, msg, BUFFLENGTH, 0, (struct sockaddr *)addr, (socklen_t *)&addr_len);
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
void ChangeNet(int bestport)
{

    PortNow = bestport;
    /*
    Code to change the network
    */
    printf("!!!!!!!!!!!!!!!!!!!!!Control Port changed to port%d\n", PortNow);
}


int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Usage: %s port port port\n", argv[0]);
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
    //printf("sock init....finish\n");

    srand(time(NULL));

    PortNow = 0;
    Maxtot = 0;

    fd_set allPortSet;
    fd_set inputs;
    FD_ZERO(&allPortSet);
    for(pi = 0; pi < 3; pi++) 
    {
        printf("sock %d : %d\n", pi, sock[pi]);
        FD_SET(sock[pi], &allPortSet);
    }

    char buff[BUFFLENGTH];
    struct sockaddr_in addr_c[3];
    while (1)
    {
        FD_ZERO(&inputs);
        inputs = allPortSet;
        int result = select(FD_SETSIZE, &inputs, NULL, NULL, NULL);  //wait until some messages arrive
        if(result <= 0)continue;
        for(pi = 0; pi < 3; pi++)
        if(FD_ISSET(sock[pi], &inputs))
        {
            recvfrom_s(sock[pi], &addr_c[pi], buff);
            printf("%s %u says: %s\n", inet_ntoa(addr_c[pi].sin_addr), ntohs(addr_c[pi].sin_port), buff);
            
            //for debug, sleep to simulate delay
            int WaitTime = rand()%400;
            usleep(WaitTime * 1000);

            sendto_s(sock[pi], &addr_c[pi], buff);//send msg
            printf("sent: %s\n", buff);

            int port, tot;
            port = buff[0]-'0';
            sscanf(&buff[1], "%d", &tot);
            //printf("%d : %d\n", tot, port);
            if(tot > Maxtot || (Maxtot - tot) > MAXMINUS)
            {
                Maxtot = tot;
                if(port != PortNow)
                {
                    ChangeNet(port);
                }
            }
        }

    }
    
    return 0;
}
