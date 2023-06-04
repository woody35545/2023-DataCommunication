#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>


#define MAX_SIZE 300
#define IPADDR "127.0.0.1"
#define PORT 8811
#define PORTT 8810
#define RECEIVER_ADDR 'B' // 자신에게 온 hdlc 패킷인지 확인할 때 사용. Receiver의 주소는 'B'라고 가정
#define DEFAULT_FLAG 0x7E
#define SABM 0x01
#define UA 0x02
#define DISC 0x03

void print_current_time();
void send_ua();
void debug_hdlc_frame(char* hdlc_frame);
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
    char received[MAX_SIZE];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    int length;
    while (1) {
        strcpy(received, receive(&length));
        received[length] = '\0';

        // Sender로부터 받은 메시지 디버깅을 위해서 String 타입과 Bytes 단위로 출력
        print_current_time();
        printf("프레임을 수신하였습니다. (크기: %d bytes)\n", length);
                printf("  └─────> 수신한 프레임(bytes): ");
        for(int i=0; i<length; i++){
            printf("[%d]%#0.2x ", i, received[i]);
        } 
        printf("\n");
        

        /* HDLC Protocol Start */
        if(isConnected == -1){
            // 받은 HDLC 프레임이 SABM인지 확인
            if(received[2] == SABM) { 
                sleep(1);
                print_current_time();
                printf("Sender로부터 SABM을 수신하였습니다.\n");
                
                // UA 전송
                sleep(1);
                print_current_time();
                printf("Sender에게 UA를 전송합니다.\n\n");
                
                send_ua();
                
                print_current_time();
                printf("연결 성공\n");
                isConnected =1 ;
            } 
        }

    }
    return NULL;
}

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

}

void send_ua(){
    char hdlc_frame[MAX_SIZE];
    int total_length = 0; 

    hdlc_frame[0] = DEFAULT_FLAG;    // set [openning_flag] field
    hdlc_frame[1] = 'A'; // set [Destination Addr]: 'A'
    hdlc_frame[2] = UA; // set [control] field 
    // no need to set [data] for SABM
    hdlc_frame[3] = DEFAULT_FLAG; // set [closing_flag] field
    //debug_hdlc_frame(hdlc_frame);


    total_length = 4;
    // testing get_data function
    
    //send hdlc packet
    sendto(sndsock, &hdlc_frame, total_length, 0, (struct sockaddr*)&s_addr, sizeof(s_addr));
    
}

void chat_send(char* data, int length)
{
    sendto(sndsock, data, length, 0, (struct sockaddr*)&s_addr, sizeof(s_addr));
}


char* receive(int* length)
{    
    static char data[MAX_SIZE];
    *length = recvfrom(rcvsock, data, MAX_SIZE, 0, (struct sockaddr*)&r_addr, (unsigned int *)&clen);
    return data;  
}

void print_current_time(){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("[%02d:%02d:%02d] ", tm.tm_hour, tm.tm_min, tm.tm_sec);
}
