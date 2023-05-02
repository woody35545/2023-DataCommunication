#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <time.h> //�����Լ��� ����ϱ� ���� �������

#define MAX_SIZE 300 //������ Data ����� 300���� ����

char saddr_ip_L1[100], daddr_ip_L1[100]; //���ڿ��� L1 �� �����ּҿ� �������ּҸ� �ޱ����� �迭 ����
char saddr_ip_L2[100], daddr_ip_L2[100]; //���ڿ��� L2 �� �����ּҿ� �������ּҸ� �ޱ����� �迭 ����
int ssock;
struct sockaddr_in server_addr;

struct L1 {
	int saddr[6];
	int daddr[6]; //�������ּ�
	int length;
	int seq; //seq�� ��Ÿ���� ����
	char L1_data[MAX_SIZE];
}; //L1 ������ ��Ÿ���� ����ü

struct L2 {
	int saddr[4];
	int daddr[4];
	int length;
	char L2_data[MAX_SIZE];
}; //L2 ������ ��Ÿ���� ����ü

/*���� �޽����� �޴� �Լ� ����*/
void L1_send(char *input, int length);
void L2_send(char *input, int length);
void L3_send(char *data, int length);
void setSocket(void); //���� ����

int main (void){	
	char input[MAX_SIZE]; //���� �޼���
	int length; //���� ����
	int button; //switch���� ���� ���� 
	setSocket(); //���� ����
	while (1) {
		/*�޴� ���ù� */ 
		printf("==========Choise Menu===========\n");
		printf("1. Select L1 address\n");
		printf("2. Select L2 address\n");
		printf("3. Send Message\n");
		scanf("%d", &button);

		//�Է¹��� ���� ���� ���������� �б⹮ 
		switch (button){
		case  1:
			__fpurge(stdin);
			printf("Input my L1 address : ");
			scanf("%s", saddr_ip_L1); 
			__fpurge(stdin);
			printf("Input my dest L1 address : ");
			scanf("%s", daddr_ip_L1);
			break;
			//L1�� ���� �ּҿ� ������ �ּҸ� �Է¹޴� �κ�.
		case  2:
			__fpurge(stdin);
			printf("Input my L2 address : ");
			scanf("%s", saddr_ip_L2); 
			__fpurge(stdin);
			printf("Input my dest L2 address : ");
			scanf("%s", daddr_ip_L2);
			break;
			//L2�� ���� �ּҿ� ������ �ּҸ� �Է¹޴� �κ�.
		case  3: 
			__fpurge(stdin);
			printf("put the message: ");
			gets(input);
			L1_send(input, strlen(input));
			//������ ���ڸ� �Է¹޴� �κ�.
			break;
		default:
			printf("Error!!!!!!!!!\n");
			break;
			//����ó�� ����
		}
		if(!strcmp(input,"exit"))
			break; //�Է��� �޽����� ��exit' ���� ��exit' �̸� ���α׷� ����
	}
	close(ssock); //���� ����
}

void L1_send(char *input, int length){
	struct L1 data; //����ü ���� ����
	char temp[350];
	int size = 0; //�ʱ�ȭ
	int i;
	char *token;
	static int seq ; //�������� ����
	srand((unsigned)time(NULL)); //������ seed���� �ʱ�ȭ
	seq = rand()%100 + 1; //0���� 100������ ���� ����
	token = strtok(saddr_ip_L1, "-");//���ڿ����� '-'�� �������� ���ڿ��� ������.
	i = 0;

	while (token != NULL){
		data.saddr[i] = atoi(token);
		token = strtok(NULL, "-");
		i++;
	} //���ڿ����� ���� �����͵��� integer������ ��ȯ�Ͽ� L1�� �����ּҿ� ����
	token = strtok(daddr_ip_L1, "-"); //���ڿ����� '-'�� �������� ���ڿ��� ������.
	i = 0;

	while (token != NULL){
		data.daddr[i] = atoi(token);
		token = strtok(NULL, "-");
		i++;
	} //���ڿ����� ���� �����͵��� integer������ ��ȯ�Ͽ� L1�� �������ּҿ� ����

	data.length = length; //�޾ƿ� ���̸� �����Ϳ� ����
	data.seq = seq; //������ seq�� �����Ϳ� ����.
	memset(data.L1_data, 0x00, MAX_SIZE); //L1������ �޸� �ʱ�ȭ
	strcpy (data.L1_data, input); //�Է��� ������ L2�� ����
	size = sizeof(struct L1) - sizeof(data.L1_data) + length; //���� �Է��� �����ͱ��̸�ŭ ������ ����

	memset(temp, 0x00, 350); //temp�� �޸𸮸� 350��ŭ 0���� �ʱ�ȭ  
	memcpy(temp, (void *)&data, size); //������ ���� size��ŭ �����͸� temp����
	L2_send(temp, size); //temp �� data�� L2�� ����
	printf ("Sender_sequence  : %d\n", data.seq); //sequence �� ���
}

void L2_send(char *input, int length){
	struct L2 data; //����ü ���� ����
	char temp[350];
	int size = 0; //�ʱ�ȭ
	int i;
	char *token;
	token = strtok(saddr_ip_L2, "."); //���ڿ����� '.'�� �������� ���ڿ��� ������.
	i = 0;

	while (token != NULL){
		data.saddr[i] = atoi(token);
		token = strtok(NULL, ".");
		i++;
	} //���ڿ����� ���� �����͵��� integer������ ��ȯ�Ͽ� L2�� �����ּҿ� ����

	token = strtok(daddr_ip_L2, "."); //���ڿ����� '.'�� �������� ���ڿ��� ������.
	i = 0;

	while (token != NULL){
		data.daddr[i] = atoi(token);
		token = strtok(NULL, ".");
		i++;
	} //���ڿ����� ���� �����͵��� integer������ ��ȯ�Ͽ� L2�� �������ּҿ� ����

	data.length = length; //�޾ƿ� ���̸� �����Ϳ� ����
	memset(data.L2_data, 0x00, MAX_SIZE); //L1������ �޸� �ʱ�ȭ
	memcpy (data.L2_data, (void *)input, length); //�Է��� ������ L1�� ����

	size = sizeof(struct L2) - sizeof(data.L2_data) + length; //���� �Է��� �����ͱ��̸�ŭ ������ ����
	memset(temp, 0x00, 350); //temp�� �޸𸮸� 350��ŭ 0���� �ʱ�ȭ
	memcpy(temp, (void *)&data, size); //������ ���� size��ŭ �����͸� temp����
	L3_send(temp,size); //temp �� data�� L3�� ����
}

void L3_send(char *data, int length){
	char temp[300];
	memset(temp, 0x00, MAX_SIZE); //temp�� �޸𸮸� 350��ŭ 0���� �ʱ�ȭ
	memcpy (temp, (void *)data, length); //������ ���� size��ŭ �����͸� temp����
	if(sendto (ssock, temp, length, 0, (struct sockaddr *)&server_addr,sizeof(server_addr)) <= 0){
		perror("write error : ");
		exit(1);
	}
}

void setSocket(){ //���� ���� 
	if((ssock=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0) {
		perror("socket error : ");
		exit(1);
	}
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
	server_addr.sin_port = htons(9658);
}