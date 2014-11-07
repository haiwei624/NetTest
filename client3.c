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
#define MINFAIL 2

struct netflag
{
    int arr[ARRLENGTH];
    int p,tot;
};
struct asock
{
    int sock;
    struct sockaddr_in addr;
};
struct param
{
    struct asock *asock;
    int id;
};

pthread_mutex_t tot_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;    //just for printf when debug
pthread_mutex_t switch_mutex = PTHREAD_MUTEX_INITIALIZER;   //for SwitchPort()

const int port_priority[3] = {0, 1, 2}; //means: port0 has the highest priority to be choosed, than port1,than port2 
int netstatus[3];
unsigned int tot;
struct netflag Flags;
struct asock asock;
int PortNow;

void init_mutex();
int init_netflag(struct netflag *a);
int update_netflag(struct netflag *a, int flag);
struct asock * get_sock_c(char s_addr[], char s_port[]);
int sendto_c(struct asock * asock, char msg[]);
int recvfrom_c(struct asock * asock, char msg[]);
int wait_recv(struct asock * asock, char msg[], int WaitTime);
void * TestPort(void * v);
void * SwitchPort(void * v);
void ChangeNet(int bestport);
struct param * make_param(struct asock * asock, int id);




int main(int argc, char **argv)
{
    if (argc != 5)
    {
        printf("Usage: %s ip port port port\n", argv[0]);
        exit(1);
    }
    printf("This is a UDP client\n");

    init_mutex();   //initialize all mutex

    int pi;
    tot = 0;
    PortNow = 0;
    for(pi = 0; pi < 3; pi++)
    {
        netstatus[pi] = 1;
    }


    pthread_t pt_t[3],pt_s;
    pthread_create(&pt_s, NULL, SwitchPort, NULL);
    
    for(pi = 0; pi < 3; pi++)
    {
        struct asock * asock = get_sock_c(argv[1], argv[2+pi]);
        struct param * para = make_param(asock, pi);
        pthread_create(&pt_t[pi], NULL, TestPort, (void *)para);
    }

    for(pi = 0; pi < 3; pi++)
    {
        pthread_join(pt_t[pi], NULL);
    }
    return 0;
}


void * TestPort(void * v)
{
    //pthread_t tid = pthread_self();
    struct param *para = v;
    struct asock *asock = para->asock;
    int id = para->id;
    //init_sock_c(&asock, argv[1], argv[2]);//init the sock and addr
    struct netflag Flags;
    init_netflag(&Flags);

    //netstatus[id] = 1;//1 good,0 close
    
    char msg[BUFFLENGTH];
    while (1)
    {
        //gets(buff);
        //printf("tid: %u\n", (unsigned int) tid);

        pthread_mutex_lock(&tot_mutex);
        tot++;
        sprintf(msg, "%d%u", PortNow, tot);//prepare msg
        pthread_mutex_unlock(&tot_mutex);

        sendto_c(asock, msg);//send msg
        //printf("sent: %s\n", msg);
        


        int flag = wait_recv(asock, msg, 1000000);//wait 1s for response
        /*
        if(flag) printf("Success\n");
            else printf("Failed...timeout\n");
        */

        update_netflag(&Flags, flag);
        pthread_mutex_lock(&print_mutex);
        printf("Port%d Statue: %s %d/%d \n", id, (flag?("Success."):("Failed. ")), Flags.tot, ARRLENGTH);
        pthread_mutex_unlock(&print_mutex);

        if(ARRLENGTH - Flags.tot >= MAXFAIL && netstatus[id] == 1)
        {
            netstatus[id] = 0;
            pthread_mutex_unlock(&switch_mutex);//let SwitchPort runs~
        }
        else if(ARRLENGTH - Flags.tot <= MINFAIL && netstatus[id] == 0)
        {
            netstatus[id] = 1;
            pthread_mutex_unlock(&switch_mutex);//let SwitchPort runs~
        } 
        
        /*
        if(netstatus[id] == 0) printf("Closed.\n");
            else printf("Good\n");
        */

        /*recvfrom_c(sock, &addr, buff);//recv msg
        printf("received: %s\n", buff);*/
        usleep(10000);
    }
    
    return 0;
}


void * SwitchPort(void * v)
{
    while(1)
    {
        pthread_mutex_lock(&switch_mutex);
        pthread_mutex_lock(&print_mutex);
        printf("!!!SwitchPort runs~  Netstatus: ");
        int i;
        for(i = 0; i < 3; i++)
            if(netstatus[i] == 0) printf("Closed.");
                else printf("Good.  ");
        printf("\n");
        pthread_mutex_unlock(&print_mutex);
        int bestport = port_priority[2];// assume that the last priority port is most stable
        for(i = 0; i < 3; i++)
        if(netstatus[port_priority[i]])
        {
            bestport = port_priority[i];
            break;
        }
        if(bestport != PortNow) ChangeNet(bestport);
    }
}

void ChangeNet(int bestport)
{

    pthread_mutex_lock(&tot_mutex);
    PortNow = bestport;
    /*
    Code to change the network
    */
    pthread_mutex_unlock(&tot_mutex);

    pthread_mutex_lock(&print_mutex);
    printf("!!!!!!!!!!!!!!!!!!!!!Control Port changed to port%d\n", PortNow);
    pthread_mutex_unlock(&print_mutex);
}

void init_mutex()//initialize all mutex
{
    pthread_mutex_init(&tot_mutex, NULL);
    pthread_mutex_init(&print_mutex, NULL);//just for printf when debug
    pthread_mutex_init(&switch_mutex, NULL);//for SwitchPort()
}

struct param * make_param(struct asock * asock, int id)
{
    struct param *a = malloc(sizeof(struct param));
    a->id = id;
    a->asock = asock;
    return a;
}

int init_netflag(struct netflag *a)
{
    int i;
    for(i = 0; i < ARRLENGTH; i++)
    {
        a->arr[i] = 1;
    }
    a->p = 0;
    a->tot = ARRLENGTH;
    return 0;
}

int update_netflag(struct netflag *a, int flag)
{
    a->tot -= a->arr[a->p];
    a->arr[a->p] = flag && 1;
    a->tot += a->arr[a->p];
    a->p = (a->p + 1) % ARRLENGTH;
    return 0;
}

struct asock * get_sock_c(char s_addr[], char s_port[])
{
    struct asock * asock;
    asock = malloc(sizeof(struct asock));
    if((asock->sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("init_socket");
        exit(1);
    }
    asock->addr.sin_family = AF_INET;
    asock->addr.sin_port = htons(atoi(s_port));
    asock->addr.sin_addr.s_addr = inet_addr(s_addr);
    if(asock->addr.sin_addr.s_addr == INADDR_NONE)
    {
        printf("Incorrect ip address!\n");
        close(asock->sock);
        exit(1);
    }
    return asock;
}

int sendto_c(struct asock *asock, char msg[]) // sendto for client
{
    int n;
    n = sendto(asock->sock, msg, strlen(msg), 0, (struct sockaddr *)&(asock->addr), sizeof(asock->addr));
    if (n < 0)
    {
        perror("sendto");
        //close(asock->sock);
        /* print error but continue */
        return -1;
    }
    return 0;
}

int recvfrom_c(struct asock *asock, char msg[]) //recvfrom for client
{
    int addr_len = sizeof(asock->addr);
    int n;
    n = recvfrom(asock->sock, msg, BUFFLENGTH, MSG_DONTWAIT, (struct sockaddr *)&(asock->addr), (socklen_t *)&addr_len);
    if (n>0)
        {
            msg[n] = 0;
        }
    else
        {
            msg[0] = 0;//clear the string if failed
        }
    return n;
}

int wait_recv(struct asock * asock, char msg[], int WaitTime)// Wait UDP response for WaitTime usec
{
    static char buff[BUFFLENGTH];
    fd_set inputs;
    struct timeval timeout;
    FD_ZERO(&inputs);
    FD_SET(asock->sock, &inputs);
    timeout.tv_sec = WaitTime / 1000000;
    timeout.tv_usec = WaitTime % 1000000;
    while(timeout.tv_usec > 0 || timeout.tv_sec > 0)
    {
        int result = select(FD_SETSIZE, &inputs, NULL, NULL, &timeout);
        //printf("%d : %d : %d\n", result, (int)timeout.tv_sec, (int)timeout.tv_usec);
        if(result <= 0)
            return 0;
        int n = recvfrom_c(asock, buff);
        if((n > 0) && (strcmp(msg, buff) == 0)) 
            return 1;
    }
    return 0;
}