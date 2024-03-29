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
#define DEFAULT_FLAG 0x7E
#define SABM 0x01
#define UA 0x02
#define DISC 0x03
#define IFRAME 0x04
#define MIN_HDLC_SIZE 4
#define RECEIVER_ADDR 'B'
#define SENDER_ADDR 'A'

struct control;

/* getters */
int get_iframe_sequence_number(struct control* c);
int get_iframe_acknowledge_number(struct control* c);
char get_hdlc_addr(char* hdlc_frame);

/* setters */
void set_iframe_sequence_number(struct control* c, unsigned seq_num);
void set_iframe_acknowledge_number(struct control* c, unsigned seq_num);
void set_hdlc_iframe(struct control* control_bits);
void set_hdlc_uframe(struct control* control_bits);
void set_hdlc_sframe(struct control* control_bits);

/* conditions */
int is_uframe(char* hdlc_frame);
int is_iframe(char* hdlc_frame);
int is_sframe(char* hdlc_frame);

/* send functions */
void send_ua();
void chat_send(char* data, int length);

/* receive functions */
char* receive(int*);
void* do_thread(void*);

/* socket */
int sndsock, rcvsock;
int clen;
struct sockaddr_in s_addr, r_addr;
void setSocket(void);

/* etc */
char* make_response_str(const char* received_data);
void debug_hdlc_frame(char* hdlc_frame);
void print_current_time();
void print_frame(const unsigned char* array, int length);
int ack=0;

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
       
        // if(isConnected){
        // fgets(input, sizeof(input), stdin);
        // printf("[%02d시%02d분/전송] %s", tm.tm_hour,tm.tm_min, input);

        // chat_send(input, strlen(input));   
        // }
    }
}

void* do_thread(void* arg) {
    char received[MAX_SIZE];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    int length;
    while (1) {
        length = recvfrom(rcvsock, received, MAX_SIZE, 0, (struct sockaddr*)&r_addr, (unsigned int *)&clen);

        if(isConnected == 1) {
            // receive chatting

            if(is_iframe(received)) { 
                
                time_t t = time(NULL);
                struct tm tm = *localtime(&t);
                
                char data[length-MIN_HDLC_SIZE];

                //printf("[%02d시%02d분/수신]\n", tm.tm_hour,tm.tm_min);
                //printf("  └────> 수신내용 > ");
                
                printf("\t");print_current_time();
                printf("Received [SEQ:%d] " , get_iframe_sequence_number((struct control *)&received[2]));
                for(int i=0; i<length-4; i++){
                    data[i] = received[3+i];
                }          
                data[length-4] = '\0';
                printf("%s", data);
                print_frame((unsigned char *)received, length);
                

                char* response_data = make_response_str(data);
                int length_of_response_data = length-4;
                response_data[length_of_response_data] = '\0';

                printf("\t[*] Sender에게 다음과 같이 응답을 전송합니다:\n");
                printf("\t  └────> 전송 내용 > [ACK:%d] %s", get_iframe_sequence_number((struct control *)&received[2])+1 ,response_data);
                printf("\t"); print_current_time();
                printf("Send [ACK:%d] %s", get_iframe_sequence_number((struct control *)&received[2])+1 ,response_data);

                
                /* 응답을 위한 iframe 프레임 생성 */
                int length_of_response_iframe = length_of_response_data+MIN_HDLC_SIZE;
                unsigned char response_iframe[length_of_response_iframe];
                
                /* Control 비트 설정 */
                struct control ctr;
                set_hdlc_iframe(&ctr); // iframe 으로 설정
                set_iframe_sequence_number(&ctr, 0);
                 // 방금 sender로부터 수신한 sequence number+1 한 값으로 ACK 값 설정.
                //printf("Contorl ACK 설정 %d\n",get_iframe_sequence_number((struct control *)&received[2]) + 1);
                set_iframe_acknowledge_number(&ctr, get_iframe_sequence_number((struct control *)&received[2]) + 1); 
                //printf("control: %#02x %#02x %#02x %#02x %#02x %#02x %#02x %#02x\n", ctr.b0,ctr.b1,ctr.b2,ctr.b3,ctr.b4,ctr.b5,ctr.b6,ctr.b7);
            
                response_iframe[0] = DEFAULT_FLAG;
                // Sender 주소를 Destination으로 설정
                response_iframe[1] = 'A'; 
                response_iframe[2] = *(unsigned char *)&ctr;

                // Response Data 값 할당
                for(int i=0; i< length_of_response_data; i++){
                    response_iframe[3+i] = response_data[i];
                }

                // cflag 할당
                response_iframe[3 + length_of_response_data] = DEFAULT_FLAG;
            /*--------------------------*/

                sleep(1);
                
                sendto(sndsock, response_iframe, length_of_response_data+MIN_HDLC_SIZE, 0, (struct sockaddr*)&s_addr, sizeof(s_addr));

                print_frame(response_iframe, length_of_response_iframe);
            
            }

            else if(received[2]==DISC){
                print_current_time();
                printf("Sender로부터 연결 해제 요청(DISC)을 수신하였습니다.\n");
                sleep(1);

                
            printf("\t[*] flag, cflag 값을 확인합니다...\n");
            if(received[0] == DEFAULT_FLAG && received[length-1] == DEFAULT_FLAG) 
            {
                printf("\t  └────> flag:%#02x\n\t  └────> cflag:%#02x\n\t\t\t(확인완료)\n\n", received[0], received[length-1]);}// [!] 조건 추후 수정 예정
            else{
                printf("\n\t\t[!] flag 또는 cflag 값이 유효하지 않습니다.\n");
            }

            printf("\t[*] Address 값을 확인합니다...\n");
            if(get_hdlc_addr(received) == RECEIVER_ADDR) 
            {printf("\t  └────> address:%c\n\t\t\t(확인완료)\n\n", get_hdlc_addr(received));} 
            else{
                printf("\n\t[!] address 값이 유효하지 않습니다.\n");
            }

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
                //sleep(1);
                print_current_time();
                printf("Sender로부터 SABM을 수신하였습니다.\n");
                printf("  └─────> 수신한 프레임(%d bytes): ", length);
                for(int i=0; i<length; i++){
                    printf("[%d]%#0.2x ", i, received[i]);
                } printf("\n");

                print_frame((unsigned char *)received, length);

                printf("\t[*] flag, cflag 값을 확인합니다...\n");
                if(received[0] == DEFAULT_FLAG && received[length-1] == DEFAULT_FLAG) 
                {
                    printf("\t  └────> flag:%#02x\n\t  └────> cflag:%#02x\n\t\t\t(확인완료)\n\n", received[0], received[length-1]);}// [!] 조건 추후 수정 예정
                else{
                    printf("\n\t\t[!] flag 또는 cflag 값이 유효하지 않습니다.\n");
                }

                printf("\t[*] Address 값을 확인합니다...\n"); 
                if(get_hdlc_addr(received) == RECEIVER_ADDR) {
                    printf("\t  └────> address:%c\n\t\t\t(확인완료)\n\n", get_hdlc_addr(received));} 
                else{
                    printf("\n\t[!] address 값이 유효하지 않습니다.\n");
                }

                // UA 전송
                //sleep(1);
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
    int total_length = MIN_HDLC_SIZE; // MIN_HDLC_SIZE=4

    hdlc_frame[0] = DEFAULT_FLAG; // flag 설정, DEFAULT_FLAG(=0x7E)
    hdlc_frame[1] = SENDER_ADDR; // sender의 주소인 SENDER_ADDER(='A')로 설정
    hdlc_frame[2] = UA; // 사전 정의한 UA에 대한 비트값 UA(=0x02)로 설정
    hdlc_frame[3] = DEFAULT_FLAG; // cflag 설정, DEFAULT_FLAG(=0x7E)

    // 전송 내용 출력
    print_current_time();
    printf("Sender에게 UA를 전송합니다.\n");
    printf("  └─────> 전송한 프레임(%d bytes): ", total_length);
    for(int i=0; i<total_length; i++){
        printf("[%d]%#0.2x ", i, hdlc_frame[i]);
    } printf("\n");
    print_frame((unsigned char *)hdlc_frame,total_length);
    
    // socket으로 생성한 UA Frame 전송
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
    printf("(%02d:%02d:%02d) ", tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void print_frame(const unsigned char* array, int length) {
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

    printf("\t");
    printf("│ Value  │");
    for (int i = 0; i < length; i++) printf("  0x%02x  │", array[i]);
    printf("\n");

    printf("\t");
    printf("└────────┴");
    for (int i = 0; i < length; i++) printf("────────┴");
    printf("\n");
}

char* make_response_str(const char* received_data) {
    int i = 0;
    char* res = malloc(strlen(received_data) + 1);  // 결과 문자열을 저장할 메모리를 동적으로 할당

    while (received_data[i]) {
        if (islower(received_data[i])) {
            res[i] = toupper(received_data[i]);  // 소문자인 경우 대문자로 변환 
        } else if (isupper(received_data[i])) {
            res[i] = tolower(received_data[i]);  // 대문자인 경우 소문자로 변환
        } else {
            res[i] = received_data[i];  // 알파벳이 아닌 경우 그대로 저장
        }
        i++;
    }

    res[i] = '\0';  
    return res;
}

char get_hdlc_addr(char* hdlc_frame){
    /* hldc frame size at least 4 bit */
    if(sizeof(hdlc_frame) < MIN_HDLC_SIZE) { 
        printf("Invalid hdlc frame! please check format!"); 
        return -1; }
    return hdlc_frame[1];
}


int is_iframe(char* hdlc_frame){
    // control field는 hdlc_frame의 index 2에 위치해있음.
    struct control *control_ptr = (struct control *)&hdlc_frame[2];
    // control(8bit)의 첫비트가 0 이면 iframe임을 의미
    if(control_ptr->b0 == 0) return 1;
    return 0; 
}

int is_uframe(char* hdlc_frame){
    // control field는 hdlc_frame의 index 2에 위치해있음.
    struct control *control_ptr = (struct control *)&hdlc_frame[2];
    // control(8bit)의 첫 두비트가 11 이면 uframe임을 의미
    if(control_ptr->b0 == 1 && control_ptr->b1 == 1) return 1;
    return 0; 
}

int is_sframe(char* hdlc_frame){
    // control field는 hdlc_frame의 index 2에 위치해있음.
    struct control *control_ptr = (struct control *)&hdlc_frame[2];
    // control(8bit)의 첫 두비트가 10 이면 sframe임을 의미
    if(control_ptr->b0 == 1 && control_ptr->b1 == 0) return 1; 
    return 0; 
}


int get_iframe_sequence_number(struct control* c){
    unsigned int res = (c->b1 << 2) | (c->b2 << 1) | c->b3;
    return res;
}

int get_iframe_acknowledge_number(struct control* c){
    unsigned int res = (c->b5 << 2) | (c->b6 << 1) | c->b7;
    return res;
}

void set_hdlc_iframe(struct control* control_bits){
    control_bits->b0 = 0; 
}

void set_hdlc_uframe(struct control* control_bits){
    control_bits->b0 = 1;
    control_bits->b1 = 1; 
}
void set_hdlc_sframe(struct control* control_bits){
    control_bits->b0 = 1;
    control_bits->b1 = 0; 
}
void set_iframe_sequence_number(struct control* c, unsigned seq_num) {
    if (seq_num < 8) { 
        // 각 자리수에 해당하는 값으로 &를 취해서 해당자리 값만 남긴 후, 한자리 비트값으로 만들기 위해 자리수 만큼 right shift
        c->b1 = (seq_num & 4) >> 2; 
        c->b2 = (seq_num & 2) >> 1;
        c->b3 = seq_num & 1;
    }
}

void set_iframe_acknowledge_number(struct control* c, unsigned ack_num){

    if (ack_num < 8) {
        // 각 자리수에 해당하는 값으로 &를 취해서 해당자리 값만 남긴 후, 한자리 비트값으로 만들기 위해 자리수 만큼 right shift
        c->b5 = (ack_num & 4) >> 2; 
        c->b6 = (ack_num & 2) >> 1;
        c->b7 = ack_num & 1;
    }
    
}
