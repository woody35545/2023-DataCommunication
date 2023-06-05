#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>



#define MAX_SIZE 300
#define IPADDR "127.0.0.1"
#define PORT 8811
#define PORTT 8810
#define RECEIVER_ADDR 'B' // 자신에게 온 hdlc 패킷인지 확인할 때 사용. Receiver의 주소는 'B'라고 가정
#define DEFAULT_FLAG 0x7E
#define SABM 0x01
#define UA 0x02
#define DISC 0x03
#define IFRAME 0x04
#define MIN_HDLC_SIZE 4
#define RECEIVER_ADDR 'B'



struct control;

void print_current_time();
void print_frame(const char* array, int length);

void send_ua();
void debug_hdlc_frame(char* hdlc_frame);
void chat_send(char* data, int length);
char* receive(int*);
char* make_response_str(const char* received_data);
char get_hdlc_addr(char* hdlc_frame);

int sndsock, rcvsock;
int clen;
struct sockaddr_in s_addr, r_addr;


void setSocket(void);
void* do_thread(void*);


struct control{
    unsigned b0 : 1;
    unsigned b1 : 1;
    unsigned b2 : 1;
    unsigned b3 : 1;
    unsigned b4 : 1;
    unsigned b5 : 1;
    unsigned b6 : 1;
    unsigned b7 : 1;
};

int isConnected = -1; // 1: connected , -1: not connected

int main(void)
{        
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    
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
        if(isConnected){

        fgets(input, sizeof(input), stdin);
        printf("[%02d시%02d분/전송] %s", tm.tm_hour,tm.tm_min, input);

        chat_send(input, strlen(input));   
        }
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

        if(isConnected == 1) {
            // receive chatting
            if(received[2] == IFRAME) { 
                time_t t = time(NULL);
                struct tm tm = *localtime(&t);

                // Decapsulate
                
                char data[length-4];

                printf("[%02d시%02d분/수신] ", tm.tm_hour,tm.tm_min);
                
                for(int i=0; i<length-4; i++){
                    data[i] = received[3+i];
                    printf("%c", received[3+i]);
                }                

                char* converted = make_response_str(data);
                converted[length-4] = '\0';
                printf("[*] 다음과 같이 응답합니다: %s\n",converted);
                
                sleep(1);
                sendto(sndsock, converted, length-4, 0, (struct sockaddr*)&s_addr, sizeof(s_addr));

            
            }

            else if(received[2]==DISC){
                print_current_time();
                printf("Sender로부터 연결 해제 요청(DISC)을 수신하였습니다.\n");
                sleep(1);

                send_ua(); 
                print_current_time();
                printf("연결 해제 완료\n");
                isConnected = -1;
            }
            
        }

        /* HDLC Protocol Start */
        //if(isConnected == -1){
            // 받은 HDLC 프레임이 SABM인지 확인
            if(received[2] == SABM) { 
                sleep(1);
                print_current_time();
                printf("Sender로부터 SABM을 수신하였습니다.\n");
                printf("  └─────> 수신한 프레임(%d bytes): ", length);
                for(int i=0; i<length; i++){
                    printf("[%d]%#0.2x ", i, received[i]);
                } printf("\n");

                print_frame(received, length);

                printf("\t[*] flag, cflag 값을 확인합니다...\n"); sleep(1);
                if(received[0] == DEFAULT_FLAG && received[length-1] == DEFAULT_FLAG) 
                {
                    printf("\t  └────> flag:%#02x\n\t  └────> cflag:%#02x\n\t\t\t(확인완료)\n\n", received[0], received[length-1]);}// [!] 조건 추후 수정 예정
                else{
                    printf("\n\t\t[!] flag 또는 cflag 값이 유효하지 않습니다.\n");
                }

                printf("\t[*] Address 값을 확인합니다...\n"); sleep(1);
                if(get_hdlc_addr(received) == RECEIVER_ADDR) {
                    printf("\t  └────> address:%c\n\t\t\t(확인완료)\n\n", get_hdlc_addr(received));} 
                else{
                    printf("\n\t[!] address 값이 유효하지 않습니다.\n");
                }

                // UA 전송
                sleep(1);
                send_ua();
                
                print_current_time();
                printf("연결 완료\n");
                isConnected =1 ;
            } 
        //}

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
    print_current_time();
    printf("Sender에게 UA를 전송합니다.\n");
    printf("  └─────> 전송한 프레임(%d bytes): ", total_length);
    for(int i=0; i<total_length; i++){
        printf("[%d]%#0.2x ", i, hdlc_frame[i]);
    } printf("\n");
    print_frame(hdlc_frame,total_length);

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

void print_frame(const char* array, int length) {
    // 테이블 헤더 출력
    printf("\t");
    printf("┌────────┬");
    for (int i = 0; i < length; i++) printf("────────┬"); 
    printf("\n");

    printf("\t");
    printf("│ Index  │"); for (int i = 0; i < length; i++) printf("  %02d    │", i);
    printf("\n");

    printf("\t");
    printf("├────────┼"); for (int i = 0; i < length; i++) printf("────────┼");
    printf("\n");

    // 값 출력
    printf("\t");
    printf("│ Value  │");
    for (int i = 0; i < length; i++) printf("  0x%02x  │", array[i]);
    printf("\n");

    // 구분선 출력
    printf("\t");
    printf("└────────┴");
    for (int i = 0; i < length; i++) printf("────────┴");
    printf("\n");
}

char* make_response_str(const char* received_data) {
    // ctype.h 헤더를 통해 구현

    int i = 0;
    char* res = malloc(strlen(received_data) + 1);  // 결과 문자열을 저장할 메모리를 동적으로 할당

    while (received_data[i]) {
        if (islower(received_data[i])) {
            res[i] = toupper(received_data[i]);  // 소문자를 대문자로 변환하여 결과 문자열에 저장
        } else if (isupper(received_data[i])) {
            res[i] = tolower(received_data[i]);  // 대문자를 소문자로 변환하여 결과 문자열에 저장
        } else {
            res[i] = received_data[i];  // 알파벳이 아닌 문자는 그대로 결과 문자열에 저장
        }
        i++;
    }

    res[i] = '\0';  // 결과 문자열의 끝을 표시하는 널 문자('\0') 추가
    return res;
}

char get_hdlc_addr(char* hdlc_frame){
    /* hldc frame size at least 4 bit */
    if(sizeof(hdlc_frame) < MIN_HDLC_SIZE) { 
        printf("Invalid hdlc frame! please check format!"); 
        return -1; }
    return hdlc_frame[1];
}