#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_SIZE 300
#define IP_ADDRESS "127.0.0.1"
int sndsock,rcvsock;
int clen;
int IP_condition = 0, MAC_condition = 0; // IP, MAC의 상태를 저장할 변수
char output[MAX_SIZE]; // 전달 받은 메시지를 저장할 변수

int IP[4] = {192, 168, 43, 13};// 고정 IP
int rec_IP[4]; // 전달 받은 IP를 저장할 변수
char MAC[20] = "0A:77:81:9A:52:36";// 고정 MAC
char rec_MAC[20]; // 전달 받은 MAC을 저장할 변수
int type; // type을 저장할 변수
char thread_type = 'r'; // thread_type을 지정할 변수

struct sockaddr_in s_addr,r_addr;

struct L1{
	char rec_MAC[20];
	char own_MAC[20];
	int length;
	int type;
	char L1_data[MAX_SIZE];
}; // 구조체 L1

struct L2{
	int rec_IP[4];
	int own_IP[4];
	int length;
	char L2_data[MAX_SIZE];
}; // 구조체 L2

char *L1_receive(int *);
char *L2_receive(int *);
char *L3_receive(int *);
void L1_send(char *input, int length);
void L2_send(char *input, int length);
void L3_send(char *data, int length);
void setSocket(void);
void *do_thread(void *);

int main (void){
	int length; 
	pthread_t tid; 

	setSocket();
	pthread_create(&tid, NULL, do_thread, (void *)0); // receiver용 thread 생성

	while (1){
		if(thread_type == 'r'){
			memset(output, 0x00, MAX_SIZE);
			strcpy(output, L1_receive(&length));
			output[length] = '\0';

			if((IP_condition == 0) && (type == 1)){
				if(type == 1)
					type = 2;
				if(output != '\0')
					thread_type = 's';
			} // 나의 MAC주소를 요청할 경우 MAC주소를 넘겨주기 위해
			else if((IP_condition == 0) && (MAC_condition == 0) && (type == 2)){
				printf("Length %d\n", length);
				printf("Receiver Message : %s\n\n", output);
			} // 메시지를 전송 받을 경우 메시지 출력
			else{
				thread_type = 'e';
			} // 전송 받은 데이터가 나의 것이 아닌 경우
			if(!strcmp(output,"exit")) break;
		}
	}
	close(rcvsock);
}

void *do_thread(void *arg){
	while(1){
		if(thread_type == 's'){
			L1_send(output, strlen(output));

			thread_type = 'r';
		} // MAC주소 전송
		else if(thread_type == 'e'){
			strcpy(output, "error");
			type = 3;
			L1_send(output, strlen(output));

			thread_type = 'r';
		} // error전송
		if(!strcmp(output,"exit")) break; 
	}
	close(sndsock);
}

char * L1_receive(int *length){
	struct L1 *data; 
	int i;

	data = (struct L1*)L2_receive(length); 

	type = data->type;

	if(type == 1){
		strcpy(rec_MAC, data->rec_MAC);
	} // MAC 응답을 위한 상대방의 MAC저장
	else if(type == 2){
		if(strcmp(MAC, data->own_MAC) == 0)
			MAC_condition = 0;
		else
			MAC_condition = 1;
	} // 메시지를 전송 받을 경우 나의 것인지 MAC 검사

	*length = *length - sizeof(struct L1) + sizeof(data->L1_data);
	return (char *)data->L1_data; 
}

char * L2_receive(int *length){
	struct L2 *data; 
	int i;

	data = (struct L2*)L3_receive(length); 

	for(i = 0; i < 4; i++){
		if(IP[i] == data->own_IP[i]){
			IP_condition = 0;
		}
		else{
			IP_condition = 1;
			break;
		}
	} // IP검사

	if(IP_condition == 0) // IP의 값이 이상이 없을 경우 상대방의 IP저장
		memcpy(rec_IP, data->rec_IP, sizeof(rec_IP));

	*length = *length - sizeof(struct L2) + sizeof(data->L2_data);

	return (char *)data->L2_data;
}

char *L3_receive(int *length){
	static char data[MAX_SIZE];
	int i=0;

	if((i=recvfrom(rcvsock, data, MAX_SIZE, 0,(struct sockaddr *)&r_addr, &clen)) <= 0){
		perror("read error : ");
		exit(1);
	}
	*length = i;
	return data;
}

void L1_send(char *input, int length){
	struct L1 data; 
	char temp[350]; 
	int size = 0; 

	data.type = type; 
	data.length = length; 

	strcpy(data.own_MAC, MAC); // 나의 MAC주소 저장
	strcpy(data.rec_MAC, rec_MAC); // 전송할 곳의 MAC주소 저장

	memset(data.L1_data, 0x00, MAX_SIZE); 
	strcpy (data.L1_data, input); 

	size = sizeof(struct L1) - sizeof(data.L1_data) + length; 

	memset(temp, 0x00, 350); 
	memcpy(temp, (void *)&data, size); 

	L2_send(temp, size); 
}

void L2_send(char *input, int length){
	struct L2 data; 
	char temp[350]; 
	int size = 0; 

	memcpy(data.own_IP, &IP, sizeof(IP)); // 나의 IP주소 저장
	memcpy(data.rec_IP, &rec_IP, sizeof(rec_IP)); // 전송할 곳의 IP주소 저장

	data.length = length; 
	memset(data.L2_data, 0x00, MAX_SIZE); 
	memcpy (data.L2_data, (void *)input, length); 

	size = sizeof(struct L2) - sizeof(data.L2_data) + length; 

	memset(temp, 0x00, 350); 
	memcpy(temp, (void *)&data, size); 

	L3_send(temp, size); 
}

void L3_send(char *data, int length){
	char temp[300]; 

	memset(temp, 0x00, MAX_SIZE); 
	memcpy (temp, (void *)data, length); 

	if(sendto (sndsock, temp, length, 0, (struct sockaddr *)&s_addr,sizeof(s_addr)) <= 0){
		perror("write error : ");
		exit(1);
	}
}

void setSocket(){
	//Create Send Socket///////////////////////////////////////////////
	if((sndsock=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0){
		perror("socket error : ");
		exit(1);
	}

	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr=inet_addr(IP_ADDRESS);
	s_addr.sin_port = htons(8811);
	///////////////////////////////////////////////////////////////////

	//Create Receive Socket////////////////////////////////////////////
	if((rcvsock=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0){
		perror("socket error : ");
		exit(1);
	}

	clen = sizeof(r_addr);
	memset(&r_addr,0,sizeof(r_addr));

	r_addr.sin_family = AF_INET;
	r_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	r_addr.sin_port = htons(8810);

	if(bind(rcvsock,(struct sockaddr *)&r_addr, sizeof(r_addr)) < 0){
		perror("bind error : ");
		exit(1);
	}

	int optvalue = 1;
	int optlen = sizeof(optvalue);

	setsockopt(sndsock,SOL_SOCKET,SO_REUSEADDR,&optvalue,optlen);
	setsockopt(rcvsock,SOL_SOCKET,SO_REUSEADDR,&optvalue,optlen);
	///////////////////////////////////////////////////////////////////
}