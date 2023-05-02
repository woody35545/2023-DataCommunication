#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <time.h> //랜덤함수를 사용하기 위한 헤더파일
#include <libgen.h>

#define MAX_SIZE 300 //전송할 Data 사이즈를 300으로 선언
#define PASS 0 
#define L1_FAIL 1
#define L2_FAIL 2
#define L1_L2_FAIL 3
#define IP_ADDRESS "127.0.0.1"

struct L1 {
	int saddr[4];
	int daddr[4]; //목적지주소
	int length;
	int seq; //seq를 나타내는 변수
	char L1_data[MAX_SIZE];
}; //L1 계층을 나타내는 구조체

struct L2 {
	int saddr[6];
	int daddr[6];
	int length;
	char L2_data[MAX_SIZE];
}; //L2 계층을 나타내는 구조체

/*보낼 메시지를 받는 함수 선언*/
void L1_send(char *input, int length);
void L2_send(char *input, int length);
void L3_send(char *data, int length);
/*서버로부터 받은 메시지를 각 계층별로 받아서 풀어나가는 함수 선언*/
char *L1_receive(int *);
char *L2_receive(int *);
char *L3_receive(int *);
/*이전 소스에서는 Addr 구조체를 같이 사용했습니다*/
struct L1 L1_Data; //L1계층의 시작주소와 목적지주소를 비교하기 위한 구조체 선언
struct L2 L2_Data; //L2계층의 시작주소와 목적지주소를 비교하기 위한 구조체 선언
char saddr_ip_L1[100], daddr_ip_L1[100]; //문자열로 L1 의 시작주소와 목적지주소를 받기위한 배열 선언
char saddr_ip_L2[100], daddr_ip_L2[100]; //문자열로 L2 의 시작주소와 목적지주소를 받기위한 배열 선언
int Error_Flag; // sender와 receive의 목적지 주소를 비교하여 다른 부분이 어딘지 알려주는 변수

int sndsock, rcvsock, clen;
struct sockaddr_in s_addr, r_addr;
void *do_thread(void *);
void init_socket();
void check_is_server(char *const *argv);
int is_server = 0;

int main (int argc, char *argv[]){	
	
	check_is_server(argv);
    init_socket();
	
	char input[MAX_SIZE]; //보낼 메세지
	char output[MAX_SIZE]; //들어올 메세지
	int length; //길이 변수
	int button; //switch문을 위한 변수 
	if (is_server == 0){
		// sender
		while (1) {
			/*메뉴 선택문 */ 
			printf("==========Choise Menu===========\n");
			printf("1. Select L1 address\n");
			printf("2. Select L2 address\n");
			printf("3. Send Message\n");
			scanf("%d", &button);

			//입력받은 값에 따라 나누어지는 분기문 
			switch (button){
			case  1:
				__fpurge(stdin);
				printf("Input my L1 address(IP) : ");
				scanf("%s", saddr_ip_L1); 
				__fpurge(stdin);
				printf("Input my dest L1 address(IP) : ");
				scanf("%s", daddr_ip_L1);
				break;
				//L1의 시작 주소와 목적지 주소를 입력받는 부분.
			case  2:
				__fpurge(stdin);
				printf("Input my L2 address(MAC) : ");
				scanf("%s", saddr_ip_L2); 
				__fpurge(stdin);
				printf("Input my dest L2 address(MAC) : ");
				scanf("%s", daddr_ip_L2);
				break;
				//L2의 시작 주소와 목적지 주소를 입력받는 부분.
			case  3: 
				__fpurge(stdin);
				printf("message: ");
			    fgets(input, sizeof(input), stdin);					
				L1_send(input, strlen(input));
				//전송할 문자를 입력받는 부분.
				break;
			default:
				printf("Error!!!!!!!!!\n");
				break;
				//예외처리 구문
			}
				if(!strcmp(input,"exit"))break; //입력한 메시지랑 ‘exit' 비교후 ’exit' 이면 프로그램 종료
		}
	}else{
		// receiver
		while (1) {
		strcpy(output, L1_receive(&length)); //들어올 메시지 복사 후 저장		
		output[length] = '\0'; //저장된 메시지를 널로 초기화
		if(Error_Flag == L1_L2_FAIL){ //L1과 L2의 목적지주소가 다를 경우 
			printf("L1, L2 Address is Not Correct!! \n");
			Error_Flag = PASS;
		}
		else if(Error_Flag == L1_FAIL){ //L1의 목적지주소가 다를 경우 
			printf("L1 Address is Not Correct!! \n");
			Error_Flag = PASS;
		}
		else if(Error_Flag == L2_FAIL){ //L2의 목적지주소가 다를 경우 
			printf("L2 Address is Not Correct!! \n");
			Error_Flag = PASS;
		}
		else{ //L1과 L2의 목적지주소가 같을 경우 
			printf("Length %d\n", length);
			printf("Received: %s", output);
		}
			if(!strcmp(output,"exit")) break; //들어올 메시지가 ‘exit' 비교후 exit라면 프로그램 종료
		}
	}
	
}

void L1_send(char *input, int length){
    printf("call--> [%s]  \n", __func__);
	struct L1 data; //구조체 변수 선언
	char temp[350];
	int size = 0; //초기화
	int i;
	char *token;
	token = strtok(saddr_ip_L1, "."); //문자열에서 '.'를 기준으로 문자열을 나눈다.	
	static int seq ; //정적변수 선언
	srand((unsigned)time(NULL)); //난수의 seed값을 초기화
	seq = rand()%100 + 1; //0부터 100까지의 난수 생성
	while (token != NULL){
		data.saddr[i] = atoi(token);
		token = strtok(NULL, ".");
		i++;
	} //문자열에서 나눈 데이터들을 integer값으로 변환하여 L2의 시작주소에 저장
	i = 0;
	token = strtok(daddr_ip_L1, "."); //문자열에서 '.'를 기준으로 문자열을 나눈다.
	while (token != NULL){
		data.daddr[i] = atoi(token);
		token = strtok(NULL, ".");
		i++;
	} //문자열에서 나눈 데이터들을 integer값으로 변환하여 L2의 목적지주소에 저장

	data.length = length; //받아온 길이를 데이터에 저장
	data.seq = seq; //전송할 seq를 데이터에 저장.
	memset(data.L1_data, 0x00, MAX_SIZE); //L1데이터 메모리 초기화
	strcpy (data.L1_data, input); //입력한 데이터 L2에 복사
	size = sizeof(struct L1) - sizeof(data.L1_data) + length; //내가 입력한 데이터길이만큼 사이즈 지정

	memset(temp, 0x00, 350); //temp의 메모리를 350만큼 0으로 초기화  
	memcpy(temp, (void *)&data, size); //위에서 구한 size만큼 데이터를 temp복사
	L2_send(temp, size); //temp 의 data를 L2로 보냄
	printf ("Sender_sequence  : %d\n", data.seq); //sequence 값 출력
}

void L2_send(char *input, int length){
    printf("call--> [%s]  \n", __func__);
	struct L2 data; //구조체 변수 선언
	char temp[350];
	int size = 0; //초기화
	int i;
	char *token;
	token = strtok(saddr_ip_L2, ":");//문자열에서 ':'를 기준으로 문자열을 나눈다.
	i = 0;

	while (token != NULL){
		data.saddr[i] = atoi(token);
		token = strtok(NULL, ":");
		i++;
	} //문자열에서 나눈 데이터들을 integer값으로 변환하여 L1의 시작주소에 저장
	token = strtok(daddr_ip_L2, ":"); //문자열에서 ':'를 기준으로 문자열을 나눈다.
	i = 0;

	while (token != NULL){
		data.daddr[i] = atoi(token);
		token = strtok(NULL, ":");
		i++;
	} //문자열에서 나눈 데이터들을 integer값으로 변환하여 L1의 목적지주소에 저장

	data.length = length; //받아온 길이를 데이터에 저장
	
	memset(data.L2_data, 0x00, MAX_SIZE); //L1데이터 메모리 초기화
	memcpy (data.L2_data, (void *)input, length); //입력한 데이터 L1에 복사

	size = sizeof(struct L2) - sizeof(data.L2_data) + length; //내가 입력한 데이터길이만큼 사이즈 지정
	memset(temp, 0x00, 350); //temp의 메모리를 350만큼 0으로 초기화
	memcpy(temp, (void *)&data, size); //위에서 구한 size만큼 데이터를 temp복사
	L3_send(temp,size); //temp 의 data를 L3로 보냄
}

void L3_send(char *data, int length){
	char temp[300];
	memset(temp, 0x00, MAX_SIZE); //temp의 메모리를 350만큼 0으로 초기화
	memcpy (temp, (void *)data, length); //위에서 구한 size만큼 데이터를 temp복사
	if(sendto (sndsock, temp, length, 0, (struct sockaddr *)&s_addr,sizeof(s_addr)) <= 0){
		perror("write error : ");
		exit(1);
	}
}

char * L1_receive(int *length){
	static int seq; //정적변수 선언
	struct L1 *data; //data변수 선언

	/*L1의 시작주소와 목적지주소를 정의*/
	//unsigned char ip[] = {192, 168, 0, 1};
	L1_Data.saddr[0] = 192;
	L1_Data.saddr[1] = 168;
	L1_Data.saddr[2] = 30;
	L1_Data.saddr[3] = 1;
	L1_Data.daddr[0] = 192;
	L1_Data.daddr[1] = 168;
	L1_Data.daddr[2] = 0;
	L1_Data.daddr[3] = 1;

	data = (struct L1*)L2_receive(length); //L2에서 받은 길이 만큼 L1 data에 저장                                                      
	*length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr) - sizeof(data->seq); //받은 데이터의 길이를 계산하여 저장

	for (int i = 0; i <= 100; i++){ //sender에서 발생된 난수 찾기                                  
		if (data->seq == i)
			seq = i;
	}
	if(data->seq == seq) //sender에서 받아온 seq와 값이 같으면 seq값 출력
		printf ("Expected_sequence  : %d\n", data->seq);
	for(int i = 0; i < 4;  i++){ //sender에서 보내온 목적주소와 receiver의 목적주소를 비교하는 반복문
		printf("Your IP[%d]%d\n",i,data->daddr[i]);
		if(L1_Data.daddr[i] != data->daddr[i]){ //sender에서 보내온 목적주소와 receiver의 목적주소가 다를 경우 
			if(Error_Flag == L2_FAIL){ //L2에서도 목적지 주소가 다를 경우
				Error_Flag = L1_L2_FAIL;
				i = 99;
			}
			else{ //L1의 목적지 주소가 다를 경우	
				Error_Flag = L1_FAIL;
				i = 99;
			}
		}		
	}

	return (char *)data->L1_data; //main 데이터만 추출한 것을 에 보냄
}

char * L2_receive(int *length){
	struct L2 *data; //data변수 선언
	int i;
	/*L2의 시작주소와 목적지주소를 정의*/
	L2_Data.saddr[0] = 0x11;
	L2_Data.saddr[1] = 0x11;
	L2_Data.saddr[2] = 0x11;
	L2_Data.saddr[3] = 0x11;
	L2_Data.saddr[4] = 0x11;
	L2_Data.saddr[5] = 0x11;
	// MAC{0x01, 0x01, 0x01, 0x11, 0x01, 0x01};
	L2_Data.daddr[0] = 22;
	L2_Data.daddr[1] = 22;
	L2_Data.daddr[2] = 22;
	L2_Data.daddr[3] = 22;
	L2_Data.daddr[4] = 22;
	L2_Data.daddr[5] = 22;
	data = (struct L2*)L3_receive(length); //L3에서 받은 길이 만큼 L2 data에 저장
	*length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr); //받은 데이터의 길이를 계산하여 저장
	for(i = 0;  i < 6;  i++){ //sender에서 보내온 목적주소와 receiver의 목적주소를 비교하는 반복문
		printf("Your MAC[%d]%d\n",i,data->daddr[i]);
		if(L2_Data.daddr[i] != data->daddr[i]) //L2의 목적지 주소가 다를 경우
			Error_Flag = L2_FAIL; 
	}
	
	return (char *)data->L2_data; //데이터만 추출한 것을 L1에 보냄
}

char *L3_receive(int *length){
	static char data[MAX_SIZE]; //정적 변수 선언
	int i=0; //0번째 방부터 하나씩 확인	
	if((i=recvfrom(rcvsock, data, MAX_SIZE, 0,(struct sockaddr *)&r_addr, &clen)) <= 0){
		perror("read error : ");
		exit(1);
	}
	*length = i; //받아온 길이만큼 length에 저장
	return data; //데이터 보냄
}


void check_is_server(char *const *argv){
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