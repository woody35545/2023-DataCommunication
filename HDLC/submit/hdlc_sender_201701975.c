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
#define PORT 8810
#define PORTT 8811
#define DEFAULT_FLAG 0x7E
#define SABM 0x01
#define UA 0x02
#define DISC 0x03
#define IFRAME 0x04
#define MIN_HDLC_SIZE 4
#define SENDER_ADDR 'A'
#define RECEIVER_ADDR 'B'
#define WINDOW_SIZE 4 // for GBN

int sock = 0, valread;
struct sockaddr_in serv_addr;

void send_frame(char* data, int length);
unsigned char buffer[1024] = {0};
/*────────────────────────────────────────────────────────*/
struct control;

/* getter */
char get_hdlc_addr(char* hdlc_frame);
char get_hdlc_control(char* hdlc_frame);
unsigned char* get_hdlc_data(unsigned char* hdlc_frame);
int get_iframe_sequence_number(struct control* c);

/* setter */
void set_iframe_sequence_number(struct control* c, unsigned seq_num);
void set_iframe_acknowledge_number(struct control* c, unsigned seq_num);
int get_iframe_acknowledge_number(struct control* c);
void set_hdlc_data(unsigned char* hdlc_frame, unsigned char* data);
void set_hdlc_addr(char* hdlc_frame, char hdlc_addr);
void set_hdlc_iframe(struct control* control_bits);
void set_hdlc_uframe(struct control* control_bits);
void set_hdlc_sframe(struct control* control_bits);

/* conditions */
int is_uframe(char* hdlc_frame);
int is_iframe(char* hdlc_frame);
int is_sframe(char* hdlc_frame);

/* send functions */
void send_sabm();
void send_disc();

/* receive functions */
char* chat_receive(unsigned int*);
void* do_thread(void*);

/* socket */
int sndsock, rcvsock;
int clen;
struct sockaddr_in s_addr, r_addr;
void setSocket(void);
int valread;

/* etc */
void debug_hdlc_frame(char* hdlc_frame);
void print_current_time();
void print_frame(const unsigned char* array, int length);
void print_titlebar(const char* selectedMenu);
// 사용자가 입력한 채팅 데이터로 iframe을 만든후 리턴해주는 함수
unsigned char* make_chat_frame(char *chat_data, int chat_length); 

int DEBUG_MODE = 1;
int isConnected = -1; // Connect 또는 Disconnect 상태를 저장하기 위한 변수. 1: connect, -1: disconnect


/* for GBN */
int seq_num = 0;
int seq;
int base = 0;
unsigned char* sending_buffer[WINDOW_SIZE];

/*────────────────────────────────────────────────────────*/
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

int main(void)
{        
    char input[MAX_SIZE];
    char output[MAX_SIZE];
    int length;
    int i = 0;
    int select; // 사용자가 선택한 메뉴 번호 저장

    setSocket();

    while(1){
    print_titlebar("MAIN PAGE");
    printf("\t┌──────── STATUS ──────────────┐\n");        
    printf("\t│ - Connection: %-14s │\n", (isConnected == 1) ? "Connected" : "Not Connected");
    printf("\t└──────────────────────────────┘\n");
    
    // 메뉴 출력
    printf("\t┌───────── MENU ─────────┐\n");
    printf("\t│ [1] Connect            │\n");
    printf("\t│ [2] Chat               │\n");
    printf("\t│ [3] Disconnect         │\n");
    printf("\t└────────────────────────┘\n");

        // 사용자 메뉴 선택
        /* [#] 유효한 값이 입력되지 않으면 오류 메시지 출력되도록 수정예정  */
        printf("[>] 메뉴를 선택하세요: ");
        scanf("%d", &select);
        printf("\n────────────────────────────────────────────────────────\n");
        if(select > 3){
            printf("Invalid number!\n");
        }

        if(select == 1){
            print_titlebar("Connect");

            // 연결을 위해 Receiver에 SABM(u-frame) 전송
            send_sabm();

            // UA를 대기
            char received[MAX_SIZE]; // 수신받은 UA 값을 담을 배열 
            unsigned int length; 

            printf("\n\tUA 수신 대기 중...\n");            

            int start_time;
            start_time = (unsigned)time(NULL);
    
                while(1) {
                // printf("start_time: %d\n", start_time); 확인해보니까 chat_receive에서 Recv 할 때까지 멈춰버리기 때문에 시간조건 timer로 안됨. 
                if((unsigned)time(NULL) > start_time + 2){
                     printf("연결 시간 초과. 연결에 실패하였습니다.\n");
                      break;}
                
                strcpy(received, chat_receive(&length));
                received[length] = '\0';
                // Reciever로부터 받은 메시지 디버깅을 위해서 String 타입과 Bytes 단위로 출력
        
                if(received[2] == UA) { 
                    sleep(1);
                    print_current_time();
                    printf("Receiver로부터 UA를 수신하였습니다. (크기: %d bytes)\n", length);
                    printf("  └────> 수신한 프레임(bytes): ");
                    for(int i=0; i<length; i++){
                        printf("[%d]%#0.2x ", i, received[i]);
                    } 
                    printf("\n");
                    print_frame((unsigned char*)received, length);

                    printf("\t[*] flag, cflag 값을 확인합니다...\n");
                    if(received[0] == DEFAULT_FLAG && received[length-1] == DEFAULT_FLAG) 
                    {
                        printf("\t  └────> flag:%#02x\n\t  └────> cflag:%#02x\n\t\t\t(확인완료)\n\n", received[0], received[length-1]);}// [!] 조건 추후 수정 예정
                    else{
                        printf("\n\t\t[!] flag 또는 cflag 값이 유효하지 않습니다.\n");
                    }

                    printf("\t[*] Address 값을 확인합니다...\n");
                    if(get_hdlc_addr(received) == SENDER_ADDR) 
                    {printf("\t  └────> address:%c\n\t\t\t(확인완료)\n\n", get_hdlc_addr(received));} 
                    else{
                        printf("\n\t[!] address 값이 유효하지 않습니다.\n");
                    }
                    sleep(1);
                    print_current_time();
                    printf("연결 완료\n\n");
                    printf("────────────────────────────────────────────────────────\n");

                    isConnected = 1;
                    break;
                } // 수신받은 메시지의 control이 UA이면 통과.                 
            }                     
        }

        if(select == 2){
            if(isConnected == -1) {
                printf("[!] 연결이 필요합니다.\n");
                printf("────────────────────────────────────────────────────────\n");
                sleep(1); 
            }

            else{
                int nextseqnum = 0;

                print_titlebar("Chatting");
                printf("////////////////////////////////////////////////////////////////\n");
                printf("// [>] 채팅이 시작되었습니다. \t\t\t\t      //\n//\t * 메시지를 입력하신 후 Enter를 누르시면 전송됩니다.  //\n//\t (\"exit\" 또는 \"quit\"을 입력하시면 채팅이 종료됩니다.) //\n");
                printf("////////////////////////////////////////////////////////////////\n");
                sleep(1);
                pthread_t t_id;

                while (1) {
                    fflush(stdin);
                    fflush(stdout);
                    time_t t = time(NULL);
                    struct tm tm = *localtime(&t);

                    memset(input, 0x00, sizeof(input));

                    fgets(input, sizeof(input),stdin);

                    // 채팅 입력에 "quit" 또는 "exit" 입력시 채팅 종료 후 메인 메뉴로 돌아가도록 함.
                    if(strcmp(input, "quit\n")== 0 || strcmp(input, "exit\n")== 0){
                        printf("\n────────────────────────────────────────────────────────\n");
                        printf("[>] 채팅을 종료합니다.\n");
                        printf("────────────────────────────────────────────────────────\n");
                        sleep(1);
                        break;
                    }

                    if(nextseqnum < base+WINDOW_SIZE){
                        // 현재 sliding window 상태와 보내려는 sequence 비교.
                        sending_buffer[nextseqnum % WINDOW_SIZE] = malloc(1024); 
                        if (sending_buffer[nextseqnum % WINDOW_SIZE] == NULL) {
                        printf("[!] 메모리 할당에 실패하였습니다.\n");
                        return -1;
                    }
                                        
                    // 사용자가 입력한 채팅 내용을 담은 Iframe을 만들어서 chat_iframe 할당.
                    unsigned char* chat_iframe = make_chat_frame(input, strlen(input));
                    int chat_frame_length = strlen(input) + MIN_HDLC_SIZE;

                   
                    memset(sending_buffer[nextseqnum % WINDOW_SIZE], 0x00, 1024); // sending_buffer 초기화

                    unsigned char* p = sending_buffer[nextseqnum % WINDOW_SIZE];
                    for(int i=0; i<chat_frame_length; i++){
                        p[i] = chat_iframe[i];    
                    }

                    // Sending Buffer에 담겨있는 내용 window size 만큼 모두 전송
                    for (int i = 0; i < WINDOW_SIZE; i++) {
                        if (sending_buffer[i] != NULL){
                            printf("\t");print_current_time();
                            printf("Send[SEQ:%d] %s", seq, input);
                            sendto(sndsock, sending_buffer[i], chat_frame_length, 0, (struct sockaddr*)&s_addr, sizeof(s_addr));
                            print_frame(sending_buffer[i], chat_frame_length); printf("\n");
                            nextseqnum++;
                        } 
                    }   
                    // 할당했던 메모리 해제
                    free(chat_iframe);
                    seq ++;
                    }
                    else{ 
                        printf("[!] Sending Buffer가 가득 찼습니다.\n");
                    }
                        /*────────────────────────────────────────────────────────────────────────────────────────────────────────────────*/
                        memset(buffer,0x00, sizeof(buffer));
                        
                        // Timer start
                        time_t start, end;
                        double result;
                        // GBN timer 시작. 전송 후 ACK를 5초간 대기하고 수신하지 못하면 해당 frame부터 재전송.
                        start = time(NULL);

                        // Receiver로 부터 ACK 수신 대기
                        valread = read(rcvsock, buffer, 1024);
                        char data[valread-MIN_HDLC_SIZE];
                        for(int i=0; i<valread-MIN_HDLC_SIZE; i++){
                            data[i] = buffer[3+i];
                        }
                        data[valread-MIN_HDLC_SIZE-1] = '\0';

                        struct control* c = (struct control *)(&buffer[2]);
            
                        // Timer end
                        end = time(NULL);
                        result = (double)(end - start);

                        if (sizeof(buffer) > 0 && result <= 5) {
                            // 수신한 ACK 번호 출력
                            printf("\t");print_current_time();
                            printf("Received[ACK:%d]: %s\n", get_iframe_acknowledge_number(c), data); 
                            print_frame((unsigned char *)buffer,strlen((const char*)buffer));
                            printf("\n");
                            
                            base++;

                            // ack를 받은 frame에 대한 메모리 해제
                            free(sending_buffer[(base - 1) % WINDOW_SIZE]);
                            sending_buffer[(base - 1) % WINDOW_SIZE] = NULL;

                            if (base % WINDOW_SIZE == 0) {
                                for (int i = 0; i < WINDOW_SIZE; i++) {
                                    // send_frames
                                    if (sending_buffer[i] != NULL) {
                                        sendto(sndsock, sending_buffer[i], strlen(input)+MIN_HDLC_SIZE , 0, (struct sockaddr*)&s_addr, sizeof(s_addr));
                                        printf("Sequence number %d번 Frame을 전송합니다.\n", seq + i);
                                    }
                                }
                            }
                        } else {
                            printf("Sequence number %d번에 대한 NAK을 수신하였습니다.\n", base);
                            // Resend frames
                            for (int i = 0; i < WINDOW_SIZE; i++) {
                                if (sending_buffer[i] != NULL) {
                                    sendto(sndsock, sending_buffer[i], strlen(input)+MIN_HDLC_SIZE , 0, (struct sockaddr*)&s_addr, sizeof(s_addr));
                                    printf("Sequence number %d번 Frame부터 재전송합니다.\n", seq + i);
                                }
                            }
                        }
                        /*────────────────────────────────────────────────────────────────────────────────────────────────────────────────*/
                }
            }
        }

        if(select == 3){
            char received[MAX_SIZE];
            unsigned int length;

            if(isConnected == -1) {
                printf("[!] 연결되어 있지 않습니다.\n");
                printf("────────────────────────────────────────────────────────\n\n");
                sleep(1);
            }
            
            else{         

                print_current_time();                   
                // 연결 해제를 위해서 Receiver로 DISC(u-frame) 전송
                send_disc();
                
                // DISC 전송 후 Receiver로부터 UA 수신 대기
                printf("\n\tUA 수신 대기 중...\n");

                while(1)
                {
                    strcpy(received, chat_receive(&length));
                    received[length] = '\0';
                    if(received[2] == UA) { 
                        sleep(1);
                        print_current_time();
                        printf("Receiver로부터 UA를 수신하였습니다. (크기: %d bytes)\n", length);
                        printf("  └────> 수신한 프레임(bytes): ");
                        for(int i=0; i<length; i++){
                            printf("[%d]%#0.2x ", i, received[i]);
                        } 
                        printf("\n");
                        print_frame((unsigned char*)received, length);

                        /*────────────────── flag, cflag 값 확인 ──────────────────*/
                        printf("\t[*] flag, cflag 값을 확인합니다...\n");
                        if(received[0] == DEFAULT_FLAG && received[length-1] == DEFAULT_FLAG) 
                        {
                            printf("\t  └────> flag:%#02x\n\t  └────> cflag:%#02x\n\t\t\t(확인완료)\n\n", received[0], received[length-1]);}// 
                        else{
                            printf("\n\t\t[!] flag 또는 cflag 값이 유효하지 않습니다.\n");
                        }
                        
                        /*──────────────────── Address 값 확인 ─────────────────────*/
                        printf("\t[*] Address 값을 확인합니다...\n");
                        if(get_hdlc_addr(received) == SENDER_ADDR) 
                        {printf("\t  └────> address:%c\n\t\t\t(확인완료)\n\n", get_hdlc_addr(received));} 
                        else{
                            printf("\n\t[!] address 값이 유효하지 않습니다.\n");
                        }

                        sleep(1);
                        print_current_time();
                        printf("연결 해제 완료\n\n");
                        printf("────────────────────────────────────────────────────────\n");

                        isConnected = -1;
                        break;
                    }
                }
            }
        }
    }
}

void* do_thread(void* arg) {
    char received[MAX_SIZE];
    unsigned int length;

    while (1) {
        strcpy(received, chat_receive(&length));
        received[length] = '\0';
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);

        printf("\t[%02d시%02d분/Receiver 응답] %s", tm.tm_hour,tm.tm_min,received);
    }
    return NULL;
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

unsigned char* make_chat_frame(char *chat_data, int chat_length) {
    unsigned char* hdlc_frame = (unsigned char*)malloc((chat_length + MIN_HDLC_SIZE + 1) * sizeof(unsigned char));
    if (hdlc_frame == NULL) {
        return NULL;  // 메모리 할당 실패 시 NULL 반환
    }

    struct control ctr;
    set_hdlc_iframe(&ctr);
    set_iframe_sequence_number(&ctr, seq);
    set_iframe_acknowledge_number(&ctr, 0);

    hdlc_frame[0] = DEFAULT_FLAG;
    hdlc_frame[1] = 'B';
    hdlc_frame[2] = *(unsigned char *)&ctr;

    // /* DEBUG */
    // printf("[DEBUG] %s\n" , __func__);
    // for(int i=0; i< chat_length; i++){
    //     printf("chat_data[%d]: %#02x\n" , i ,chat_data[i]);
    // }

    for(int i=0; i< chat_length; i++){
        hdlc_frame[3+i] = chat_data[i];
    }

    hdlc_frame[3 + chat_length] = DEFAULT_FLAG;

    return hdlc_frame;
}

void send_frame(char* frame, int length) { 
    // socket을 이용하여 Receiver로 Frame 전송
    sendto(sndsock, frame, length, 0, (struct sockaddr*)&s_addr, sizeof(s_addr)); }

int is_iframe(char* hdlc_frame){
    // control field는 hdlc_frame의 index 2에 위치해있음.
    struct control * control_ptr = (struct control *) &hdlc_frame[2];
    // control(8bit)의 첫비트가 0 이면 iframe임을 의미
    if(control_ptr->b0 == 0) return 1;
    return 0; 
}

int is_uframe(char* hdlc_frame){
    // control field는 hdlc_frame의 index 2에 위치해있음.
    struct control * control_ptr = (struct control *) &hdlc_frame[2];
    // control(8bit)의 첫 두비트가 11 이면 uframe임을 의미
    if(control_ptr->b0 == 1 && control_ptr->b1 == 1) return 1;
    return 0; 
}

int is_sframe(char* hdlc_frame){
    // control field는 hdlc_frame의 index 2에 위치해있음.
    struct control * control_ptr = (struct control *) &hdlc_frame[2];
    // control(8bit)의 첫 두비트가 10 이면 sframe임을 의미
    if(control_ptr->b0 == 1 && control_ptr->b1 == 0) return 1; 
    return 0; 
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

int get_iframe_sequence_number(struct control* c){
    unsigned int res = (c->b1 << 2) | (c->b2 << 1) | c->b3;
    return res;
}

int get_iframe_acknowledge_number(struct control* c){
    unsigned int res = (c->b5 << 2) | (c->b6 << 1) | c->b7;
    return res;
}

char get_hdlc_addr(char* hdlc_frame){
    if(sizeof(hdlc_frame) < MIN_HDLC_SIZE) { 
        printf("Invalid hdlc frame! please check format!"); 
        return -1; }
    return hdlc_frame[1];
}

void set_hdlc_addr(char* hdlc_frame, char hdlc_addr){
    hdlc_frame[1] = hdlc_addr;
}
char get_hdlc_control(char* hdlc_frame){
    if(sizeof(hdlc_frame) < MIN_HDLC_SIZE) { 
        printf("Invalid hdlc frame! please check format!"); 
        return -1; }
    
    return hdlc_frame[2];    
}

void set_hdlc_control(char* hdlc_frame, char hdlc_control){ 
    hdlc_frame[2] = hdlc_control; 
}


unsigned char* get_hdlc_data(unsigned char* hdlc_frame) {
    if (sizeof(hdlc_frame) < MIN_HDLC_SIZE) {  // hdlc_frame의 크기가 MIN_HDLC_SIZE보다 작거나 같은 경우 hdlc 프레임의 포멧이 잘못되었음을 의미.
        printf("Invalid hdlc frame! please check format!"); 
        return NULL;
    }
    else if(sizeof(hdlc_frame) == MIN_HDLC_SIZE){
        // data field가 empty 인 경우
        return NULL;
    }
    
    int total_size;
    // closing flag 위치 찾기
    for (int i = 3; i < sizeof(hdlc_frame); i++) {
        if (hdlc_frame[i] == 0x7E)  // 0x7E는 HDLC 프레임의 closing flag 값
            printf("hdlc_frame[%d]: %x\n", i, hdlc_frame[i] );
            total_size = i + 1;  // total_size 갱신
    }

    printf("total_length: %d\n", total_size);

    // hdlc의 데이터 필드 출력
    int data_length = total_size - MIN_HDLC_SIZE;
    printf("data_length: %d\n", data_length);

    unsigned char* data = malloc(data_length);  // 데이터 필드를 저장할 동적 메모리 할당
    if (data == NULL) {
        printf("Failed to allocate memory for data array");  // 데이터 배열에 메모리 할당 실패
        return NULL;
    }


    // 데이터 필드 복사
    printf("Copy data...\n");
    for (int i = 0; i < data_length; i++) {    
        data[i] = (unsigned char)hdlc_frame[i + 3];  // HDLC에서 데이터 필드는 인덱스 3부터 시작하므로 hdlc_frame 3번부터 차례대로 복사
        printf("data[%d]: %x ", i, data[i]);  // 데이터 필드 출력

    }

    return data;
}

void set_hdlc_data(unsigned char* hdlc_frame, unsigned char* data){
    // [!] check needed!
    for(int i=0; i<sizeof(data); i++){
        hdlc_frame[i+3] = data[i];
    }
}


void send_sabm(){    
    // Receiver로 SABM 전송하는 함수
    char hdlc_frame[MAX_SIZE];
    int total_length = 0; 

    /* Frame의 각 필드값 할당 */
    hdlc_frame[0] = DEFAULT_FLAG;   // flag 설정
    hdlc_frame[1] = RECEIVER_ADDR; // Receiver의 주소(='B')로 목적지 주소를 설정
    hdlc_frame[2] = SABM; // control을 SABM로 설정
    // SABM(uframe)은 별도의 데이터를 필요로 하지 않으므로 생략
    hdlc_frame[3] = DEFAULT_FLAG; // cflag 설정

    // 별도의 데이터를 포함하지 않기 때문에 전송하는 프레임 크기는 헤더 크기인 MIN_HDLC_SIZE(=4)로 할당
    total_length = MIN_HDLC_SIZE;
    
    // 생성한 SABM 프레임을 send socket을 이용하여 Receiver로 전송
    sendto(sndsock, &hdlc_frame, total_length, 0, (struct sockaddr*)&s_addr, sizeof(s_addr));

    // 연결 요청했다는 메시지와 방금 보낸 프레임 내용 출력 
    print_current_time();
    printf("연결을 요청합니다.(SABM)\n");
    printf("  └────> 보낸 프레임(%d bytes): \n", total_length);
    print_frame((unsigned char *)hdlc_frame, total_length);
    
}

void send_disc(){
    char hdlc_frame[MAX_SIZE];
    int total_length = 0; 

    hdlc_frame[0] = DEFAULT_FLAG;
    hdlc_frame[1] = 'B'; 
    hdlc_frame[2] = DISC; 
    hdlc_frame[3] = DEFAULT_FLAG; 

    total_length = 4;
    
    //send hdlc packet
    sendto(sndsock, &hdlc_frame, total_length, 0, (struct sockaddr*)&s_addr, sizeof(s_addr));

    print_current_time();
    printf("연결 해제를 요청합니다.(DISC)\n");
    printf("  └────> 보낸 프레임(bytes): \n");
    print_frame((unsigned char*)hdlc_frame, total_length);
    
}

void print_current_time(){
    /* 현재 시간 출력해주는 함수 */
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("(%02d:%02d:%02d) ", tm.tm_hour, tm.tm_min, tm.tm_sec);
}

char* chat_receive(unsigned int* length)
{    
    /* 채팅을 Receive Socket을 통해 수신한 후 받은 채팅 데이터를 리턴하는 함수 */
    static char data[MAX_SIZE];
    *length = recvfrom(rcvsock, data, MAX_SIZE, 0, (struct sockaddr*)&r_addr, (unsigned int *)&clen);
    return data;  
}

void print_frame(const unsigned char* array, int length) {
    /* 
        프레임 내에 값을 깔끔하게 테이블 형태로 출력해주기 위한 함수. 
        바이트 단위 인덱스를 나누고 16진수 형태로 Value를 출력하도록 구현하였음
    */
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

void print_titlebar(const char* selectedMenu) {
    int headerWidth = 14;  

    printf("╔");
    for (int i = 0; i < headerWidth - 2; i++) {
        printf("═");
    }
    printf("╗\n");

    printf("║");
    int padding = (headerWidth - 2 - strlen(selectedMenu)) / 2;
    for (int i = 0; i < padding; i++) {
        printf(" ");
    }
    printf("%s", selectedMenu);
    for (int i = 0; i < padding; i++) {
        printf(" ");
    }
    if ((strlen(selectedMenu) % 2) != 0) {
        printf(" ");
    }
    printf("║\n");

    printf("╠");
    for (int i = 0; i < headerWidth - 2; i++) {
        printf("═");
    }
    printf("╣\n");

}

/* 디버깅용 함수, 과제와 무관한 함수 */ 
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
}
