#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_SIZE 300
#define IPADDR "127.0.0.1"
#define PORT 8810
#define PORTT 8811
#define DEFAULT_FLAG 0x7E
#define SABM 0x01
#define UA 0x02
#define DISC 0x03


struct hdlc
{
    char opening_flag;
    char addr;
    char control;
    char data[MAX_SIZE];
    char closing_flag;
};
int isConnected = -1; // Connect 또는 Disconnect 상태를 저장하기 위한 변수. 1: connect, -1: disconnect

struct hdlc;
void set_hdlc_addr(struct hdlc * hdlc_packet, char addr);
void debug_hdlc_packet(struct hdlc hdlc_packet);
void send_sabm(char *input, int length);



void chat_send(char* data, int length);

char* chat_receive(int*);
int sndsock, rcvsock;
int clen;
struct sockaddr_in s_addr, r_addr;
void setSocket(void);
void* do_thread(void*);



int main(void)
{        
    char input[MAX_SIZE];
    char output[MAX_SIZE];
    int length;
    int i = 0;
    int select; // 사용자가 선택한 메뉴 번호 저장

    while(1){
    // 메뉴 출력
    printf("===== MENU =====\n");
    printf("[1] Connect\n[2] Chat\n[3] Disconnect\n");
    printf("================\n");

    // 사용자 메뉴 선택
    /* [#] 유효한 값이 입력되지 않으면 오류 메시지 출력되도록 수정예정  */
    scanf("%d", &select);
    if(select > 3){printf("Invalid number!\n");}
    if(select == 1){
        printf("[DEBUG] Connect selected!\n");
        
        /* for test */
        send_sabm(NULL, NULL);


        // After finishing Connect
        isConnected = 1;
    }
    if(select == 2){
        if(isConnected == -1) {printf("[DEBUG] Connect first!\n");}
        else{
            printf("[DEBUG] Chat selected!\n");
            printf("[DEBUG] Chat started!\n");

            setSocket();
            pthread_t t_id;
            int status = pthread_create(&t_id, NULL, do_thread, NULL);
            if (status != 0) {
                printf("Thread Error!\n");
                exit(1);
            }
            while (1) {
                fflush(stdin);
                fflush(stdout);
                printf("[Send] ");
                fgets(input, sizeof(input), stdin);
                chat_send(input, strlen(input));   
            }
        }


    }
     if(select == 3){
        if(isConnected == -1) {printf("[DEBUG] Connect first!\n");}
        else{
        printf("[DEBUG] Disconnect selected!\n");
        
        // After finishing disConnect
        isConnected = -1;
        }
    
    }
    }
}

void* do_thread(void* arg) {
    char output[MAX_SIZE];
    int length;
    while (1) {
        strcpy(output, chat_receive(&length));
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
    s_addr.sin_addr.s_addr = inet_addr(IPADDR);
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


// for checking values in hdlc packet
void debug_hdlc_packet(struct hdlc hdlc_packet){
    
    printf("openning_flag: %#0.2x\n", hdlc_packet.opening_flag);
    printf("addr: %c\n", hdlc_packet.addr);
    printf("control: %#0.2x\n", hdlc_packet.control);
    printf("closing_flag: %#0.2x\n", hdlc_packet.closing_flag);
    printf("\n");
}


void set_hdlc_addr(struct hdlc * hdlc_packet, char addr){ 
    hdlc_packet->addr = addr;
}
void set_hdlc_control(struct hdlc * hdlc_packet, char control){ 
    hdlc_packet->control = control;
}
void set_hdlc_flag(struct hdlc * hdlc_packet, char flag){ 
    hdlc_packet->opening_flag = flag;
}
void set_hdlc_cflag(struct hdlc * hdlc_packet, char cflag){ 
    hdlc_packet->closing_flag = cflag;
}


/* Function for testing HDLC packet*/
void send_sabm(char *input, int length){
    struct hdlc hdlc_packet;

    set_hdlc_flag(&hdlc_packet, DEFAULT_FLAG);
    set_hdlc_addr(&hdlc_packet, 'B');
    set_hdlc_control(&hdlc_packet, SABM); 
    set_hdlc_cflag(&hdlc_packet, DEFAULT_FLAG);

    debug_hdlc_packet(hdlc_packet);

    // send hdlc packet
    //printf("[DEBUG] $ sendto(sndsock, &hdlc_packet, sizeof(hdlc_packet), 0, (struct sockaddr*)&s_addr, sizeof(s_addr));");
    sendto(sndsock, &hdlc_packet, sizeof(hdlc_packet), 0, (struct sockaddr*)&s_addr, sizeof(s_addr));
}


char* chat_receive(int* length)
{    
    static char data[MAX_SIZE];
    *length = recvfrom(rcvsock, data, MAX_SIZE, 0, (struct sockaddr*)&r_addr, &clen);
    return data;  
}

