#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <time.h> //�����Լ��� ����ϱ� ���� �������
#include <libgen.h>

#define MAX_SIZE 300 //������ Data ����� 300���� ����
#define PASS 0 
#define L1_FAIL 1
#define L2_FAIL 2
#define L1_L2_FAIL 3
#define IP_ADDRESS "127.0.0.1"

struct L1 {
	int saddr[4];
	int daddr[4]; //�������ּ�
	int length;
	int seq; //seq�� ��Ÿ���� ����
	char L1_data[MAX_SIZE];
}; //L1 ������ ��Ÿ���� ����ü

struct L2 {
	int saddr[6];
	int daddr[6];
	int length;
	char L2_data[MAX_SIZE];
}; //L2 ������ ��Ÿ���� ����ü

/*���� �޽����� �޴� �Լ� ����*/
void L1_send(char *input, int length);
void L2_send(char *input, int length);
void L3_send(char *data, int length);
/*�����κ��� ���� �޽����� �� �������� �޾Ƽ� Ǯ����� �Լ� ����*/
char *L1_receive(int *);
char *L2_receive(int *);
char *L3_receive(int *);
/*���� �ҽ������� Addr ����ü�� ���� ����߽��ϴ�*/
struct L1 L1_Data; //L1������ �����ּҿ� �������ּҸ� ���ϱ� ���� ����ü ����
struct L2 L2_Data; //L2������ �����ּҿ� �������ּҸ� ���ϱ� ���� ����ü ����
char saddr_ip_L1[100], daddr_ip_L1[100]; //���ڿ��� L1 �� �����ּҿ� �������ּҸ� �ޱ����� �迭 ����
char saddr_ip_L2[100], daddr_ip_L2[100]; //���ڿ��� L2 �� �����ּҿ� �������ּҸ� �ޱ����� �迭 ����
int Error_Flag; // sender�� receive�� ������ �ּҸ� ���Ͽ� �ٸ� �κ��� ����� �˷��ִ� ����

int sndsock, rcvsock, clen;
struct sockaddr_in s_addr, r_addr;
void *do_thread(void *);
void init_socket();
void check_is_server(char *const *argv);
int is_server = 0;

int main (int argc, char *argv[]){	
	
	check_is_server(argv);
    init_socket();
	
	char input[MAX_SIZE]; //���� �޼���
	char output[MAX_SIZE]; //���� �޼���
	int length; //���� ����
	int button; //switch���� ���� ���� 
	if (is_server == 0){
		// sender
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
				printf("Input my L1 address(IP) : ");
				scanf("%s", saddr_ip_L1); 
				__fpurge(stdin);
				printf("Input my dest L1 address(IP) : ");
				scanf("%s", daddr_ip_L1);
				break;
				//L1�� ���� �ּҿ� ������ �ּҸ� �Է¹޴� �κ�.
			case  2:
				__fpurge(stdin);
				printf("Input my L2 address(MAC) : ");
				scanf("%s", saddr_ip_L2); 
				__fpurge(stdin);
				printf("Input my dest L2 address(MAC) : ");
				scanf("%s", daddr_ip_L2);
				break;
				//L2�� ���� �ּҿ� ������ �ּҸ� �Է¹޴� �κ�.
			case  3: 
				__fpurge(stdin);
				printf("message: ");
			    fgets(input, sizeof(input), stdin);					
				L1_send(input, strlen(input));
				//������ ���ڸ� �Է¹޴� �κ�.
				break;
			default:
				printf("Error!!!!!!!!!\n");
				break;
				//����ó�� ����
			}
				if(!strcmp(input,"exit"))break; //�Է��� �޽����� ��exit' ���� ��exit' �̸� ���α׷� ����
		}
	}else{
		// receiver
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
			printf("Received: %s", output);
		}
			if(!strcmp(output,"exit")) break; //���� �޽����� ��exit' ���� exit��� ���α׷� ����
		}
	}
	
}

void L1_send(char *input, int length){
    printf("call--> [%s]  \n", __func__);
	struct L1 data; //����ü ���� ����
	char temp[350];
	int size = 0; //�ʱ�ȭ
	int i;
	char *token;
	token = strtok(saddr_ip_L1, "."); //���ڿ����� '.'�� �������� ���ڿ��� ������.	
	static int seq ; //�������� ����
	srand((unsigned)time(NULL)); //������ seed���� �ʱ�ȭ
	seq = rand()%100 + 1; //0���� 100������ ���� ����
	while (token != NULL){
		data.saddr[i] = atoi(token);
		token = strtok(NULL, ".");
		i++;
	} //���ڿ����� ���� �����͵��� integer������ ��ȯ�Ͽ� L2�� �����ּҿ� ����
	i = 0;
	token = strtok(daddr_ip_L1, "."); //���ڿ����� '.'�� �������� ���ڿ��� ������.
	while (token != NULL){
		data.daddr[i] = atoi(token);
		token = strtok(NULL, ".");
		i++;
	} //���ڿ����� ���� �����͵��� integer������ ��ȯ�Ͽ� L2�� �������ּҿ� ����

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
    printf("call--> [%s]  \n", __func__);
	struct L2 data; //����ü ���� ����
	char temp[350];
	int size = 0; //�ʱ�ȭ
	int i;
	char *token;
	token = strtok(saddr_ip_L2, ":");//���ڿ����� ':'�� �������� ���ڿ��� ������.
	i = 0;

	while (token != NULL){
		data.saddr[i] = atoi(token);
		token = strtok(NULL, ":");
		i++;
	} //���ڿ����� ���� �����͵��� integer������ ��ȯ�Ͽ� L1�� �����ּҿ� ����
	token = strtok(daddr_ip_L2, ":"); //���ڿ����� ':'�� �������� ���ڿ��� ������.
	i = 0;

	while (token != NULL){
		data.daddr[i] = atoi(token);
		token = strtok(NULL, ":");
		i++;
	} //���ڿ����� ���� �����͵��� integer������ ��ȯ�Ͽ� L1�� �������ּҿ� ����

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
	if(sendto (sndsock, temp, length, 0, (struct sockaddr *)&s_addr,sizeof(s_addr)) <= 0){
		perror("write error : ");
		exit(1);
	}
}

char * L1_receive(int *length){
	static int seq; //�������� ����
	struct L1 *data; //data���� ����

	/*L1�� �����ּҿ� �������ּҸ� ����*/
	//unsigned char ip[] = {192, 168, 0, 1};
	L1_Data.saddr[0] = 192;
	L1_Data.saddr[1] = 168;
	L1_Data.saddr[2] = 30;
	L1_Data.saddr[3] = 1;
	L1_Data.daddr[0] = 192;
	L1_Data.daddr[1] = 168;
	L1_Data.daddr[2] = 0;
	L1_Data.daddr[3] = 1;

	data = (struct L1*)L2_receive(length); //L2���� ���� ���� ��ŭ L1 data�� ����                                                      
	*length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr) - sizeof(data->seq); //���� �������� ���̸� ����Ͽ� ����

	for (int i = 0; i <= 100; i++){ //sender���� �߻��� ���� ã��                                  
		if (data->seq == i)
			seq = i;
	}
	if(data->seq == seq) //sender���� �޾ƿ� seq�� ���� ������ seq�� ���
		printf ("Expected_sequence  : %d\n", data->seq);
	for(int i = 0; i < 4;  i++){ //sender���� ������ �����ּҿ� receiver�� �����ּҸ� ���ϴ� �ݺ���
		printf("Your IP[%d]%d\n",i,data->daddr[i]);
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
	data = (struct L2*)L3_receive(length); //L3���� ���� ���� ��ŭ L2 data�� ����
	*length = *length - sizeof(data->daddr) - sizeof(data->length) - sizeof(data->saddr); //���� �������� ���̸� ����Ͽ� ����
	for(i = 0;  i < 6;  i++){ //sender���� ������ �����ּҿ� receiver�� �����ּҸ� ���ϴ� �ݺ���
		printf("Your MAC[%d]%d\n",i,data->daddr[i]);
		if(L2_Data.daddr[i] != data->daddr[i]) //L2�� ������ �ּҰ� �ٸ� ���
			Error_Flag = L2_FAIL; 
	}
	
	return (char *)data->L2_data; //�����͸� ������ ���� L1�� ����
}

char *L3_receive(int *length){
	static char data[MAX_SIZE]; //���� ���� ����
	int i=0; //0��° ����� �ϳ��� Ȯ��	
	if((i=recvfrom(rcvsock, data, MAX_SIZE, 0,(struct sockaddr *)&r_addr, &clen)) <= 0){
		perror("read error : ");
		exit(1);
	}
	*length = i; //�޾ƿ� ���̸�ŭ length�� ����
	return data; //������ ����
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