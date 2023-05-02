#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define MAX_SIZE 300
#define IP_ADDRESS "127.0.0.1"
int sndsock,rcvsock;
int clen;
int IP_condition = 0, MAC_condition = 0, table_count = -1, table_num; // IP, MAC�� ���¿� Cache���̺� ī���Ϳ� ���̺� üũ�� ���� ����

int IP[4] = {203, 255, 43, 21}; // ���� IP�ּ�
int input_IP[4]; // �Է� ���� IP�� ������ ����
char MAC[20] = "0A:10:9C:31:79:6F"; // ���� MAC�ּ�
int type = 1; 
char mode = 'n'; // sender�� Cache_table thread�� ������ ����
int re_time = 60; // renewal Ÿ�� ����
char thread_type = 's'; // thread Ÿ���� ������ ����
clock_t past; // clock�� �̿��� ���� �ð��� ������ ����

struct sockaddr_in s_addr,r_addr;

struct L1{
	char own_MAC[20];
	char rec_MAC[20];
	int length;
	int type;
	char L1_data[MAX_SIZE];
}; // ����ü L1

struct L2{
	int own_IP[4];
	int rec_IP[4];
	int length;
	char L2_data[MAX_SIZE];
}; // ����ü L2

struct ARP_Cache_Table{
	int IP[4];
	char MAC[20];
} ARP_Cache[5]; // ARP_Cache_Table ����ü

void L1_send(char *input, int length);
void L2_send(char *input, int length, char);
void L3_send(char *data, int length);
char *L1_receive(int *);
char *L2_receive(int *);
char *L3_receive(int *);
void setSocket(void);
void *do_thread(void *); // receiver�� thread
void *table_thread(void *); // Cache_table�� thread
int check_table(char *); // Cache_table�� ����� ������ �˻��� �Լ�

int main (void){
	char input[MAX_SIZE]; // �޽����� �Է� ���� input �迭
	int select; // // �޴��� ������ ����
	char dest_ip[50]; // ip���� �����ϱ� ���� ����
	pthread_t tid; // thread�� �����ϱ����� ����

	setSocket(); // // receiver�� ����
	pthread_create(&tid, NULL, do_thread, (void *)0); //receiver�� thread ����
	pthread_create(&tid, NULL, table_thread, (void *)0); //Cache_table�� thread ����

	while (1){
		if(thread_type == 's' && type == 1 && mode == 'n'){
			printf("\n----------------------------\n");
			printf("- 1. Send Data             -\n");
			printf("- 2. View ARP Cache Table  -\n");
			printf("- 3. Exit                  -\n");
			printf("----------------------------\n");
			printf("- What do you want ? : ");
			scanf("%d", &select);

			switch(select){
			case 1: // 	send data�� ip�� �Է¹޴´�
				memset(dest_ip, 0x00, 50);
				printf("Send Text : ");
				scanf("%s", input);
				printf("Destination IP : ");
				scanf("%s", dest_ip);
				if(check_table(dest_ip) == 1){
					strcpy(dest_ip, "Requst");
					L1_send(dest_ip, strlen(dest_ip));
					thread_type = 'r';
				}// Cache_table�� IP�� ���� ���� ��� MAC�� ȣ���ϱ� ����
				else{
					type = 2;
				}// Cache_table�� IP�� ���� ���� ��� �޽����� �����ϱ� ����
				break;
			case 2: // Cache_table�� ����
				mode = 'v';
				break; 
			case 3: // ����
				strcpy(input,"exit");
				break;
			default: // 1~3 �̿��� ���� �Է��� ���
				printf("Please Write number in menu\n");
			}		
			if(!strcmp(input,"exit")) break;
		}
		else if(thread_type == 's' && type == 2 && mode == 'n'){
			L1_send(input, strlen(input));
			type = 1;
		}// Cache_table�� �Է��� IP�� ���� ������� �޽��� ����
	}
	close(sndsock);
}

void *do_thread(void *arg){
	char output[MAX_SIZE]; // ���� ���� �޽����� ������ ����
	int length; // �������� ����� ������ ����

	while (1){
		if(thread_type == 'r'){
			memset(output, 0x00, MAX_SIZE);

			strcpy(output, L1_receive(&length));

			if(type == 2) // MAC �ּҸ� �޾ƿ��� ���
				printf("ARP_Reply\n");
			else if(type == 3 ){ // �Է� ���� IP�� ������ �ִ� ���� �������
				printf("Nobody have input IP\n");
				type = 1;
			}

			output[length] = '\0';

			if(output != '\0') // sender�� ����
				thread_type = 's';

			if(!strcmp(output,"exit")) break; // 
		}
	}
	close(rcvsock);
}

void *table_thread(void *arg){
	int i, back, p_time;

	past = clock();// ������ clock���� ����
	re_time = 60;

	while(1){
		if((clock() - past) / CLOCKS_PER_SEC){
			re_time--;
			past = clock();
		} // 1�ʸ��� renewal time�� ���� ���δ�.

		if(re_time == 0){
			memset(ARP_Cache, 0x00, sizeof(struct ARP_Cache_Table));
			table_count = -1;
			re_time = 10;
		} // renewal�� ���� 0�� �� ��� ARP_Cache_Table�� �ʱ�ȭ�Ѵ�.

		if(mode == 'v'){ // ARP_Cache_Table ���
			for(i = 0; i <= table_count; i++){
				printf("ARP_Cache Table number : %d\n", i);
				printf("Destination_IP : %d.%d.%d.%d\n", ARP_Cache[i].IP[0], ARP_Cache[i].IP[1], ARP_Cache[i].IP[2], ARP_Cache[i].IP[3]);
				printf("Destination_MAC : %s\n\n", ARP_Cache[i].MAC);
			}

			if(table_count <= -1){
				printf("ARP_Cache Table is empty\n");
				mode = 'n';
			}
			else
				mode = 't';

			back = re_time;
			p_time = re_time + 1;
		}

		if(mode == 't' && table_count > -1){ // renewal ���� ���� �ð��� ���
			if(re_time != p_time){
				printf("\rRenewal Time : %2d", re_time);
				fflush(stdout);

				p_time = re_time;
			}

			if((back - re_time) >= 5) // 5�ʰ� renewal���� ���� �ð��� �����ְ� �޴��� ���ư���.
				mode = 'n';
		}
		else if(mode == 't' && table_count == -1){
			printf("Renewal Cache_table\n\n");
			mode = 'n';
		} // Cache_Table�� ���� ���� �� renewal time�� 0�̵Ǹ� ���
	}
}

int check_table(char *ip){
	int i, j, c;
	char token[10];
	int result = 1;

	memset(token, 0x00, 10); // token �ʱ�ȭ

	for(i = 0, j = 0; i < 4; i++) {
		for(c = 0; j < 50; j++) {
			if('0' <= ip[j] && ip[j] <= '9') {
				token[c] = ip[j];
				c++;
			} 
			else{
				j++;
				break;
			} 
		}
		input_IP[i] = atoi(token); 
		memset(token, 0x00, 10); 
	} // �Է¹��� IP�� ���� token�� �̿��� �и��Ͽ� input_IP�� �����Ѵ�.


	for(i = 0; i <= table_count; i++){
		for(j = 0, c = 0; j < 4; j++){
			if(ARP_Cache[i].IP[j] == input_IP[j])
				c++;
		}		
		if(c == 4){
			table_num = i;
			result = 0;
			break;
		}
		else
			result = 1;
	} // �Է¹��� IP�� ���� Cache_Table�� �����ϴ��� �˻��Ѵ�.

	return result;
}

void L1_send(char *input, int length){
	struct L1 data; 
	char temp[350]; 
	int size = 0; 
	int i, j, c; 

	memset(data.L1_data, 0x00, MAX_SIZE); 

	strcpy(data.own_MAC, MAC); 
	strcpy (data.L1_data, input); 
	data.type = type; 
	data.length = length; 

	if(type == 2) // �Է� ���� IP�� MAC�ּҰ� Cache_Table�� �����Ұ��
		strcpy(data.rec_MAC, ARP_Cache[table_num].MAC);
	else // �Է� ���� IP�� MAC �ּҸ� �𸦰��
		memset(data.rec_MAC, 0x00, sizeof(data.rec_MAC));

	size = sizeof(struct L1) - sizeof(data.L1_data) + length; // input ���� ������ ����ü L1�� ������ ���

	memset(temp, 0x00, 350); 
	memcpy(temp, (void *)&data, size); 

	L2_send(temp, size, input[0]); 
}

void L2_send(char *input, int length, char parity){
	struct L2 data; 
	char temp[350]; 
	int size = 0; 

	memcpy(data.own_IP, &IP, sizeof(IP)); // ���� IP�� ���� ����

	if(type == 1)
	{
		memcpy(data.rec_IP, &input_IP, sizeof(input_IP));
		printf("ARP_Requst\n");
	} // Cache_Table�� �Է¹��� IP�� MAC�ּҰ� �������� ���� ��� �Է¹��� IP�� ����
	else if(type == 2) // Cache_Table�� �Է¹��� IP�� ���� �����Ұ�� Cache_Table�� ���� �����´�.
		memcpy(data.rec_IP, &ARP_Cache[table_num].IP, sizeof(data.rec_IP));

	data.length = length; 
	memset(data.L2_data, 0x00, MAX_SIZE); 
	memcpy (data.L2_data, (void *)input, length); 

	size = sizeof(struct L2) - sizeof(data.L2_data) + length; 

	memset(temp, 0x00, 350); 
	memcpy(temp, (void *)&data, size); 

	L3_send(temp, size); 
}

void L3_send(char *data, int length) {
	char temp[300]; 

	memset(temp, 0x00, MAX_SIZE); 
	memcpy (temp, (void *)data, length); 

	if(sendto (sndsock, temp, length, 0, (struct sockaddr *)&s_addr,sizeof(s_addr)) <= 0){
		perror("write error : ");
		exit(1);
	}
}

char * L1_receive(int *length){
	struct L1 *data; 

	data = (struct L1*)L2_receive(length); 

	if(strcmp(MAC, data->own_MAC) == 0) // ���� ���� data�� MAC�ּҰ��� �˻��Ѵ�.
		MAC_condition = 0;
	else
		MAC_condition = 1;

	if(IP_condition == 0 && MAC_condition == 0 && data->type != 3){ // �޾ƿ� MAC�ּҸ� Cache_Table�� ����
		strcpy(ARP_Cache[table_count].MAC, data->rec_MAC);
		re_time = 60;
		printf("ARP_Cache Table is Set!!\n");
	}
	else{  // ���� ���� data�� ���� ���� ���� �ƴҰ�� IP�� �ʱ�ȭ
		memset(ARP_Cache[table_count].IP, 0x00, sizeof(ARP_Cache[table_count].IP));
		table_count--;
	}
	*length = *length - sizeof(struct L1) + sizeof(data->L1_data);
	type = data->type; 

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
	}  // ���۹��� data�� IP �˻� 

	if(IP_condition == 0){
		table_count++;
		memcpy(ARP_Cache[table_count].IP, data->rec_IP, sizeof(data->rec_IP));
	}  // IP ���� �̻� ���� ��� Cache_Table�� IP�� ����

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

void setSocket(){
	//Create Send Socket///////////////////////////////////////////////
	if((sndsock=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0){
		perror("socket error : ");
		exit(1);
	}
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr=inet_addr(IP_ADDRESS);
	s_addr.sin_port = htons(8810);
	///////////////////////////////////////////////////////////////////

	//Create Receive Socket////////////////////////////////////////////
	if((rcvsock=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0) {
		perror("socket error : ");
		exit(1);
	}

	clen = sizeof(r_addr);
	memset(&r_addr,0,sizeof(r_addr));

	r_addr.sin_family = AF_INET;
	r_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	r_addr.sin_port = htons(8811);

	if(bind(rcvsock,(struct sockaddr *)&r_addr, sizeof(r_addr)) < 0) {
		perror("bind error : ");
		exit(1);
	}

	int optvalue = 1;
	int optlen = sizeof(optvalue);

	setsockopt(sndsock,SOL_SOCKET,SO_REUSEADDR,&optvalue,optlen);
	setsockopt(rcvsock,SOL_SOCKET,SO_REUSEADDR,&optvalue,optlen);
	///////////////////////////////////////////////////////////////////
}