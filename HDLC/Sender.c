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

// deprecated
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
char get_hdlc_control(struct hdlc hdlc_packet);
void set_hdlc_addr(struct hdlc * hdlc_packet, char addr);
//void debug_hdlc_packet(struct hdlc hdlc_packet);
void debug_hdlc_frame(char* hdlc_frame);

//void send_sabm(char *input, int length);
void send_sabm();



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
    
     setSocket();

    while(1){
        // 메뉴 출력
        printf("===== MENU =====\n");
        printf("[1] Connect\n[2] Chat\n[3] Disconnect\n");
        printf("================\n");

        // 사용자 메뉴 선택
        /* [#] 유효한 값이 입력되지 않으면 오류 메시지 출력되도록 수정예정  */
        scanf("%d", &select);
        if(select > 3){
            printf("Invalid number!\n");
        }

        if(select == 1){
            printf("[DEBUG] Connect selected!\n");
            
            /* for test */
            send_sabm();


            // After finishing Connect
            isConnected = 1;
        }

        if(select == 2){
            if(isConnected == -1) {printf("[DEBUG] Connect first!\n");}
            else{
                printf("[DEBUG] Chat selected!\n");
                printf("[DEBUG] Chat started!\n");

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


// for checking values in hdlc packet, DEPRECATEd
// void debug_hdlc_packet(struct hdlc hdlc_packet){
//     printf("openning_flag: %#0.2x\n", hdlc_packet.opening_flag);
//     printf("addr: %c\n", hdlc_packet.addr);
//     printf("control: %#0.2x\n", get_hdlc_control(hdlc_packet));
//     printf("closing_flag: %#0.2x\n", hdlc_packet.closing_flag);
//     printf("\n");
// }


void debug_hdlc_frame(char* hdlc_frame){
    
    int total_size = 0; 
    printf("openning_flag: %#0.2x\n", hdlc_frame[0]);
    printf("addr: %c\n", hdlc_frame[1]);
    printf("control: %#0.2x\n", hdlc_frame[2]);

    
    // find closing flag
    for(int i=3; i<sizeof(MAX_SIZE); i++){
        if (hdlc_frame[i] == 0x7E) total_size = i+1;
    }
    
    // print hdlc's data field
    printf("data: ");

    if(total_size==4){printf("empty\n");}
    else{
    for(int i=3; i<total_size-2; i++){
            
        printf("data: %#0.2x ", hdlc_frame[i]);
    }
    printf("\n");
    }
    printf("closing_flag: %#0.2x\n", hdlc_frame[total_size-1]);
    printf("\n");
//     printf("closing_flag: %#0.2x\n", hdlc_packet.closing_flag);
//     printf("\n");
}

/* DEPRECATED, NOT USING STRUCT ANYMORE
char get_hdlc_addr(struct hdlc hdlc_packet){
    return hdlc_packet.addr;
}

// char get_hdlc_addr(char* hdlc_frame_ptr){
//     return hdlc_frame_ptr[1];
// }

void set_hdlc_addr(struct hdlc * hdlc_packet_ptr, char addr){ 
    hdlc_packet_ptr->addr = addr;
}

char get_hdlc_control(struct hdlc hdlc_packet){
    return hdlc_packet.control;
}

void set_hdlc_control(struct hdlc * hdlc_packet_ptr, char control){ 
    hdlc_packet_ptr->control = control;
}

void set_hdlc_flag(struct hdlc * hdlc_packet_ptr, char flag){ 
    hdlc_packet_ptr->opening_flag = flag;
}
void set_hdlc_cflag(struct hdlc * hdlc_packet_ptr, char cflag){ 
    hdlc_packet_ptr->closing_flag = cflag;
}
*/

char get_hdlc_addr(char* hdlc_frame){
    /* hldc frame size at least 4 bit */
    if(sizeof(hdlc_frame) < 4) { 
        printf("Invalid hdlc frame! please check format!"); 
        return -1; }
    return hdlc_frame[1];
    }
/* Function for testing HDLC packet*/
//void send_sabm(char *input, int length){
void send_sabm(){
    char hdlc_frame[MAX_SIZE];
    
    int total_length = 0;

    // set [openning_flag] field
    hdlc_frame[0] = DEFAULT_FLAG;

    // set destination [addr] field
    hdlc_frame[1] = 'B'; // Destination Addr : 'B' 
    
    // set [control] field 
    hdlc_frame[2] = SABM;
    printf("hdlc_frame_bytes[2] (= control): %#0.2x\n", hdlc_frame[2]);
    
    // no need to set [data] for SABM

    // set [closing_flag] field
    hdlc_frame[3] = DEFAULT_FLAG;

    debug_hdlc_frame(hdlc_frame);

    
    total_length = 4; // fixed size if there's no data in hdlc_frame

    printf("total length = %d\n", total_length);

    //send hdlc packet
    printf("[DEBUG] $ sendto(sndsock, &hdlc_packet, total_length, 0, (struct sockaddr*)&s_addr, sizeof(s_addr));");
    sendto(sndsock, &hdlc_frame, total_length, 0, (struct sockaddr*)&s_addr, sizeof(s_addr));
}


char* chat_receive(int* length)
{    
    static char data[MAX_SIZE];
    *length = recvfrom(rcvsock, data, MAX_SIZE, 0, (struct sockaddr*)&r_addr, &clen);
    return data;  
}

