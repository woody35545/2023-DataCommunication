#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <libgen.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#define MAX_SIZE 300
#define IP_ADDR "127.0.0.1"
#define PORT 8810
#define PORTT 8811

struct L1 {
    int saddr[6];
    int daddr[6];
    int length;
    char L1_data[MAX_SIZE];
};

struct L2 {
    int saddr[4];
    int daddr[4];
    int length;
    char L2_data[MAX_SIZE];
};

void L1_send(char* input, int length);
void L2_send(char* input, int length);
void L3_send(char* data, int length);
char* L1_receive(int*);
char* L2_receive(int*);
char* L3_receive(int*);
int sndsock, rcvsock;
int clen;
struct sockaddr_in s_addr, r_addr;
void setSocket(void);
void* do_thread(void*);

void check_is_server(char *const *argv);
void init_socket();
int is_server = 0;


int main(int argc, char *argv[])
{        
    char input[MAX_SIZE];
    char output[MAX_SIZE];
    int length;
    int i = 0;
    //setSocket();
    check_is_server(argv);
    init_socket();
    pthread_t t_id;
    int status = pthread_create(&t_id, NULL, do_thread, NULL);
    if (status != 0) {
        printf("Thread Error!\n");
        exit(1);
    }
    while (1) {
        fgets(input, sizeof(input), stdin);
        L1_send(input, strlen(input));   
    }
}

void* do_thread(void* arg) {
    char output[MAX_SIZE];
    int length;
    while (1) {
        strcpy(output, L1_receive(&length));
        output[length] = '\0';
        printf("Received: %s", output);
    }
    return NULL;
}

void setSocket()
{
    //Create Send Socket///////////////////////////////////////////////
    if ((sndsock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        perror("socket error : ");
        exit(1);
    }
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(IP_ADDR);
    s_addr.sin_port = htons(PORT);
   
    ///////////////////////////////////////////////////////////////////
    //Create Receive Socket////////////////////////////////////////////
    if ((rcvsock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        perror("socket error : ");
        exit(1);
    }

    clen = sizeof(r_addr);
    memset(&r_addr, 0, sizeof(r_addr));

    r_addr.sin_family = AF_INET;
    r_addr.sin_addr.s_addr = htonl(INADDR_ANY);    
    r_addr.sin_port = htons(PORTT);

    if (bind(rcvsock, (struct sockaddr*)&r_addr, sizeof(r_addr)) < 0)
    {
        perror("bind error : ");
        exit(1);
    }
    int optvalue = 1;
    int optlen = sizeof(optvalue);
    setsockopt(sndsock, SOL_SOCKET, SO_REUSEADDR, &optvalue, optlen);
    setsockopt(rcvsock, SOL_SOCKET, SO_REUSEADDR, &optvalue, optlen);
    printf("####################################################\n");
    printf("[READY]sndsock port: %d, rcvsock port: %d \n",PORT,PORTT);
    printf("[READY]IP: %s \n",IP_ADDR);
    printf("####################################################\n");
    ///////////////////////////////////////////////////////////////////
}
void L1_send(char* input, int length)
{
    struct L1 data;
    char temp[350];
    int size = 0;

    data.saddr[0] = 0x11;
    data.saddr[1] = 0x12;
    data.saddr[2] = 0x13;
    data.saddr[3] = 0x14;
    data.saddr[4] = 0x15;
    data.saddr[5] = 0x16;

    data.daddr[0] = 0x21;
    data.daddr[1] = 0x22;
    data.daddr[2] = 0x23;
    data.daddr[3] = 0x24;
    data.daddr[4] = 0x25;
    data.daddr[5] = 0x26;

    data.length = length;

    memset(data.L1_data, 0x00, MAX_SIZE);
    strcpy(data.L1_data, input);

    size = sizeof(struct L1) - sizeof(data.L1_data) + length;

    memset(temp, 0x00, 350);
    memcpy(temp, (void*)&data, size);

    L2_send(temp, size);
}

void L2_send(char* input, int length)
{
    struct L2 data;
    char temp[350];
    int size = 0;

    data.saddr[0] = 0x33;
    data.saddr[1] = 0x33;
    data.saddr[2] = 0x33;
    data.saddr[3] = 0x33;

    data.daddr[0] = 0x44;
    data.daddr[1] = 0x44;
    data.daddr[2] = 0x44;
    data.daddr[3] = 0x44;

    data.length = length;

    memset(data.L2_data, 0x00, MAX_SIZE);
    memcpy(data.L2_data, (void*)input, length);

    size = sizeof(struct L2) - sizeof(data.L2_data) + length;

    memset(temp, 0x00, 350);
    memcpy(temp, (void*)&data, size);

    L3_send(temp, size);
}
int optvalue = 1;
int optlen = sizeof(optvalue);
void L3_send(char* data, int length)
{
    sendto(sndsock, data, length, 0, (struct sockaddr*)&s_addr, sizeof(s_addr));
}

char* L1_receive(int* length)
{
    struct L1* data;
    data = (struct L1*)L2_receive(length);
    *length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr);
    return (char*)data->L1_data;
}

char* L2_receive(int* length)
{
    struct L2* data;
    data = (struct L2*)L3_receive(length);
    *length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr);

    return (char*)data->L2_data;
}

char* L3_receive(int* length)
{    
    static char data[MAX_SIZE];
    *length = recvfrom(rcvsock, data, MAX_SIZE, 0, (struct sockaddr*)&r_addr, &clen);
    return data;  
}

void check_is_server(char *const *argv)
{
    // Check is 1 or 2
    char *progname = basename(argv[0]);
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pgrep -x %s | wc -l", progname);
    FILE *fp = popen(cmd, "r");
    if (fp == NULL)
    {
        perror("popen");
        exit(EXIT_FAILURE);
    }
    int process_count = 0;
    char line[256];
    if (fgets(line, sizeof(line), fp) != NULL)
    {
        process_count = atoi(line);
    }
    pclose(fp);
	// 처음 켜지는 쪽이 server가 되도록 설정
    if(process_count == 0) is_server = 1; else is_server = 0;
	//is_server = process_count == 1;
	printf("[!] process_count: %d\n", process_count);
	printf("[Debug] check_is_server: is_server: %d\n", is_server);

}

void init_socket()
{

    int send_port;
    int receive_port; 	

    if (is_server == 0)
    {
        printf("Session 2 starting...\n");
        send_port = 8811;
        receive_port = 8810;
    }
    else
    {
        printf("Session 1 starting...\n");
        send_port = 8810;
        receive_port = 8811;
		
    }
    // Create Send Socket
    if ((sndsock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        perror("socket error : ");
        exit(1);
    }
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(IP_ADDR);
    s_addr.sin_port = htons(send_port);
    if (connect(sndsock, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)
    {
        perror("connect error : ");
        exit(1);
    }
    // Create Receive Socket
    if ((rcvsock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        perror("socket error : ");
        exit(1);
    }

    clen = sizeof(r_addr);
    memset(&r_addr, 0, sizeof(r_addr));

    r_addr.sin_family = AF_INET;
    r_addr.sin_addr.s_addr = inet_addr(IP_ADDR);
    r_addr.sin_port = htons(receive_port);
    if (bind(rcvsock, (struct sockaddr *)&r_addr, sizeof(r_addr)) < 0)
    {
        perror("bind error : ");
        exit(1);
    }
    int optvalue = 1;
    int optlen = sizeof(optvalue);
    setsockopt(sndsock, SOL_SOCKET, SO_REUSEADDR, &optvalue, optlen);
    setsockopt(rcvsock, SOL_SOCKET, SO_REUSEADDR, &optvalue, optlen);
    printf("####################################################\n");
    printf("[READY]sndsock port: %d, rcvsock port: %d \n", send_port, receive_port);
    printf("[READY]IP: %s \n", IP_ADDR);
    printf("####################################################\n");
			
}
