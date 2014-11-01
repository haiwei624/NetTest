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
#define ARRLENGTH 10
#define MAXFAIL 3
#define MINFAIL 1
int netstatue;

int init_sock_c(int *sock, struct sockaddr_in *addr, char s_addr[], char s_port[])
{
    
    if((*sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("init_socket");
        exit(1);
    }
    addr->sin_family = AF_INET;
    addr->sin_port = htons(atoi(s_port));
    addr->sin_addr.s_addr = inet_addr(s_addr);
    if(addr->sin_addr.s_addr == INADDR_NONE)
    {
        printf("Incorrect ip address!");
        close(*sock);
        exit(1);
    }
    return 0;
}

int sendto_c(int sock, struct sockaddr_in *addr, char msg[])
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

int recvfrom_c(int sock, struct sockaddr_in *addr, char msg[])
{
    int addr_len = sizeof(*addr);
    int n;
    //n = recvfrom(sock, msg, BUFFLENGTH, 0, (struct sockaddr *)&addr, &addr_len);
    n = recvfrom(sock, msg, BUFFLENGTH, MSG_DONTWAIT, (struct sockaddr *)addr, (socklen_t *)&addr_len);
    if (n>0)
        {
            msg[n] = 0;
        }
    else if (n==0)
        {
            printf("server closed\n");
            //close(sock);
            //exit(1);
            msg[0] = 0;
            return 0;
        }
        else if (n == -1)
        {
            //printf("n==-1 here\n");
            //perror("recvfrom");
            //close(sock);
            //exit(1);
            msg[0] = 0;
            return -1;
        }
    return n;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: %s ip port\n", argv[0]);
        exit(1);
    }
    printf("This is a UDP client\n");

    int sock;
    struct sockaddr_in addr;
    init_sock_c(&sock, &addr, argv[1], argv[2]);//init the sock and addr

    char msg[BUFFLENGTH], buff[BUFFLENGTH];
    unsigned int tot = 0;
    int maxtimes = 100;//100*0.01s=1s
    int flagarr[ARRLENGTH];
    int arr_p;
    for(arr_p = 0; arr_p < ARRLENGTH; arr_p++)
    {
        flagarr[arr_p] = 1;
    }
    arr_p=0;
    int totflag = ARRLENGTH;
    netstatue = 1;//1 good,0 close

    while (1)
    {
        //gets(buff);
        tot++;
        sprintf(msg, "%u", tot);//prepare msg

        sendto_c(sock, &addr, msg);//send msg
        printf("sent: %s\n", msg);

        //usleep(1000000);
        int flag = 0, times;
        for(times = 1; times <= maxtimes; times++)
        {
            usleep(10000);//0.01s
            int n = recvfrom_c(sock, &addr, buff);
            if((n > 0) && (strcmp(msg, buff) == 0))
            {
                flag = 1;
                break;
            }
        }

        if(flag)
        {
            printf("Success: received after %d times.\n", times);
        }
        else
        {
            printf("Failed...timeout\n");
        }

        totflag -= flagarr[arr_p];
        flagarr[arr_p] = flag;
        totflag += flagarr[arr_p];
        arr_p = (arr_p+1)%ARRLENGTH;
        printf("Statue: %d/%d\n", totflag, ARRLENGTH);

        if(ARRLENGTH - totflag >= MAXFAIL) netstatue = 0;
            else if(ARRLENGTH - totflag <= MINFAIL) netstatue = 1;
        if(netstatue == 0) printf("Closed.\n");
            else printf("Good\n");

        /*recvfrom_c(sock, &addr, buff);//recv msg
        printf("received: %s\n", buff);*/
        usleep(500000);
    }
    
    return 0;
}