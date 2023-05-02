#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h> // packet capture api
// ARP ��Ŷ ĸó�� ��Ŷ ���� Ȯ���� ���� �����Դϴ�. (������ ��� �ǽ��ð�)
// ARP ��� ������ �ڽ��� ��Ʈ��ũ ���� ARP ��Ŷ�� ĸó�Ͽ� �����͸� Ȯ���غ��ϴ�.
struct ether_addr {
  unsigned char mac_add[6];  
};

struct ether_header {
    struct ether_addr etherdst_mac; 
    struct ether_addr ethersed_mac; 
    unsigned short ether_type; // ARP���� Ȯ�� �� �� �ִ� �κ� 0x0806=ARP 0x0800=IP // short=2byte
};

#pragma pack(push, 2)  // ����ü ũ�� �����ϱ� = 2����Ʈ ������ �޸� ������� �����ϱ� ����.
struct arp_header {
    unsigned short hw_type; // ��� ������ ��ü �����ּ�(MAC) ����
    unsigned short protocol_type; // ������� �������� �ּ� 
    unsigned char hw_addr_len; // ��Ŷ�� ���Ǵ� MAC ����
    unsigned char protocol_addr_len; // ��Ŷ�� ���Ǵ� IP �ּ� ����
    unsigned short operation_code; // ��û(1) �Ǵ� ����(2)
    struct ether_addr source_mac; // Sender L2 ������ �����ּ�
    struct in_addr source_ip; // Sender L3 ������ �� �ּ� 
    struct ether_addr destination_mac; // Receiver L2 �ּ�
    struct in_addr destination_ip; // Receiver L3 �� �ּ�
};
#pragma pack(pop)

// �Լ� ����
void print_ether_header(const unsigned char *pkt_data);
void print_arp_header(const unsigned char *pkt_data);

// main
int main() {
  pcap_if_t *alldevs;   // ������ alldevs�� �ڷ����� pcap_if_t
  pcap_if_t *d;
  int inum,res,i=0;
  struct pcap_pkthdr *header;  //pcap_pkthdr ����ü : 
  const unsigned char *pkt_data;  //��Ŷ�� ������ ����
  pcap_t *adhandle;
  char errbuf[PCAP_ERRBUF_SIZE];
  char packet_filter[] = ""; // ����ڰ� ���ϴ� �������� ���� ������ ���� �� �ִ� ����
  struct bpf_program fcode; // Ư�� �������ݸ��� ĸ���ϱ� ���� ��å���� ����

  // alldevs �� ����̽� ��� ����,������ errbuf�� ��������
  if(pcap_findalldevs(&alldevs, errbuf) == -1) {   
    printf("Error in pcap_findalldevs: %s\n", errbuf);
    exit(1);
  }

  for(d=alldevs; d; d=d->next) {  //��Ʈ��Ʈ ����̽� ������ ���
    printf("%d. %s", ++i, d->name);
    if (d->description)
      printf(" (%s)\n", d->description);
    else
      printf(" (No description available)\n");
  }
  //����̽� ��ã�� ��� ����
  if(i==0) {   
    printf("\nNo interfaces found! Make sure LiPcap is installed.\n");
    //return -1;
  }

  printf("Enter the interface number (1-%d):",i);
  scanf("%d", &inum);
  //�Է��� ���� �ùٸ��� �Ǵ� && : �Ѵ� ���̾�� ��
  if(inum < 1 || inum > i) {  
    printf("\nAdapter number out of range.\n");
    pcap_freealldevs(alldevs);  
    return -1;
  }
  //����ڰ� ������ ��ġ ����� ����
  for(d=alldevs, i=0; i< inum-1 ;d=d->next, i++);   

  //���� ��Ʈ��ũ ����̽� ����
  if((adhandle= pcap_open_live(d->name, 65536,   1,  1000,  errbuf  )) == NULL) {   
    printf("\nUnable to open the adapter. %s is not supported by pcap\n", d->name);
    pcap_freealldevs(alldevs);
    return -1;
  }
 
 //��Ŷ ���͸� ��å�� ���� pcap_compile()�Լ� ȣ�� 
 //����ڰ� ������ ���͸� ���� bpf_program ����ü�� �����Ͽ� Ư�� �������� ��Ŷ�� ����
  if (pcap_compile(adhandle, &fcode, packet_filter, 1, 0) <0 )  { 
    printf("\nUnable to compile the packet filter. Check the syntax.\n");
    pcap_freealldevs(alldevs);
    return -1;
  }

  //pcap_compile() �Լ������� �����ϱ� ����  pcap_setfilter() �Լ��� ���ȴ�.
  if (pcap_setfilter(adhandle, &fcode)<0)  {  
    printf("\nError setting the filter.\n");
    pcap_freealldevs(alldevs);
    return -1;
  }
  // ����̽� ���� ���
  printf("\nlistening on %s...\n", d->name);    
  pcap_freealldevs(alldevs);   // ����

    while ((res = pcap_next_ex(adhandle, &header, &pkt_data)) >= 0) {
        if (res == 0) continue;
        print_ether_header(pkt_data);
        pkt_data += 14;
        print_arp_header(pkt_data);
    }
    
  return 0;
}

// �̴��� ������ �����
void print_ether_header(const unsigned char *pkt_data) {  

    struct ether_header *eth; //�̴��� ��� ������ ���� �� �ִ� ������ �̴������ ����ü�� eth�� ����
    eth = (struct ether_header *)pkt_data;  // ����ü eth�� ��Ŷ ������ ����
    unsigned short eth_type;
    eth_type= ntohs(eth->ether_type);  // ���ڷ� ���� ���� ��Ʋ ����� �������� �ٲپ���
    // ARP ��Ŷ�� �κи� ĸó
    if (eth_type == 0x0806) {
        printf("\n====== ARP packet ======\n");
        printf("\nSrc MAC : ");
        for (int i=0; i<=5; i++)
            printf("%02x:",eth->ethersed_mac.mac_add[i]);
        printf("\nDst MAC : ");
        for (int i=0; i<=5; i++)
            printf("%02x:",eth->etherdst_mac.mac_add[i]);
        printf("\n");
    }
}
// ARP ��Ŷ ������ ���
void print_arp_header(const unsigned char *pkt_data) {  

    struct arp_header *arprqip;
    struct arp_header *arprpip;
    struct arp_header *arpmac;
    struct arp_header *arpop;
    arprqip = (struct arp_header *)pkt_data;  
    arprpip = (struct arp_header *)pkt_data;
    arpmac = (struct arp_header *)pkt_data;
    arpop = (struct arp_header *)pkt_data;
    unsigned short arp_op_code = ntohs(arpop -> operation_code);  // ���ڷ� ���� ���� short �������� �ٲپ���
	unsigned short pro_type = ntohs(arpop -> protocol_type);  // ���ڷ� ���� ���� short �������� �ٲپ���	
    // request = 1
    if (arp_op_code == 0x0001) {  
        printf(" ******* request ******* \n");
        printf(" Sender IP : %s\n ", inet_ntoa(arprqip -> source_ip));  // ����Ʈ ������ 32��Ʈ ���� �ּҰ����� ��ȯ�ϱ� ����(in_addr �ʿ�)
        printf("Target IP : %s\n ", inet_ntoa(arprqip -> destination_ip));
		printf("pro_type : %d, hw_addr_len : %d, protocol_addr_len : %d \n",pro_type, arpop -> hw_addr_len,arpop -> protocol_addr_len);
        printf("\n");
    }
    // reply = 2
    if (arp_op_code == 0x0002) {  
        printf(" ********  reply  ******** \n");
        printf(" Sender IP  : %s\n ", inet_ntoa(arprpip -> source_ip));
        printf("Sender MAC : ");
			for (int i=0; i <=5; i ++) printf("%02x:",arpmac -> source_mac.mac_add[i]); // ��� ���� �ô� ��¹�...
        printf("\n");
        printf(" Target IP  : %s\n ", inet_ntoa(arprpip -> destination_ip));
        printf("Target MAC : ");
			for (int i=0; i <=5; i ++) printf("%02x:",arpmac -> source_mac.mac_add[i]);
        printf("\n");
		printf(" pro_type : %d, hw_addr_len : %d, protocol_addr_len : %d \n",pro_type, arpop -> hw_addr_len,arpop -> protocol_addr_len);

    }

}