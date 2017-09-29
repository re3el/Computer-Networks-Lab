#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/signal.h>

#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#define SIZE 100

#define BUF_SIZE 10000
#define maximum(a,b) (((a) > (b)) ? (a) : (b))

int flag_1 = 0, flag_2 = 0, flag_3 = 0, flag_4 = 0;
int alarm_flag = 0;
char src_ip_port[30];
char dst_ip_port[30];

// 		gcc overlayrouter.c -o overlayrouter ; ./overlayrouter 10024
int max(int a, int b, int c, int d, int e)
{
	int res;
	int tmp1 = (a > b?a:b);
	int tmp2 = (c > d?c:d);	
	int tmp3 = tmp1>tmp2?tmp1:tmp2;
	res = tmp3>e?tmp3:e;
	return res;
}

struct DataItem {
   char data[30];   
   char key[30];
};

struct DataItem* hashArray[SIZE]; 
struct DataItem* dummyItem;
struct DataItem* item;

int hashCode(char* key) {
	
	unsigned long int hashval =0;
	int i = 0;
	
	/* Convert our string to an integer */
	while( hashval < ULONG_MAX && i < strlen( key ) ) {
		//hashval = hashval << 8;
		hashval += key[ i ];
		i++;
		//printf("hashval: %ld,i: %d\n",hashval,i);
	}
	
	printf("key: %s, hash_key: %ld\n",key,hashval % SIZE);
	return hashval % SIZE;
	
   //return key % SIZE;
}

struct DataItem *search(char* key) {
   //get the hash 
   int hashIndex = hashCode(key);  
   //printf("inside search!: %d\n",hashIndex);
   //printf("hashArray[hashIndex]->key: %s, key: %s\n",hashArray[hashIndex]->key,key);
   //move in array until an empty 
   while(hashArray[hashIndex] != NULL) {
	
      if(strcmp(hashArray[hashIndex]->key,key)==0)
         return hashArray[hashIndex]; 
			
      //go to next cell
      ++hashIndex;
		
      //wrap around the table
      hashIndex %= SIZE;
   }        
	
   return NULL;        
}

void insert(char* key,char* data) {

   struct DataItem *item = (struct DataItem*) malloc(sizeof(struct DataItem));
   strcpy(item->data,data);     
   strcpy(item->key,key);
   item->data[strlen(item->data)]='\0';
   item->key[strlen(item->key)]='\0';

   //get the hash 
   int hashIndex = hashCode(key);

   //move in array until an empty or deleted cell
   while(hashArray[hashIndex] != NULL ) {
      //go to next cell
      ++hashIndex;
		
      //wrap around the table
      hashIndex %= SIZE;
   }
	
   hashArray[hashIndex] = item;
}

struct DataItem* delete(char* key) {
   
   //get the hash 
   int hashIndex = hashCode(key);

   //move in array until an empty
   while(hashArray[hashIndex] != NULL) {
	
      if(strcmp(hashArray[hashIndex]->key,key)==0) {
         struct DataItem* temp = hashArray[hashIndex]; 
			
         //assign a dummy item at deleted position
         hashArray[hashIndex] = NULL; 
         return temp;
      }
		
      //go to next cell
      ++hashIndex;
		
      //wrap around the table
      hashIndex %= SIZE;
   }      
	
   return NULL;        
} 

void display() {
   int i = 0;
	
   for(i = 0; i<SIZE; i++) {
	
      if(hashArray[i] != NULL)
         printf(" %s    -   %s\n",hashArray[i]->key,hashArray[i]->data);
      /* else
         printf(" ~~\n"); */
   }
	
   printf("\n");
}

static void timer_handler(int sig)
{
	if(alarm_flag == 1){
		//don't delete
		printf("dont delete\n");  
	}
	else
		printf("delete\n");  
}

int main(int argc, char *argv[])
{
	pid_t k;       												// signed integer for representing process_id	
	int status;    												// for storing the status information of the child process
	int len,n;	   												// buffer length		
	int option_val;      										// needed for setsockopt
	struct timeval t1,t2;										// gettimeofday() calculations
    unsigned int elapsedTime = 0;                               // to store diff of time values
	int totBytes = 0;                                           // variabe to store the totByte count
	unsigned int totPkts = 0;                                   // variable to store total packets count
	//int payload_size;											// variable to store payload_size
	
	// check the arguments
	if(argc!=2)
	{
		printf("Usage: %s portnumber \n",argv[0]);
		exit(1);
	}	
	
	// buffer array to receive input of size payload_size
	char buf[BUF_SIZE];  											

	// variables for socket	
	int sock_id1,sock_id2,sock_id3,sock_id4,sock_id5,port_no1,data_port,port_no3,cmp;
	struct sockaddr_in router,t1_addport,t2_addport,t3_addport,s1_addport,s2_addport,s3_addport,s4_addport,c1_addport,c2_addport,c3_addport,c4_addport,r_addport;		
	struct hostent *hostp; 
	bzero((char *) &t1_addport, sizeof(t1_addport));	
	bzero((char *) &t2_addport, sizeof(t2_addport));
			
	// Setup UDP Socket
	sock_id1 = socket(AF_INET, SOCK_DGRAM, 0);					// UDP Datagram
	if(sock_id1<0)
		perror("Error opening Socket\n");	

	sock_id2 = socket(AF_INET, SOCK_DGRAM, 0);					// UDP Datagram
	if(sock_id2<0)
		perror("Error opening Socket\n");	
	
	
	// setsockopt: to rerun the server immediately after killing it
	option_val = 1;
	setsockopt(sock_id1, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));
	
	option_val = 1;
	setsockopt(sock_id2, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));

	// binding the socket
	port_no1 = atoi(argv[1]);	
	t1_addport.sin_family = AF_INET;
	t1_addport.sin_port = htons(port_no1);
	t1_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id1, (struct sockaddr *) &t1_addport, sizeof(t1_addport))== -1) 
	{
		printf("Bind failed!\n");
	}
	
	srand(time(NULL));
    int randomnumber;	
	randomnumber = rand() % 100 + 20001;
	printf("data_port: %d\n", randomnumber);
	
	data_port = randomnumber; 	
	t2_addport.sin_family = AF_INET;
	t2_addport.sin_port = htons(data_port);
	t2_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id2, (struct sockaddr *) &t2_addport, sizeof(t2_addport))== -1) 
	{
		printf("Bind failed!\n");
	}	
	
	
	char self_ip[50];
	int fd;
	struct ifreq ifr;
	fd = socket(AF_INET, SOCK_DGRAM, 0);	
	ifr.ifr_addr.sa_family = AF_INET;	
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);	
	//printf("%s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	sprintf(self_ip,"%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	self_ip[strlen(self_ip)]='\0';
	printf("self_ip: %s\n",self_ip);
		
	while(1)
	{		
		fd_set socks;
		FD_ZERO(&socks);
		FD_SET(sock_id1, &socks);
		FD_SET(sock_id2, &socks);
				
		int nsocks = maximum(sock_id1,sock_id2) + 1;
		
		if (select(nsocks, &socks, (fd_set *)0, (fd_set *)0, 0) >= 0) 
		{
			bzero(buf,1000);
			
			// handle socket 1 - tunnel setup
			if (FD_ISSET(sock_id1, &socks)) 
			{
				socklen_t szaddr = sizeof(r_addport);
				n = recvfrom(sock_id1, buf, 1000, 0, (struct sockaddr *) &r_addport, &szaddr);
				if(n<0)
					perror("Error at server: recvfrom()!\n\n");				
				printf("Message received from src: %s\n",buf);
				
				sprintf(src_ip_port,"%s:%d",inet_ntoa(r_addport.sin_addr),ntohs(r_addport.sin_port));
				src_ip_port[strlen(src_ip_port)]='\0';
				printf("Src_IP_Port: %s\n",src_ip_port);
				
				char msgCopy[1000];
				bzero(msgCopy,1000);
				sprintf(msgCopy,"%s",buf);
				msgCopy[strlen(msgCopy)]='\0';
				//printf("msgCopy: %s\n",msgCopy);

				char *pch;
				char ip[50];
				pch = strtok (msgCopy,"$");		
				pch = strtok (NULL,"$");	
				pch = strtok (NULL,"$");	
				while(pch!=NULL)
				{
					//printf("ip: %s\n",pch);
					bzero(ip,50);
					sprintf(ip,"%s",pch);
					pch = strtok (NULL,"$");		
				}	
				ip[strlen(ip)]='\0';
				printf("Dst IP: %s\n",ip);
				//printf("Self IP: %s\n",inet_ntoa(t1_addport.sin_addr));			
				if(strcmp(self_ip,ip)==0)
				{
					
					// send port_number to previous hop					
					char conc_msg[40];
					//sprintf(conc_msg,"$%s$%s",msg,inet_ntoa(s1_addport.sin_addr));						
					sprintf(conc_msg,"%d",data_port);						
					conc_msg[strlen(conc_msg)]='\0';
					printf("data_port: %s\n",conc_msg);
					n = sendto(sock_id1, conc_msg, strlen(conc_msg), 0, (struct sockaddr *) &r_addport, sizeof(r_addport));
					if (n < 0) 
						perror("Error at Server: sendto()!\n");	
					
					char snd[1000];
					strncpy(snd,buf,strlen(buf)-strlen(ip)-1);
					snd[strlen(snd)]='\0';
					printf("Pkt for next Router: %s\n",snd);	
					
					if(strlen(snd)>21)
					{
						char msgSend[1000];
						bzero(msgSend,1000);
						sprintf(msgSend,"%s",snd);
						msgSend[strlen(msgSend)]='\0';
						//printf("msgSend: %s\n",msgSend);

						char *pch;
						char ip[50];
						pch = strtok (msgSend,"$");		
						pch = strtok (NULL,"$");	
						pch = strtok (NULL,"$");	
						while(pch!=NULL)
						{
							//printf("ip: %s\n",pch);
							bzero(ip,50);
							sprintf(ip,"%s",pch);
							pch = strtok (NULL,"$");		
						}	
						ip[strlen(ip)]='\0';
						printf("Next Router IP: %s\n",ip);					

						
						t3_addport.sin_family = AF_INET;
						t3_addport.sin_port = htons(port_no1);
						inet_aton(ip,&(t3_addport.sin_addr));
						
						n = sendto(sock_id2, snd, strlen(snd), 0, (struct sockaddr *) &t3_addport, sizeof(t3_addport));
						if (n < 0) 
							perror("Error at Server: sendto()!\n");	
						
						bzero(buf,1000);
						socklen_t szaddr = sizeof(t3_addport);
						n = recvfrom(sock_id2, buf, 1000, 0, (struct sockaddr *) &t3_addport, &szaddr);
						if(n<0)
							perror("Error at server: recvfrom()!\n\n");				
						//printf("data_port of next router: %s\n",buf);
						//printf("Next Router's IP: %s\n",inet_ntoa(t3_addport.sin_addr));
						printf("Packet's destination IP: %s, Packet's destination Port_Number: %s\n",inet_ntoa(t3_addport.sin_addr), buf);	
						
						sprintf(dst_ip_port,"%s:%s",inet_ntoa(t3_addport.sin_addr),buf);
						dst_ip_port[strlen(dst_ip_port)]='\0';
						printf("Dst_IP_Port: %s\n",dst_ip_port);
						
						insert(src_ip_port,dst_ip_port);
						insert(dst_ip_port,src_ip_port);
						
						display();
						signal(SIGALRM, timer_handler);
						alarm(30);
						
					}
					else
					{
						printf("I am the last router!\n");
					}
				}

			
				// TODO : Maintain a Table
						
			}
			
			// handle socket 2 - tunnel as VPN
			if (FD_ISSET(sock_id2, &socks)) 
			{	
				printf("\ninside sock2!!!\n\n");
				bzero(buf,1000);
				socklen_t szaddr = sizeof(r_addport);
				n = recvfrom(sock_id2, buf, 10000, 0, (struct sockaddr *) &r_addport, &szaddr);
				if(n<0)
					perror("Error at server: recvfrom()!\n\n");
				//printf("Message received from Client: %s\nMessage Length(bytes): %d\n",buf,n);
				
				hostp = gethostbyaddr((const char *)&r_addport.sin_addr.s_addr, sizeof(r_addport.sin_addr.s_addr), AF_INET);
				if(hostp == NULL)
				{
					perror("Error at gethostbyaddr()!\n");
					exit(1);
				}						
																	
				// send messages through tunnel-server from client
				if(s1_addport.sin_addr.s_addr != r_addport.sin_addr.s_addr) 
				{		
					//printf("Packet's destination IP: %s, Packet's destination Port_Number: %d\n",inet_ntoa(s1_addport.sin_addr), ntohs(s1_addport.sin_port));				
					//printf("send messages through tunnel-server from client!\n");
					c1_addport.sin_port = r_addport.sin_port;
					szaddr = sizeof(s1_addport);
					int n2 = sendto(sock_id2, buf, n, 0, (struct sockaddr *) &s1_addport, szaddr);
					if (n2 < 0) 
					{
					  perror("Error at Client: sendto()!\n");
					  exit(1);
					}
				}
				
				// send messages through tunnel-client from server
				if(s1_addport.sin_addr.s_addr == r_addport.sin_addr.s_addr) 
				{
					//printf("Packet's destination IP: %s, Packet's destination Port_Number: %d\n",inet_ntoa(c1_addport.sin_addr), ntohs(c1_addport.sin_port));				
					//printf("send messages through tunnel-client from server!\n");
					socklen_t szaddr = sizeof(c1_addport);
					int n2 = sendto(sock_id2, buf, n, 0, (struct sockaddr *) &c1_addport, szaddr);
					if (n2 < 0) 
					{
					  perror("Error at Client: sendto()!\n");
					  exit(1);
					}
				}	
				
				//flag_1 = 0;
				//printf("connection closed! flag_1:%d \n",flag_1);
									
			}
			
			
			
		}		
			
	}

	// close server_socket
	close(sock_id1);	
	close(sock_id2);		
	
	return 0;
}


// shouldn't the length be 21bytes?