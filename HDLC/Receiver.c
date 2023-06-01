#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_SIZE 300
#define IPADDR "127.0.0.1"
#define PORT 8811
#define PORTT 8810
#define RECEIVER_ADDR 'B' // 자신에게 온 hdlc 패킷인지 확인할 때 사용. Receiver의 주소는 'B'라고 가정

// struct hdlc;
// void debug_hdlc_packet(struct hdlc hdlc_packet);

void chat_send(char* data, int length);
char* receive(int*);

int sndsock, rcvsock;
int clen;
struct sockaddr_in s_addr, r_addr;


void setSocket(void);
void* do_thread(void*);


int isConnected = -1; // 1: connected , -1: not connected

int main(void)
{        
    char input[MAX_SIZE];
    char output[MAX_SIZE];
    int length;
    int i = 0;
    setSocket();
    pthread_t t_id;
    int status = pthread_create(&t_id, NULL, do_thread, NULL);
    if (status != 0) {
        printf("Thread Error!\n");
        exit(1);
    }

    
    while (1) {
        fgets(input, sizeof(input), stdin);
        chat_send(input, strlen(input));   
    }
    
}

void* do_thread(void* arg) {
    char output[MAX_SIZE];
    
    int length;
    while (1) {
        strcpy(output, receive(&length));
        output[length] = '\0';

        // Sender로부터 받은 메시지 디버깅을 위해서 String 타입과 Bytes 단위로 출력
        
        printf("[DEBUG] Received\n\t<String>\n\t\t%s", output);
        printf("\t<Bytes>\n\t\t");
        for(int i=0; i<length; i++){
            printf("[%d]%x ", i, output[i]);
        } 
        printf("\n--------------\n");
        
        

        /* HDLC Protocol Start */
        if(isConnected == -1){
            /* Wating For receiving SBAM Message from sender */
            //struct hdlc * hdlc_ptr = (struct hdlc *)output;
            //debug_hdlc_packet(*hdlc_ptr);

        }

    }
    return NULL;
}

// void debug_hdlc_packet(struct hdlc hdlc_packet){
//     printf("openning_flag: %#0.2x\n", hdlc_packet.opening_flag);
//     printf("addr: %c\n", hdlc_packet.addr);
//     printf("control: %#0.2x\n", hdlc_packet.control);
//     printf("closing_flag: %#0.2x\n", hdlc_packet.closing_flag);
//     printf("\n");
// }


void setSocket()
{
    if ((sndsock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        perror("socket error : ");
        exit(1);
    }
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(IPADDR);
    s_addr.sin_port = htons(PORT);

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
    // printf("####################################################\n");
    // printf("[READY]sndsock port: %d, rcvsock port: %d \n",PORT,PORTT);
    // printf("[READY]IP: %s \n",IPADDR);
    // printf("####################################################\n");
    ///////////////////////////////////////////////////////////////////
}

void chat_send(char* data, int length)
{
    sendto(sndsock, data, length, 0, (struct sockaddr*)&s_addr, sizeof(s_addr));
}


char* receive(int* length)
{    
    static char data[MAX_SIZE];
    *length = recvfrom(rcvsock, data, MAX_SIZE, 0, (struct sockaddr*)&r_addr, &clen);
    return data;  
}

