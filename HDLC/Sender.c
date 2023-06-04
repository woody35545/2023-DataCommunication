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

#define MIN_HDLC_SIZE 4




int isConnected = -1; // Connect 또는 Disconnect 상태를 저장하기 위한 변수. 1: connect, -1: disconnect

int get_current_hour();
int get_current_min();
int get_current_sec();

void print_current_time();
void print_frame(const char* array, int length);
void print_titlebar(const char* selectedMenu);

unsigned char* get_hdlc_data(unsigned char* hdlc_frame);
void set_hdlc_data(unsigned char* hdlc_frame, unsigned char* data);
char get_hdlc_control(char* hdlc_frame);
void set_hdlc_addr(char* hdlc_frame, char hdlc_addr);
void debug_hdlc_frame(char* hdlc_frame);

//void send_sabm(char *input, int length);
void send_sabm();


void chat_send(char* data, int length);

char* chat_receive(unsigned int*);
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

    // pthread_t t_id;
    // int status = pthread_create(&t_id, NULL, do_thread, NULL);
    // if (status != 0) {
    //     printf("Thread Error!\n");
    //     exit(1);
    // }

    while(1){
    print_titlebar("MAIN PAGE");
    printf("\t┌──────── STATUS ────────┐\n");        
        printf("\t│ - Connection: %-8s │\n", (isConnected == 1) ? "O" : "X");
        printf("\t└────────────────────────┘\n");
    

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
        printf("────────────────────────────────────────────────────────\n");
        if(select > 3){
            printf("Invalid number!\n");
        }

        if(select == 1){
            print_titlebar("Connect");
            
            /* for test */
            send_sabm();

            // UA를 대기
            char received[MAX_SIZE];
            unsigned int length;

            printf("\n\tUA 수신 대기 중...\n");
            
            int start_time;
            
            start_time = (unsigned)time(NULL);
    

                while(1) {
                // printf("start_time: %d\n", start_time); 확인해보니까 chat_receive에서 Recv 할 때까지 멈춰버리기 때문에 시간조건 timer로 안됨. 
                // printf("now: %d\n", (unsigned)time(NULL));
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
                    print_frame(received, length);
                    sleep(1);
                    print_current_time();
                    printf("연결 성공\n\n");
                    printf("────────────────────────────────────────────────────────\n");

                    isConnected = 1;
                    break;
                } // 수신받은 메시지의 control이 UA이면 통과. 

                //else if(wait_start > )
                
            }   

                    
                
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
    char received[MAX_SIZE];
    unsigned int length;

    while (1) {
        strcpy(received, chat_receive(&length));
        received[length] = '\0';
        // Reciever로부터 받은 메시지 디버깅을 위해서 String 타입과 Bytes 단위로 출력
        print_current_time();
        printf("프레임을 수신하였습니다. (크기: %d bytes)\n", length);
        printf("\t\tDEBUG > 수신한 프레임(bytes): ");
        for(int i=0; i<length; i++){
            printf("[%d]%#0.2x ", i, received[i]);
        } 
        printf("\n");
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

void chat_send(char* data, int length)
{
    sendto(sndsock, data, length, 0, (struct sockaddr*)&s_addr, sizeof(s_addr));
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



char get_hdlc_addr(char* hdlc_frame){
    /* hldc frame size at least 4 bit */
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

/* Function for testing HDLC packet*/
//void send_sabm(char *input, int length){
void send_sabm(){
    

    char hdlc_frame[MAX_SIZE];
    int total_length = 0; 

    hdlc_frame[0] = DEFAULT_FLAG;    // set [openning_flag] field
    hdlc_frame[1] = 'B'; // set [Destination Addr]: 'B' 
    hdlc_frame[2] = SABM; // set [control] field 
    // no need to set [data] for SABM
    hdlc_frame[3] = DEFAULT_FLAG; // set [closing_flag] field
    //debug_hdlc_frame(hdlc_frame);


    total_length = 4;
    // testing get_data function
    
    //send hdlc packet
    sendto(sndsock, &hdlc_frame, total_length, 0, (struct sockaddr*)&s_addr, sizeof(s_addr));

    print_current_time();
    printf("연결을 요청합니다.(SABM)\n");
    printf("  └────> 보낸 프레임(bytes): \n");
    print_frame(hdlc_frame, total_length);
    
}

int get_current_hour(){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    return tm.tm_hour;
}

int get_current_min(){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    return tm.tm_min;
}

int get_current_sec(){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    return tm.tm_sec;
}

void print_current_time(){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("[%02d:%02d:%02d] ", tm.tm_hour, tm.tm_min, tm.tm_sec);
}

char* chat_receive(unsigned int* length)
{    
    static char data[MAX_SIZE];
    *length = recvfrom(rcvsock, data, MAX_SIZE, 0, (struct sockaddr*)&r_addr, (unsigned int *)&clen);
    return data;  
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

void print_titlebar(const char* selectedMenu) {
    int headerWidth = 14;  // 상단바의 폭

    // 상단바 헤더 출력
    printf("╔");
    for (int i = 0; i < headerWidth - 2; i++) {
        printf("═");
    }
    printf("╗\n");

    // 선택된 메뉴 출력
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

    // 상단바 구분선 출력
    printf("╠");
    for (int i = 0; i < headerWidth - 2; i++) {
        printf("═");
    }
    printf("╣\n");
}


