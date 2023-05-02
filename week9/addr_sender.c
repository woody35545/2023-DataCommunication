#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <time.h> //랜덤함수를 사용하기 위한 헤더파일

#define MAX_SIZE 300 //전송할 Data 사이즈를 300으로 선언

char saddr_ip_L1[100], daddr_ip_L1[100]; //문자열로 L1 의 시작주소와 목적지주소를 받기위한 배열 선언
char saddr_ip_L2[100], daddr_ip_L2[100]; //문자열로 L2 의 시작주소와 목적지주소를 받기위한 배열 선언
int ssock;
struct sockaddr_in server_addr;

struct L1 {
	int saddr[6];
	int daddr[6]; //목적지주소
	int length;
	int seq; //seq를 나타내는 변수
	char L1_data[MAX_SIZE];
}; //L1 계층을 나타내는 구조체

struct L2 {
	int saddr[4];
	int daddr[4];
	int length;
	char L2_data[MAX_SIZE];
}; //L2 계층을 나타내는 구조체

/*보낼 메시지를 받는 함수 선언*/
void L1_send(char *input, int length);
void L2_send(char *input, int length);
void L3_send(char *data, int length);
void setSocket(void); //소켓 생성

int main (void){	
	char input[MAX_SIZE]; //보낼 메세지
	int length; //길이 변수
	int button; //switch문을 위한 변수 
	setSocket(); //소켓 생성
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
			printf("Input my L1 address : ");
			scanf("%s", saddr_ip_L1); 
			__fpurge(stdin);
			printf("Input my dest L1 address : ");
			scanf("%s", daddr_ip_L1);
			break;
			//L1의 시작 주소와 목적지 주소를 입력받는 부분.
		case  2:
			__fpurge(stdin);
			printf("Input my L2 address : ");
			scanf("%s", saddr_ip_L2); 
			__fpurge(stdin);
			printf("Input my dest L2 address : ");
			scanf("%s", daddr_ip_L2);
			break;
			//L2의 시작 주소와 목적지 주소를 입력받는 부분.
		case  3: 
			__fpurge(stdin);
			printf("put the message: ");
			gets(input);
			L1_send(input, strlen(input));
			//전송할 문자를 입력받는 부분.
			break;
		default:
			printf("Error!!!!!!!!!\n");
			break;
			//예외처리 구문
		}
		if(!strcmp(input,"exit"))
			break; //입력한 메시지랑 ‘exit' 비교후 ’exit' 이면 프로그램 종료
	}
	close(ssock); //소켓 닫음
}

void L1_send(char *input, int length){
	struct L1 data; //구조체 변수 선언
	char temp[350];
	int size = 0; //초기화
	int i;
	char *token;
	static int seq ; //정적변수 선언
	srand((unsigned)time(NULL)); //난수의 seed값을 초기화
	seq = rand()%100 + 1; //0부터 100까지의 난수 생성
	token = strtok(saddr_ip_L1, "-");//문자열에서 '-'를 기준으로 문자열을 나눈다.
	i = 0;

	while (token != NULL){
		data.saddr[i] = atoi(token);
		token = strtok(NULL, "-");
		i++;
	} //문자열에서 나눈 데이터들을 integer값으로 변환하여 L1의 시작주소에 저장
	token = strtok(daddr_ip_L1, "-"); //문자열에서 '-'를 기준으로 문자열을 나눈다.
	i = 0;

	while (token != NULL){
		data.daddr[i] = atoi(token);
		token = strtok(NULL, "-");
		i++;
	} //문자열에서 나눈 데이터들을 integer값으로 변환하여 L1의 목적지주소에 저장

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
	struct L2 data; //구조체 변수 선언
	char temp[350];
	int size = 0; //초기화
	int i;
	char *token;
	token = strtok(saddr_ip_L2, "."); //문자열에서 '.'를 기준으로 문자열을 나눈다.
	i = 0;

	while (token != NULL){
		data.saddr[i] = atoi(token);
		token = strtok(NULL, ".");
		i++;
	} //문자열에서 나눈 데이터들을 integer값으로 변환하여 L2의 시작주소에 저장

	token = strtok(daddr_ip_L2, "."); //문자열에서 '.'를 기준으로 문자열을 나눈다.
	i = 0;

	while (token != NULL){
		data.daddr[i] = atoi(token);
		token = strtok(NULL, ".");
		i++;
	} //문자열에서 나눈 데이터들을 integer값으로 변환하여 L2의 목적지주소에 저장

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
	if(sendto (ssock, temp, length, 0, (struct sockaddr *)&server_addr,sizeof(server_addr)) <= 0){
		perror("write error : ");
		exit(1);
	}
}

void setSocket(){ //소켓 생성 
	if((ssock=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0) {
		perror("socket error : ");
		exit(1);
	}
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
	server_addr.sin_port = htons(9658);
}