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
            unsigned char ip[] = {192, 168, 64, 7}; // 과제 진행했던 vm의 ip로 할당
            for (int i = 0; i < sizeof(ip); i++)
            // 서버는 전역변수(addr)에 서버 자신의 주소를 저장 
            // client에게 주소를 전달해주기 위해 addrData.ip에 server ip 값 할당
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
			// 형식상 맞춰줌, Find Address 모드에서는 사용하지 않음. 임의의 값으로 초기화
			
            /* 구현. IP 주소 헤더에 붙임 */
            /* Addr 구조체 addrData에 ip 부분에 server의 ip를 할당한후 L1_data에 할당해서 L2로 전송 */
            memset(data.L1_data, 0x00, MAX_SIZE); // L1_data에 할당하기 전에 L1_data 메모리값 초기화
            memcpy(data.L1_data, &addrData, sizeof(addrData)); // L1_data에 addrData 시작주소부터 addrData의 size만큼 복사

            size = sizeof(struct L1) - sizeof(data.L1_data) + sizeof(addrData); // 실제로 L1 struct에 할당되는 크기 계산
            memset(temp, 0x00, 350); // L2에 넘겨줄 데이터를 담을 temp 초기화
            memcpy(temp, (void *)&data, size); // temp 에 L1의 패킷 정보를 담고있는 data(header+L1_data)변수를 앞서 계산했던 L1에 실제로 할당한 size 만큼 복사
            /*----------------------*/

            L2_send(temp, size); // 다음 Layer인 L2로 temp 전달
            
        }
    }
    else if (control.type == 1) // Not Find_Addr Mode[L1]
    {		
		
		for (int i = 0; i < sizeof(addr.ip); i++) 
		{
            data.daddr[i] = addr.ip[i]; // 데이터 값 대입
        }
	
        data.saddr[0] = 0x00;
        data.saddr[1] = 0x00;
        data.saddr[2] = 0x00;
        data.saddr[3] = 0x00;
		// [!]도착지 주소 생략 가능 -> 위에서 서버 ip로 도착지 주소를 설정해주었음
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
        if (is_server == 0) // client
        {
            // 서버로 Address Request 하는 부분
            // L2 header에 있는 주소 필드(saddr, daddr)를 0으로 초기화, 가독성을 위해 기존코드에서 for문으로 변경
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
            // L1 payload(=L1.L1_data)에 있는 Addr.ip 값을 addrData.ip에 복사해서 L2의 payload(L2.L2_data)에 할당
            struct L1* tempL1 = (struct L1 *)input; // input을 L1 struct pointer로 casting 후 tempL1에 할당
            struct Addr* tempAddr = (struct Addr *)tempL1->L1_data;  // L1_data에 담긴 주소를 가져오기 위해 tempL1->L1_data를 Addr struct Pointer로 캐스팅하고 tempAddr에 할당
            memcpy(addrData.ip, tempAddr->ip, sizeof(tempAddr->ip)); // tempAddr(L1_data.ip의 시작주소)를 addrData.ip로 복사

            // 6e:d4:8d:08:5e:e9 -> 과제를 진행했던 vm의 MAC Address
            // 과제의 요구사항에 따라 서버의 주소는 사전에 정의해서 사용하도록 구현
            unsigned char mac[] = {0x6e, 0xd4, 0x8d, 0x08, 0x5e, 0xe9}; // 과제 진행했던 vm의 mac 주소로 선언
            printf("[%s] my MAC --> ", __func__);
            for (int i = 0; i < 6; i++)
            {
				/* 구현. addrData.mac 과 addr에 각각 mac 주소 할당 */ 
                addrData.mac[i] = mac[i]; // 전송할 주소를 담을 Addr 구조체 변수 addrData.mac에 서버의 MAC 주소를 할당
				addr.mac[i]= mac[i]; // 전역변수 addr.mac에 mac값 할당

                // hint. 아래는 출력문임 
                printf("%02X", addrData.mac[i]);
                if (i != 5)
                    printf(":");
            }
            printf("\n");

            // addr값 0x00으로 초기화, 코드 가독성을 위해 기존 코드를 for문으로 변경하여 초기화하였음.
			for(int i=0; i<6; i++) {
                data.saddr[i]= 0x00;
                data.daddr[i]= 0x00;
            }
    
            data.length = length;
			
			/* 구현. memset, cpy */ 
            addrData.type = 1; // response 이므로 1로 설정

            memset(data.L2_data, 0x00, MAX_SIZE); // L2 struct의 L2_data 영역 초기화
            memcpy(data.L2_data, &addrData, sizeof(addrData)); // addrData를 data.L2_data에 복사해서 넣어줌
                    
            size = sizeof(struct L2) - sizeof(data.L2_data) + length; // L2 사이즈(size)를 L2에 실제 할당된 크기만큼 다시 계산해서 할당

            memset(temp, 0x00, 350); // L2 패킷 전체를 담을 temp 초기화
            memcpy(temp, (void *)&data, size); // L2 전체를 앞서 다시 계산한 size만큼 temp에 복사

            L3_send(temp, size); // L2 전체를 복사한 temp를 다음 Layer인 L3로 전송
        }
    }
    else if (control.type == 1) // 채팅 모드
    {
        for (int i=0; i< 6; i++) {data.daddr[i] = 0x00;} // 전송할 destination MAC 주소를 의미하는 data.daddr를 초기화
        
    	
        for (int i = 0; i < 6; i++) 
		{			
			// 구현. 데이터 값 대입
            // Find Address 모드에서 server로부터 받아 전역변수 addr에 저장했던 server의 MAC 주소를 destination MAC Address로 할당.
            data.daddr[i] = addr.mac[i]; 
        }
        
        data.length = length;
        memset(data.L2_data, 0x00, MAX_SIZE); // L2_data에 할당하기전에 L2_data를 0x00로 초기화
        memcpy(data.L2_data, (void *)input, length); // 채팅 모드에서 사용자의 입력으로 들어온 input을 L2_data에 할당

        size = sizeof(struct L2) - sizeof(data.L2_data) + length; // 실제 L2에 할당한 크기로 size 갱신

        memset(temp, 0x00, 350); // temp 0x00으로 초기화
        memcpy(temp, (void *)&data, size); // L2 패킷을 앞서 계산한 size 만큼 temp에 할당
        L3_send(temp, size); // 다음 Layer인 L3로 temp 전송
    }
}

void L3_send(char *data, int length)
{
    // socket 통해 전송
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
        char str_ip[16]; // my ip
        char str_daddr[16]; // received ip

        int cur =0;

        /* 구현 . sprintf(??) */
        // sprint 함수를 이용하여 전역변수 addr에 저장했던 서버자신의 주소를 str_ip에 IP 표현 형식(x.x.x.x)에 맞게 할당
        for(int i =0; i<4; i++) {
            if(i==3) cur += sprintf(str_ip+cur, "%d" ,addr.ip[i]); else cur += sprintf(str_ip+cur, "%d." ,addr.ip[i]);}

        
        // sprint 함수를 이용하여 data->daddr을 decimal 형태로 읽어서 str_daddr에 IP 표현 형식(x.x.x.x)으로 저장
        cur = 0;
        for(int i =0; i<4; i++) {
            if(i==3) cur += sprintf(str_daddr+cur, "%d" ,data->daddr[i]); else cur += sprintf(str_daddr+cur, "%d." ,data->daddr[i]);}

        // 서버측에서 주소 검증
        if(is_server == 1){ // 서버의 경우에만 검증을 수행하도록 구현
            /* receive한 destination IP 주소 출력 */
            //printf("receive my IP --> %s\n", str_daddr); 
            
            /* server의 ip 주소 출력 */
            //printf("str_ip: %s\n", str_ip);

            int result = strcmp(str_daddr, str_ip); // 주소 비교
            
            if (result == 0) {
                /* ip 주소가 동일할 경우 출력문	*/
                // printf("daddr is equal to %s\n",str_ip); 	
                *length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr);
                return (char *)data->L1_data;
        } else {
            printf("daddr is not equal to %s\n",str_ip); // ip 주소가 다를 경우 출력문					
            }       
        } 
        else { // 클라이언트의 경우 검증 수행하지 않고 채팅기능만 수행하도록 구현
            *length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr);
            return (char *)data->L1_data; 
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
            // 구현. 데이터 파싱과 addr.mac에 값 대입 
			// data =   *length =     addrData = 
            data = (struct L2 *)L3_receive(length); // L3 에서 리턴한 L2 패킷 정보를 L2 struct pointer로 casting 하여 data에 할당
            
            /* Find Address 모드에서는 server가 response 할 때 Addr 구조체에 자신의 MAC 주소를 담은 후, Addr 구조체 전체를 L2_data에 넣어서 전송하므로
            L2_receive에서 클라이언트는 받은 패킷에서 서버가 전송한 mac 주소을 가져오기 위해 L2_data를 Addr pointer로 casting한 addrData 선언 */ 
            struct Addr* addrData = (struct Addr*) data->L2_data;

            // L2 header 제외한 크기로 갱신
            *length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr); 
            
            // 서버로부터 전달받은 MAC 주소 출력
            printf("[%s] your MAC --> ", __func__);
            for(int i=0; i<6; i++){
                if(i==5) printf( "%02X", addrData->mac[i]);
                else{
                printf("%02X:", addrData->mac[i]);
                } 
            }
            printf("\n");
            for(int i=0; i<6; i++){
                addr.mac[i] = addrData->mac[i]; // 전역변수 addr.mac에 server로부터 받은 서버 MAC 주소 할당
            }
        
            return (char *)data->L2_data; 
        }
    }
    else if (control.type == 1)
    {
        // 구현. L1_rev 참고하여 구현
        data = (struct L2 *)L3_receive(length); // data는 L3_receive가 리턴한 L2 데이터를 접근하기 위한 L2 struct pointer 변수

		char mac[18]; // 검증을 위해 서버 mac 주소를 담을 변수
		char str_daddr[18]; // received mac을 문자열 형식으로 담기 위한 변수, 주소 검증시 사용.
        
        // sprint 함수를 이용하여 전역변수 addr에 저장했던 서버 자신의 주소를 mac 변수에 MAC 주소 표현 형식(XX:XX:XX:XX:XX:XX)에 맞게 할당되도록 구현
        int cur = 0;
        for(int i =0; i<6; i++) {
            if(i==5) cur += sprintf(mac+cur, "%02X" , addr.mac[i]); else cur += sprintf(mac+cur, "%02X:" , addr.mac[i]);}

        // sprint 함수를 이용하여 client로부터 받은 패킷에서 MAC 주소를 16진수로 읽어 str_daddr변수에 MAC 주소 표현 형식(XX:XX:XX:XX:XX:XX)에 맞게 할당되도록 구현
        cur = 0;
        for(int i =0; i<6; i++) {
            if(i==5) cur += sprintf(str_daddr+cur, "%02X" ,data->daddr[i]); else cur += sprintf(str_daddr+cur, "%02X:" ,data->daddr[i]); }

        // 검증 시각화를 위한 출력문, 서버의 경우만 검증 수행하도록 구현
        if(is_server == 1){
                /* receive destination MAC 주소 출력 */
                //printf("receive my MAC --> %s\n", str_daddr); 
                
                /* server의 mac 주소 출력 */
                //printf("mac: %s\n", mac); 
                    
            int result = strcmp(str_daddr, mac); // 주소 비교

            if(result==0){			
                // destination MAC 주소와 서버의 MAC 주소가 동일한 경우 출력문
                // printf("daddr is equal to %s\n",mac);
                *length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr);
                return (char *)data->L2_data;
            }else{
                printf("daddr is not equal to %s\n",mac); // destination MAC 주소와 서버의 MAC 주소가 동일하지 않은 경우 출력문
            }           
        } else { // 클라이언트의 경우 검증은 수행하지 않고 채팅기능만 수행하도록 구현
            *length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr); 
            return (char *)data->L2_data;}
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