#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#define MAX_SIZE 300 //������ Data ����� 300���� ����
/*Flag���� ����� ��ũ�� ����*/

#define PASS		0 
#define L1_FAIL		1
#define L2_FAIL 	2
#define L1_L2_FAIL	3

int ssock; //���� ����
int clen; //sender �ּ��� ũ��	
int Error_Flag; // sender�� receive�� ������ �ּҸ� ���Ͽ� �ٸ� �κ��� ����� �˷��ִ� ����
struct sockaddr_in server_addr; //�������α׷��� ����� ����ü

struct L1 {
	int saddr[6]; //L1���� �ҽ� ��巹��
	int daddr[6]; //L1���� ������ �ּ�
	int length; //���� ��Ÿ���� ����
	int seq; //seq�� ��Ÿ���� ����
	char L1_data[MAX_SIZE]; //L1 ������ ���� �޽��� ����
}; //L1 ������ ��Ÿ���� ����ü

struct L2 {
	int saddr[4]; //L2���� �ҽ� ��巹��
	int daddr[4]; //L2���� ������ ��巹��
	int length; //���� ��Ÿ���� ����
	char L2_data[MAX_SIZE]; //L1 ������ ���� �޽��� ����
}; //L1 ������ ��Ÿ���� ����ü
struct L1 L1_Data; //L1������ �����ּҿ� �������ּҸ� ���ϱ� ���� ����ü ����
struct L2 L2_Data; //L2������ �����ּҿ� �������ּҸ� ���ϱ� ���� ����ü ����
/*�����κ��� ���� �޽����� �� �������� �޾Ƽ� Ǯ����� �Լ� ����*/
char *L1_receive(int *);
char *L2_receive(int *);
char *L3_receive(int *);

void setSocket(void); //���ϻ��� �Լ�

int main (void){
	char output[MAX_SIZE]; //���� �޼���
	int length; //���� ����
	setSocket(); //���ϻ����Լ�
	while (1) {
		strcpy(output, L1_receive(&length)); //���� �޽��� ���� �� ����		
		output[length] = '\0'; //����� �޽����� �η� �ʱ�ȭ
		if(Error_Flag == L1_L2_FAIL){ //L1�� L2�� �������ּҰ� �ٸ� ��� 
			printf("L1, L2 Address is Not Correct!! \n");
			Error_Flag = PASS;
		}
		else if(Error_Flag == L1_FAIL){ //L1�� �������ּҰ� �ٸ� ��� 
			printf("L1 Address is Not Correct!! \n");
			Error_Flag = PASS;
		}
		else if(Error_Flag == L2_FAIL){ //L2�� �������ּҰ� �ٸ� ��� 
			printf("L2 Address is Not Correct!! \n");
			Error_Flag = PASS;
		}
		else{ //L1�� L2�� �������ּҰ� ���� ��� 
			printf("Length %d\n", length);
			printf("Received : ");
			puts(output);
			printf("\n");
		}
		if(!strcmp(output,"exit")) break; //���� �޽����� ��exit' ���� exit��� ���α׷� ����
	}
	close(ssock); //���� ����
} //exit�� �޾ƿ� �� ���� �ݺ��ؼ� �޽��� ����

char * L1_receive(int *length){
	static int seq; //�������� ����
	struct L1 *data; //data���� ����
	int i; //for���� ���� ���� ����
	/*L1�� �����ּҿ� �������ּҸ� ����*/
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

	data = (struct L1*)L2_receive(length); //L2���� ���� ���� ��ŭ L1 data�� ����                                                      
	*length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr) - sizeof(data->seq); //���� �������� ���̸� ����Ͽ� ����

	for (i = 0; i <= 100; i++){ //sender���� �߻��� ���� ã��                                  
		if (data->seq == i)
			seq = i;
	}
	if(data->seq == seq) //sender���� �޾ƿ� seq�� ���� ������ seq�� ���
		printf ("Expected_sequence  : %d\n", data->seq);
	for(i = 0;  i < 6;  i++){ //sender���� ������ �����ּҿ� receiver�� �����ּҸ� ���ϴ� �ݺ���
		if(L1_Data.daddr[i] != data->daddr[i]){ //sender���� ������ �����ּҿ� receiver�� �����ּҰ� �ٸ� ��� 
			if(Error_Flag == L2_FAIL){ //L2������ ������ �ּҰ� �ٸ� ���
				Error_Flag = L1_L2_FAIL;
				i = 99;
			}
			else{ //L1�� ������ �ּҰ� �ٸ� ���	
				Error_Flag = L1_FAIL;
				i = 99;
			}		
		}
	}
	return (char *)data->L1_data; //main �����͸� ������ ���� �� ����
}

char * L2_receive(int *length){
	struct L2 *data; //data���� ����
	int i;
	/*L2�� �����ּҿ� �������ּҸ� ����*/
	L2_Data.saddr[0] = 192;
	L2_Data.saddr[1] = 168;
	L2_Data.saddr[2] = 30;
	L2_Data.saddr[3] = 33;
	L2_Data.daddr[0] = 191;
	L2_Data.daddr[1] = 167;
	L2_Data.daddr[2] = 31;
	L2_Data.daddr[3] = 33;
	data = (struct L2*)L3_receive(length); //L3���� ���� ���� ��ŭ L2 data�� ����
	*length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr); //���� �������� ���̸� ����Ͽ� ����

	for(i = 0; i < 4;  i++){ //sender���� ������ �����ּҿ� receiver�� �����ּҸ� ���ϴ� �ݺ���
		if(L2_Data.daddr[i] != data->daddr[i]) //L2�� ������ �ּҰ� �ٸ� ���
			Error_Flag = L2_FAIL; 
	}
	return (char *)data->L2_data; //�����͸� ������ ���� L1�� ����
}

char *L3_receive(int *length){
	static char data[MAX_SIZE]; //���� ���� ����
	int i=0; //0��° ����� �ϳ��� Ȯ��	

	if((i=recvfrom(ssock, data, MAX_SIZE, 0,(struct sockaddr *)&server_addr, &clen)) <= 0){
		perror("read error : ");
		exit(1);
	}
	*length = i; //�޾ƿ� ���̸�ŭ length�� ����
	return data; //������ ����
}

void setSocket(){ //���� ����
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