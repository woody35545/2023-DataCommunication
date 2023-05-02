#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h> // packet capture api
// ARP 패킷 캡처후 패킷 구조 확인을 위한 파일입니다. (데이터 통신 실습시간)
// ARP 헤더 정의후 자신의 네트워크 상의 ARP 패킷을 캡처하여 데이터를 확인해봅니다.
struct ether_addr {
  unsigned char mac_add[6];  
};

struct ether_header {
    struct ether_addr etherdst_mac; 
    struct ether_addr ethersed_mac; 
    unsigned short ether_type; // ARP인지 확인 할 수 있는 부분 0x0806=ARP 0x0800=IP // short=2byte
};

#pragma pack(push, 2)  // 구조체 크기 정렬하기 = 2바이트 단위로 메모리 낭비없이 저장하기 위함.
struct arp_header {
    unsigned short hw_type; // 사용 가능한 전체 물리주소(MAC) 유형
    unsigned short protocol_type; // 사용중인 프로토콜 주소 
    unsigned char hw_addr_len; // 패킷에 사용되는 MAC 길이
    unsigned char protocol_addr_len; // 패킷에 사용되는 IP 주소 길이
    unsigned short operation_code; // 요청(1) 또는 응답(2)
    struct ether_addr source_mac; // Sender L2 계층의 물리주소
    struct in_addr source_ip; // Sender L3 계층의 논리 주소 
    struct ether_addr destination_mac; // Receiver L2 주소
    struct in_addr destination_ip; // Receiver L3 논리 주소
};
#pragma pack(pop)

// 함수 선언
void print_ether_header(const unsigned char *pkt_data);
void print_arp_header(const unsigned char *pkt_data);

// main
int main() {
  pcap_if_t *alldevs;   // 포인터 alldevs의 자료형은 pcap_if_t
  pcap_if_t *d;
  int inum,res,i=0;
  struct pcap_pkthdr *header;  //pcap_pkthdr 구조체 : 
  const unsigned char *pkt_data;  //패킷을 저장할 공간
  pcap_t *adhandle;
  char errbuf[PCAP_ERRBUF_SIZE];
  char packet_filter[] = ""; // 사용자가 원하는 프로토콜 필터 정보를 넣을 수 있는 공간
  struct bpf_program fcode; // 특정 프로토콜만을 캡쳐하기 위한 정책정보 저장

  // alldevs 에 디바이스 목록 저장,에러시 errbuf에 에러저장
  if(pcap_findalldevs(&alldevs, errbuf) == -1) {   
    printf("Error in pcap_findalldevs: %s\n", errbuf);
    exit(1);
  }

  for(d=alldevs; d; d=d->next) {  //네트워트 디바이스 정보를 출력
    printf("%d. %s", ++i, d->name);
    if (d->description)
      printf(" (%s)\n", d->description);
    else
      printf(" (No description available)\n");
  }
  //디바이스 못찾을 경우 에러
  if(i==0) {   
    printf("\nNo interfaces found! Make sure LiPcap is installed.\n");
    //return -1;
  }

  printf("Enter the interface number (1-%d):",i);
  scanf("%d", &inum);
  //입력한 값이 올바른지 판단 && : 둘다 참이어야 참
  if(inum < 1 || inum > i) {  
    printf("\nAdapter number out of range.\n");
    pcap_freealldevs(alldevs);  
    return -1;
  }
  //사용자가 선택한 장치 목록을 선택
  for(d=alldevs, i=0; i< inum-1 ;d=d->next, i++);   

  //실제 네트워크 디바이스 오픈
  if((adhandle= pcap_open_live(d->name, 65536,   1,  1000,  errbuf  )) == NULL) {   
    printf("\nUnable to open the adapter. %s is not supported by pcap\n", d->name);
    pcap_freealldevs(alldevs);
    return -1;
  }
 
 //패킷 필터링 정책을 위해 pcap_compile()함수 호출 
 //사용자가 정의한 필터링 룰을 bpf_program 구조체에 저장하여 특정 프로토콜 패킷만 수집
  if (pcap_compile(adhandle, &fcode, packet_filter, 1, 0) <0 )  { 
    printf("\nUnable to compile the packet filter. Check the syntax.\n");
    pcap_freealldevs(alldevs);
    return -1;
  }

  //pcap_compile() 함수내용을 적용하기 위해  pcap_setfilter() 함수가 사용된다.
  if (pcap_setfilter(adhandle, &fcode)<0)  {  
    printf("\nError setting the filter.\n");
    pcap_freealldevs(alldevs);
    return -1;
  }
  // 디바이스 정보 출력
  printf("\nlistening on %s...\n", d->name);    
  pcap_freealldevs(alldevs);   // 해제

    while ((res = pcap_next_ex(adhandle, &header, &pkt_data)) >= 0) {
        if (res == 0) continue;
        print_ether_header(pkt_data);
        pkt_data += 14;
        print_arp_header(pkt_data);
    }
    
  return 0;
}

// 이더넷 정보를 출력함
void print_ether_header(const unsigned char *pkt_data) {  

    struct ether_header *eth; //이더넷 헤더 정보를 담을 수 있는 공간의 이더헤더의 구조체를 eth로 지정
    eth = (struct ether_header *)pkt_data;  // 구조체 eth에 패킷 정보를 저장
    unsigned short eth_type;
    eth_type= ntohs(eth->ether_type);  // 인자로 받은 값을 리틀 엔디안 형식으로 바꾸어줌
    // ARP 패킷인 부분만 캡처
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
// ARP 패킷 정보를 출력
void print_arp_header(const unsigned char *pkt_data) {  

    struct arp_header *arprqip;
    struct arp_header *arprpip;
    struct arp_header *arpmac;
    struct arp_header *arpop;
    arprqip = (struct arp_header *)pkt_data;  
    arprpip = (struct arp_header *)pkt_data;
    arpmac = (struct arp_header *)pkt_data;
    arpop = (struct arp_header *)pkt_data;
    unsigned short arp_op_code = ntohs(arpop -> operation_code);  // 인자로 받은 값을 short 형식으로 바꾸어줌
	unsigned short pro_type = ntohs(arpop -> protocol_type);  // 인자로 받은 값을 short 형식으로 바꾸어줌	
    // request = 1
    if (arp_op_code == 0x0001) {  
        printf(" ******* request ******* \n");
        printf(" Sender IP : %s\n ", inet_ntoa(arprqip -> source_ip));  // 바이트 순서의 32비트 값을 주소값으로 변환하기 위함(in_addr 필요)
        printf("Target IP : %s\n ", inet_ntoa(arprqip -> destination_ip));
		printf("pro_type : %d, hw_addr_len : %d, protocol_addr_len : %d \n",pro_type, arpop -> hw_addr_len,arpop -> protocol_addr_len);
        printf("\n");
    }
    // reply = 2
    if (arp_op_code == 0x0002) {  
        printf(" ********  reply  ******** \n");
        printf(" Sender IP  : %s\n ", inet_ntoa(arprpip -> source_ip));
        printf("Sender MAC : ");
			for (int i=0; i <=5; i ++) printf("%02x:",arpmac -> source_mac.mac_add[i]); // 어디서 많이 봤던 출력문...
        printf("\n");
        printf(" Target IP  : %s\n ", inet_ntoa(arprpip -> destination_ip));
        printf("Target MAC : ");
			for (int i=0; i <=5; i ++) printf("%02x:",arpmac -> source_mac.mac_add[i]);
        printf("\n");
		printf(" pro_type : %d, hw_addr_len : %d, protocol_addr_len : %d \n",pro_type, arpop -> hw_addr_len,arpop -> protocol_addr_len);

    }

}