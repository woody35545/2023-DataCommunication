#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <openssl/des.h>
#include <malloc.h>
// Constants: 매크로 선언
#define BUFFER_SIZE 256
#define IP_ADDRESS "127.0.0.1"
// 8byte char 비교
#define KEY "test123!"
#define MAX_CHALLENGE_VALUE 10000
// Structs : 사용할 구조체 선언
struct Auth {
    int type;
    char userID[100];
    int challenge;
    int response;
};
// 함수 선언 및 변수 선언
void send_auth(const struct Auth *auth_ptr);
void receive_auth(struct Auth *auth_ptr);
void init_socket();
void check_is_server(char *const *argv);
int encrypt_des(char *key, int value);
int sndsock, rcvsock;
unsigned int addr_len;
struct sockaddr_in s_addr, r_addr;
static char buffer[BUFFER_SIZE];
int is_server = 0;
struct Auth authData; // local auth data for client 전역 설정
// Main
int main(int argc, char *argv[]) {    
    srand(time(NULL));
    check_is_server(argv);
    init_socket();    
    if (is_server == 1) { //Server
        while (1) {
            receive_auth(&authData);
            if (authData.type == 0) {
                // Generate challenge, type update, call send_auth
                /*authData.type = ??;
                authData.challenge = ??;
                send_auth(??);*/
            } else if (authData.type == 2) {
                // encrypt authData.challenge using DES and check it's same as authData.response
                /*int calculated_response = encrypt_des(??);
                int received_response = ??;
				printf("Server response = %d, Client response = %d\n", ??, ??);

				// server response == client response 값 비교
				if (?? == ??) {
                    printf("true\n");
                } else {
                    printf("false\n");
                }*/
            }
        }
    } else { //Client
        // Type 0 - Login
        printf("Enter your User ID: ");        
        scanf("%s", authData.userID);
		/*
		authData.type = ??;
        authData.challenge = ??;
        authData.response = ??;
        send_auth(??);
		*/
        // Type 2 - Send response
		/*
        receive_auth(??);
        authData.response = ??;
        authData.type = ??;
        send_auth(??);
		*/
    }
    return 0;
}

void send_auth(const struct Auth *auth_ptr) {
    printf("Sent [TYPE: %d] ID: %s, CHALLENGE: %d, RESPONSE: %d\n", auth_ptr->type, auth_ptr->userID,
           auth_ptr->challenge, auth_ptr->response);
    long length = sendto(sndsock, auth_ptr, sizeof(struct Auth), 0, (struct sockaddr *) &s_addr, sizeof(s_addr));
    if (length < 0) {
        fprintf(stderr, "Send Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void receive_auth(struct Auth *auth_ptr) {
    long length = recvfrom(rcvsock, &buffer, BUFFER_SIZE, 0, (struct sockaddr *) &r_addr, &addr_len);
    if (length < 0) {
        fprintf(stderr, "Recv Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    *auth_ptr = *(struct Auth *) buffer;
    printf("Received [TYPE: %d] ID: %s, CHALLENGE: %d, RESPONSE: %d\n", authData.type, authData.userID,
           authData.challenge, authData.response);
}

void check_is_server(char *const *argv) {
    // Check is server or client
    char *progname = basename(argv[0]);
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pgrep -x %s | wc -l", progname);
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("popen");
        exit(EXIT_FAILURE);
    }
    int process_count = 0;
    char line[256];
    if (fgets(line, sizeof(line), fp) != NULL) {
        process_count = atoi(line);
    }
    pclose(fp);
    is_server = process_count == 1;
}
// DES 암호화 진행 (des.h로 분리 가능)
int encrypt_des(char *key, int value) {
    DES_key_schedule schedule;
    DES_set_key_unchecked((const_DES_cblock *) key, &schedule);

    DES_cblock input_block, output_block;
    memcpy(input_block, &value, sizeof(value));
    DES_ecb_encrypt(&input_block, &output_block, &schedule, DES_ENCRYPT);
    int ciphertext;
    memcpy(&ciphertext, output_block, sizeof(ciphertext));
    return ciphertext;
}
void init_socket() {	
	// 실행중인 소켓 확인후 첫번째 실행 세션에 서버 임무 부여
	// 실행중인 소켓 확인후 두번째 실행 세션에 클라이언트 임무 부여
    int send_port;
    int receive_port;
    if (is_server == 0) {
        printf("Client starting...\n");
        send_port = 8811;
        receive_port = 8810;
    } else {
        printf("Server starting...\n");
        send_port = 8810;
        receive_port = 8811;
    }
    // Create Send Socket
    if ((sndsock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket error : ");
        exit(1);
    }
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    s_addr.sin_port = htons(send_port);
    if (connect(sndsock, (struct sockaddr *) &s_addr, sizeof(s_addr)) < 0) {
        perror("connect error : ");
        exit(1);
    }
    // Create Receive Socket
    if ((rcvsock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket error : ");
        exit(1);
    }

    addr_len = sizeof(r_addr);
    memset(&r_addr, 0, sizeof(r_addr));

    r_addr.sin_family = AF_INET;
    r_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    r_addr.sin_port = htons(receive_port);
    if (bind(rcvsock, (struct sockaddr *) &r_addr, sizeof(r_addr)) < 0) {
        perror("bind error : ");
        exit(1);
    }
    int optvalue = 1;
    int optlen = sizeof(optvalue);
    setsockopt(sndsock, SOL_SOCKET, SO_REUSEADDR, &optvalue, optlen);
    setsockopt(rcvsock, SOL_SOCKET, SO_REUSEADDR, &optvalue, optlen);
    printf("####################################################\n");
    printf("[READY]sndsock port: %d, rcvsock port: %d \n", send_port, receive_port);
    printf("[READY]IP: %s \n", IP_ADDRESS);
    printf("[READY]key: %s \n", KEY);
    printf("####################################################\n");
    ///////////////////////////////////////////////////////////////////
}