#define _CRT_SECURE_NO_WARNINGS
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
#define IP_ADDRESS "127.0.0.1"
#define MAC_ADDRESS "76:04:A0:7E:8F:A1"

struct L1
{
    int saddr[4];
    int daddr[4];
    int length;
    char L1_data[MAX_SIZE];
};
struct L2
{
    int saddr[6];
    int daddr[6];
    int length;
    char L2_data[MAX_SIZE];
};
struct Control
{
    int type; // 1: chat 2: find adress    
};
struct Addr
{
    int type;
    char ip[4];
    char mac[6];
};

void L1_send(char *input, int length);
void L2_send(char *input, int length);
void L3_send(char *data, int length);
char *L1_receive(int *);
char *L2_receive(int *);
char *L3_receive(int *);

int sndsock, rcvsock, clen;
struct sockaddr_in s_addr, r_addr;
void *do_thread(void *);

void init_socket();
void initAdress();

void check_is_server(char *const *argv);
int is_server = 0;

struct Control control;
struct Addr addr;


int main(int argc, char *argv[])
{

    check_is_server(argv);
    init_socket();
    initAdress();
    char input[MAX_SIZE];

    int length;
    pthread_t t_id;
    int status = pthread_create(&t_id, NULL, do_thread, NULL);
    if (status != 0)
    {
        printf("Thread Error!\n");
        exit(1);
    }
	while (1)	{
        fgets(input, sizeof(input), stdin);	
		L1_send(input, strlen(input));      
	}
	
    
}

void initAdress()
{

    control.type = 2;
    char output[MAX_SIZE];
    int length;

    if (is_server == 0)
    {
        // client

        printf("\033[0;31m[Find Adress Mode - Request ...]\n\033[0m");
        addr.type = 0;
        L1_send("", strlen(""));

        strcpy(output, L1_receive(&length));
        output[length] = '\0';        
        printf("\033[0;33m[Mode Change]\n\033[0m");
        control.type = 1;
    }
    else
    {
        // server
        strcpy(output, L1_receive(&length));
        output[length] = '\0';
        L1_send("", strlen(""));

        //printf("[Find Adress Mode - Success ...]\n");
		printf("\033[0;33m[Find Adress Mode - Success ...]\n\033[0m");
        control.type = 1;
    }
}

void *do_thread(void *arg)
{
    char output[MAX_SIZE];
    int length;
    while (1)
    {
        if (control.type == 1) // Not Find_Addr Mode
        {
            strcpy(output, L1_receive(&length));
            output[length] = '\0';
            if (strlen(output) > 1)
            {
                printf("Received: %s", output);
            }
        }
    }
    return NULL;
}

void L1_send(char *input, int length)
{
    struct L1 data;
    struct Addr addrData;
	char temp[350];
    int size = 0;
    if (control.type == 2) // Find_Addr Mode
    {
        if (is_server == 0)
        {
            // client 
            data.saddr[0] = 0x00;
            data.saddr[1] = 0x00;
            data.saddr[2] = 0x00;
            data.saddr[3] = 0x00;

            data.daddr[0] = 0x00;
            data.daddr[1] = 0x00;
            data.daddr[2] = 0x00;
            data.daddr[3] = 0x00;
			
            data.length = length;
            memset(data.L1_data, 0x00, MAX_SIZE);
            strcpy(data.L1_data, input);

            size = sizeof(struct L1) - sizeof(data.L1_data) + length;
            memset(temp, 0x00, 350);
            memcpy(temp, (void *)&data, size);

            L2_send(temp, size);
			// 형식상 맞춰줌
        }
        else
        {
            // server
            printf("[%s] my IP --> ", __func__);
            unsigned char ip[] = {192, 168, 64, 5}; // 서버 ip
            for (int i = 0; i < sizeof(ip); i++)
            // addrData.ip에 server ip 값 할당
            {
                addrData.ip[i] = ip[i];
				addr.ip[i]= ip[i];
                printf("%hhu", addrData.ip[i]);
                if (i != 3)
                    printf(".");
            }
            printf("\n");
            data.saddr[0] = 0x00;
            data.saddr[1] = 0x00;
            data.saddr[2] = 0x00;
            data.saddr[3] = 0x00;

            data.daddr[0] = 0x00;
            data.daddr[1] = 0x00;
            data.daddr[2] = 0x00;
            data.daddr[3] = 0x00;
			// 형식상 맞춰줌
			
            /* 구현. IP 주소 헤더에 붙임 - 확인 필요 */
            memset(data.L1_data, 0x00, MAX_SIZE);
            memcpy(data.L1_data, &addrData, sizeof(addrData));

            size = sizeof(struct L1) - sizeof(data.L1_data) + sizeof(addrData); 
            memset(temp, 0x00, 350);
            memcpy(temp, (void *)&data, size);
            /*----------------------*/

            L2_send(temp, size);			 
            
        }
    }
    else if (control.type == 1) // Not Find_Addr Mode[L1]
    {		
		
		for (int i = 0; i < sizeof(addr.ip); i++) 
		{
			// printf("%hhu", addr.ip[i]);
			// if (i != 3) 	printf(".");
		
            data.daddr[i] = addr.ip[i]; // 데이터 값 대입 [!]
		
        }


	
        data.saddr[0] = 0x00;
        data.saddr[1] = 0x00;
        data.saddr[2] = 0x00;
        data.saddr[3] = 0x00;
		// [!]도착지 주소 생략 가능
		/*
        data.daddr[0] = 0x44;
        data.daddr[1] = 0x44;
        data.daddr[2] = 0x44;
        data.daddr[3] = 0x44;
		*/

		// 나머지 코드는 동일함
        data.length = length;
        memset(data.L1_data, 0x00, MAX_SIZE);
        strcpy(data.L1_data, input);
        size = sizeof(struct L1) - sizeof(data.L1_data) + length;
        memset(temp, 0x00, 350);
        memcpy(temp, (void *)&data, size);
        L2_send(temp, size);
    }

}

void L2_send(char *input, int length)
{
    struct L2 data;
    struct Addr addrData;

    char temp[350];
    int size = 0;
    if (control.type == 2)
    {
        if (is_server == 0) // client, request address to server
        {
            // 0으로 초기화
            for(int i=0; i<6; i++){ data.saddr[i] = 0x00; data.daddr[i] = 0x00; }

            data.length = length;
            memset(data.L2_data, 0x00, MAX_SIZE);
            memcpy(data.L2_data, (void *)input, length);

            size = sizeof(struct L2) - sizeof(data.L2_data) + length;

            memset(temp, 0x00, 350);
            memcpy(temp, (void *)&data, size);
            
            L3_send(temp, size);
        }
        else
        {

            // L1 payload에 있는 Addr.ip 값을 L2의 payload에 담을 addrData.ip에 복사해서 넣어줌
            struct L1 *tempL1 = (struct L1 *)input; // input -> struct L1 
            // tempAddr는 인자로 받은 L1.L1_data를 가리킴
            struct Addr *tempAddr = (struct Addr *)tempL1->L1_data; 
            // tempAddr(L1_data.ip의 시작주소)부터 addrData.ip의 size 만큼 addrData.ip로 복사
            memcpy(addrData.ip, tempAddr->ip, sizeof(addrData.ip)); 

            //76:04:a0:7e:8f:a1
            /*test*/
            unsigned char mac[] = {0x76, 0x04, 0xa0, 0x7e, 0x8f, 0xa1};
            printf("[%s] my MAC --> ", __func__);
            for (int i = 0; i < 6; i++)
            {
				/* 구현. addrData.mac 과 addr에 각각 mac[]의 값 할당 */ 
                // 확인 필요
                addrData.mac[i] = mac[i];
				addr.mac[i]= mac[i];
                /*----------------------------------------------*/
				
                // hint. 아래는 출력문임 
                printf("%02X", addrData.mac[i]);
                if (i != 5)
                    printf(":");
            }
            printf("\n");

            // addr값 0x00으로 초기화
			for(int i=0; i<6; i++) {
                data.saddr[i]= 0x00;
                data.daddr[i]= 0x00;
            }
    
            
            data.length = length;
			
			/*--- 구현. memset, cpy----*/ // 이부분 segmentation fault 발생함
            //memset(addrData.type, 0x00, sizeof(addrData.type));
           // type이 1일 때 reply이므로 1(=0x01)로 설정
            //memcpy(addrData.type, 0x01, 1); 
            
            memset(data.L2_data, 0x00, MAX_SIZE); // L2 struct의 L2_data 영역 초기화
            memcpy(data.L2_data, &addrData, sizeof(addrData)); // addrData를 data.L2_data에 복사해서 넣어줌
            /*-----------------------*/
                    
            size = sizeof(struct L2) - sizeof(data.L2_data) + length;

            memset(temp, 0x00, 350);
            memcpy(temp, (void *)&data, size);

            L3_send(temp, size);
        }
    }
    else if (control.type == 1)
    {
	
        for (int i=0; i< 6; i++) {data.daddr[i] = 0x00;}
    	for (int i = 0; i < 6; i++) 
		{			
			// 구현. 데이터 값 대입
            data.daddr[i] = addr.mac[i];
        }
        
        data.length = length;
        memset(data.L2_data, 0x00, MAX_SIZE);
        memcpy(data.L2_data, (void *)input, length);

        size = sizeof(struct L2) - sizeof(data.L2_data) + length;

        memset(temp, 0x00, 350);
        memcpy(temp, (void *)&data, size);
        L3_send(temp, size);
    }
}

void L3_send(char *data, int length)
{
    sendto(sndsock, data, length, 0, (struct sockaddr *)&s_addr, sizeof(s_addr));
}

char *L1_receive(int *length)
{
    struct L1 *data;
    struct Addr *addrData;
	
    if (control.type == 2)
    {
        data = (struct L1 *)L2_receive(length);         
        if (is_server == 1)
        {
            // server            
			printf("\033[0;31m[Find Adress Mode - Response ...]\n\033[0m");			
            *length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr);
            return (char *)data->L1_data;
        }
        else if (is_server == 0)
        { // client
            addrData = (struct Addr *) data;

            printf("[%s] your IP --> ", __func__);
            for(int i=0; i<4; i++){
                if(i==3) printf( "%d", addrData->ip[i]);
                else{
                printf( "%d.", addrData->ip[i]);
                } 
            }
            printf("\n");

            for(int i=0; i<sizeof(addrData->ip); i++){
                addr.ip[i] = addrData->ip[i];
            } 
            *length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr);
            return (char *)data->L1_data;
        }
    }
    else if (control.type == 1)
    {
        data = (struct L1 *)L2_receive(length);		
		// 편의상 char 형태로  두개의 값을 비교
		
        //char str_ip[16];// my ip
		int my_ip[4] = {192,168,64,5};
        char str_ip[16];
        int cur =0;
        for(int i =0; i<4; i++) {
            if(i==3) cur += sprintf(str_ip+cur, "%d" ,my_ip[i]);
            else cur += sprintf(str_ip+cur, "%d." ,my_ip[i]);
        }

        char str_daddr[16]; // receive ip
        
        // data->daddr을 integer 형태로 읽어서 str_daddr 배열에 저장
        cur = 0;
        for(int i =0; i<4; i++) {
            if(i==3) cur += sprintf(str_daddr+cur, "%d" ,data->daddr[i]);
            else cur += sprintf(str_daddr+cur, "%d." ,data->daddr[i]);
        }
        // printf("my ip: %s\n", str_ip);
        // printf("str_daddr: %s\n", str_daddr);

		/* 구현 . sprintf(??)
        

		sprintf();
		sprintf();		
		
        */

        int result = strcmp(str_daddr, str_ip); // 검증
		if (result == 0) {
            //printf("IP Equal\n");	

			*length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr);
	        return (char *)data->L1_data;
		} else {
			printf("daddr is not equal to %s\n",str_ip);			
		}       
    }
}

char *L2_receive(int *length)
{
    struct L2 *data;
    struct Addr *addrData;
    if (control.type == 2)
    {
        if (is_server == 1)
        {
            // server
            data = (struct L2 *)L3_receive(length);
            *length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr);
            return (char *)data->L2_data;
        }
        else
        {
            // client
            

            // client
            // 구현. 데이터 파싱과 addr.mac에 값 대입 
			// data =   *length =     addrData = 
            /*----------------------------------*/
            data = (struct L2 *)L3_receive(length);

            struct Addr* payload = (struct Addr*) data->L2_data;

            *length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr);
            
            // 전역변수 addr.mac에 받은 mac값 할당
            printf("[%s] your MAC --> ", __func__);
            for(int i=0; i<6; i++){
                if(i==5) printf( "%02X", payload->mac[i]);
                else{
                printf("%02X:", payload->mac[i]);
                } 
            }
            printf("\n");
            for(int i=0; i<6; i++){
                addr.mac[i] = payload->mac[i];
            }
           
            /*----------------------------------*/

            return (char *)data->L2_data;
        }
    }
    else if (control.type == 1)
    {

        data = (struct L2 *)L3_receive(length);
        
        *length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr);

		char mac[18] = MAC_ADDRESS; // my mac
		char str_daddr[18]; // receive mac
         // data->daddr을 integer 형태로 읽어서 str_daddr 배열에 저장
        int cur = 0;
        for(int i =0; i<6; i++) {
            if(i==5) cur += sprintf(str_daddr+cur, "%02X" ,data->daddr[i]);
            else cur += sprintf(str_daddr+cur, "%02X:" ,data->daddr[i]);
        }
        
        //printf("my mac: %s\n", mac);
        //printf("str_daddr: %s\n", str_daddr);
        // 구현. L1_rev 참고하여 구현
        
        //구현완료 전까지 임시로 주석처리
		int result = strcmp(str_daddr, mac);
		if(result==0){			
            //printf("Mac Equal\n");	
			*length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr);
	        return (char *)data->L2_data;
		}else{
			printf("daddr is not equal to %s\n",mac);	
		} 
        
        return (char *)data->L2_data;
    }
}

char *L3_receive(int *length)
{   
    static char data[MAX_SIZE];
    *length = recvfrom(rcvsock, data, MAX_SIZE, 0, (struct sockaddr *)&r_addr, &clen);
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
	is_server = process_count == 1;
	
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
    s_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
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
    r_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
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
    printf("[READY]IP: %s \n", IP_ADDRESS);
    printf("####################################################\n");
			
}
