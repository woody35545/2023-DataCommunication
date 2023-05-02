#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#define MAX_SIZE 300 //전송할 Data 사이즈를 300으로 선언
/*Flag에서 사용할 메크로 변수*/

#define PASS		0 
#define L1_FAIL		1
#define L2_FAIL 	2
#define L1_L2_FAIL	3

int ssock; //소켓 변수
int clen; //sender 주소의 크기	
int Error_Flag; // sender와 receive의 목적지 주소를 비교하여 다른 부분이 어딘지 알려주는 변수
struct sockaddr_in server_addr; //소켓프로그램에 내장된 구조체

struct L1 {
	int saddr[6]; //L1계층 소스 어드레스
	int daddr[6]; //L1계층 목적지 주소
	int length; //길이 나타내는 변수
	int seq; //seq를 나타내는 변수
	char L1_data[MAX_SIZE]; //L1 계층에 보낼 메시지 저장
}; //L1 계층을 나타내는 구조체

struct L2 {
	int saddr[4]; //L2계층 소스 어드레스
	int daddr[4]; //L2계층 목적지 어드레스
	int length; //길이 나타내는 변수
	char L2_data[MAX_SIZE]; //L1 계층에 보낼 메시지 저장
}; //L1 계층을 나타내는 구조체
struct L1 L1_Data; //L1계층의 시작주소와 목적지주소를 비교하기 위한 구조체 선언
struct L2 L2_Data; //L2계층의 시작주소와 목적지주소를 비교하기 위한 구조체 선언
/*서버로부터 받은 메시지를 각 계층별로 받아서 풀어나가는 함수 선언*/
char *L1_receive(int *);
char *L2_receive(int *);
char *L3_receive(int *);

void setSocket(void); //소켓생성 함수

int main (void){
	char output[MAX_SIZE]; //들어올 메세지
	int length; //길이 변수
	setSocket(); //소켓생성함수
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
			printf("Received : ");
			puts(output);
			printf("\n");
		}
		if(!strcmp(output,"exit")) break; //들어올 메시지가 ‘exit' 비교후 exit라면 프로그램 종료
	}
	close(ssock); //소켓 닫음
} //exit가 받아올 때 까지 반복해서 메시지 받음

char * L1_receive(int *length){
	static int seq; //정적변수 선언
	struct L1 *data; //data변수 선언
	int i; //for문을 위한 변수 선언
	/*L1의 시작주소와 목적지주소를 정의*/
	L1_Data.saddr[0] = 11;
	L1_Data.saddr[1] = 22;
	L1_Data.saddr[2] = 33;
	L1_Data.saddr[3] = 44;
	L1_Data.saddr[4] = 55;
	L1_Data.saddr[5] = 66;
	L1_Data.daddr[0] = 12;
	L1_Data.daddr[1] = 23;
	L1_Data.daddr[2] = 34;
	L1_Data.daddr[3] = 45;
	L1_Data.daddr[4] = 56;
	L1_Data.daddr[5] = 67;

	data = (struct L1*)L2_receive(length); //L2에서 받은 길이 만큼 L1 data에 저장                                                      
	*length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr) - sizeof(data->seq); //받은 데이터의 길이를 계산하여 저장

	for (i = 0; i <= 100; i++){ //sender에서 발생된 난수 찾기                                  
		if (data->seq == i)
			seq = i;
	}
	if(data->seq == seq) //sender에서 받아온 seq와 값이 같으면 seq값 출력
		printf ("Expected_sequence  : %d\n", data->seq);
	for(i = 0;  i < 6;  i++){ //sender에서 보내온 목적주소와 receiver의 목적주소를 비교하는 반복문
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
	L2_Data.saddr[0] = 192;
	L2_Data.saddr[1] = 168;
	L2_Data.saddr[2] = 30;
	L2_Data.saddr[3] = 33;
	L2_Data.daddr[0] = 191;
	L2_Data.daddr[1] = 167;
	L2_Data.daddr[2] = 31;
	L2_Data.daddr[3] = 33;
	data = (struct L2*)L3_receive(length); //L3에서 받은 길이 만큼 L2 data에 저장
	*length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr); //받은 데이터의 길이를 계산하여 저장

	for(i = 0; i < 4;  i++){ //sender에서 보내온 목적주소와 receiver의 목적주소를 비교하는 반복문
		if(L2_Data.daddr[i] != data->daddr[i]) //L2의 목적지 주소가 다를 경우
			Error_Flag = L2_FAIL; 
	}
	return (char *)data->L2_data; //데이터만 추출한 것을 L1에 보냄
}

char *L3_receive(int *length){
	static char data[MAX_SIZE]; //정적 변수 선언
	int i=0; //0번째 방부터 하나씩 확인	

	if((i=recvfrom(ssock, data, MAX_SIZE, 0,(struct sockaddr *)&server_addr, &clen)) <= 0){
		perror("read error : ");
		exit(1);
	}
	*length = i; //받아온 길이만큼 length에 저장
	return data; //데이터 보냄
}

void setSocket(){ //소켓 생성
	if((ssock=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0) {
		perror("socket error : ");
		exit(1);
	}
	clen = sizeof(server_addr);
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	server_addr.sin_port = htons(9658);
	if(bind(ssock,(struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("bind error : ");
		exit(1);
	}
}