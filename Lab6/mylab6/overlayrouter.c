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
char self_ip[50];
char src_ip_port[50];
char dst_ip_port[50];
char tgt_ip_port[50];

struct timeval t1, t2;
double elapsedTime;

int max(int a, int b, int c, int d)
{	
	int tmp1 = (a > b?a:b);
	int tmp2 = (c > d?c:d);	
	int tmp3 = tmp1>tmp2?tmp1:tmp2;	
	return tmp3;
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
	
	//printf("key: %s, hash_key: %ld\n",key,hashval % SIZE);
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
   
   printf("Delete: %s\n",key);
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
         printf("%s    -   %s\n",hashArray[i]->key,hashArray[i]->data);
      /* else
         printf(" ~~\n"); */
   }
	
   printf("\n\n");
}

static void timer_handler(int sig)
{
	if(alarm_flag == 0){
		//don't delete
		printf("No Confirmation received in 30secs! Deleting the respective entries\n");  
		delete(src_ip_port);
		delete(dst_ip_port);
		printf("\nRouting Table\n");
		display();
		
		//gettimeofday(&t2, NULL);	
		//elapsedTime = ((t2.tv_sec + t2.tv_usec/1000000) - (t1.tv_sec  + t1.tv_usec/1000000));		
		//printf("Completion Time(seconds): %f\n",((double)elapsedTime));	
		//printf("Self_IP: %s\n",self_ip);
	}	
}


// 		gcc overlayrouter.c -o overlayrouter ; ./overlayrouter 10024

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
	int sock_id1,sock_id2,sock_id3,sock_id4,sock_id5,port_no1,data_port1,data_port2,data_port3,port_no3,cmp;
	struct sockaddr_in router,t1_addport,t2_addport,t3_addport,t4_addport,t5_addport,s1_addport,r_addport,p_addport;		
	struct hostent *hostp; 
	bzero((char *) &t1_addport, sizeof(t1_addport));	
	bzero((char *) &t2_addport, sizeof(t2_addport));
	bzero((char *) &t3_addport, sizeof(t3_addport));	
	bzero((char *) &t4_addport, sizeof(t4_addport));	
			
	// Setup UDP Socket
	sock_id1 = socket(AF_INET, SOCK_DGRAM, 0);					// UDP Datagram
	if(sock_id1<0)
		perror("Error opening Socket\n");	

	sock_id2 = socket(AF_INET, SOCK_DGRAM, 0);					// UDP Datagram
	if(sock_id2<0)
		perror("Error opening Socket\n");	
	
	sock_id3 = socket(AF_INET, SOCK_DGRAM, 0);					// UDP Datagram
	if(sock_id3<0)
		perror("Error opening Socket\n");
	
	sock_id4 = socket(AF_INET, SOCK_DGRAM, 0);					// UDP Datagram
	if(sock_id4<0)
		perror("Error opening Socket\n");
	
	
	// setsockopt: to rerun the server immediately after killing it
	option_val = 1;
	setsockopt(sock_id1, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));	
	setsockopt(sock_id2, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));
	setsockopt(sock_id3, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));
	setsockopt(sock_id4, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));

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
	randomnumber = rand() % 1000 + 20101;	
	data_port1 = randomnumber; 	
	printf("data_port1: %d\n", data_port1);	
	t2_addport.sin_family = AF_INET;
	t2_addport.sin_port = htons(data_port1);
	t2_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id2, (struct sockaddr *) &t2_addport, sizeof(t2_addport))== -1) 
	{
		printf("Bind failed!\n");
	}	
	
	srand(time(NULL));    
	randomnumber = rand() % 1000 + 20201;	
	data_port2 = randomnumber; 	
	printf("data_port2: %d\n", data_port2);	
	t5_addport.sin_family = AF_INET;
	t5_addport.sin_port = htons(data_port2);
	t5_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id3, (struct sockaddr *) &t5_addport, sizeof(t5_addport))== -1) 
	{
		printf("Bind failed!\n");
	}
	
	srand(time(NULL));    
	randomnumber = rand() % 1000 + 20301;	
	data_port3 = randomnumber;
	printf("data_port3: %d\n", data_port3);	 	
	t4_addport.sin_family = AF_INET;
	t4_addport.sin_port = htons(data_port3);
	t4_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id4, (struct sockaddr *) &t4_addport, sizeof(t4_addport))== -1) 
	{
		printf("Bind failed!\n");
	}
	
	
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
	//printf("self_ip: %s\n",self_ip);
		
	while(1)
	{		
		fd_set socks;
		FD_ZERO(&socks);
		FD_SET(sock_id1, &socks);
		FD_SET(sock_id2, &socks);
		FD_SET(sock_id3, &socks);
		FD_SET(sock_id4, &socks);
				
		int nsocks = max(sock_id1,sock_id2,sock_id3,sock_id4) + 1;
		
		if (select(nsocks, &socks, (fd_set *)0, (fd_set *)0, 0) >= 0) 
		{
			bzero(buf,1000);
			
			// handle socket 1 - tunnel setup
			if (FD_ISSET(sock_id1, &socks)) 
			{			
				if(flag_1 == 0)
				{
					bzero((char *) &r_addport, sizeof(r_addport));
					socklen_t szaddr = sizeof(r_addport);					
					n = recvfrom(sock_id1, buf, 1000, 0, (struct sockaddr *) &r_addport, &szaddr);
					if(n<0)
						perror("Error at server: recvfrom()!\n\n");				
					printf("\nMessage received: %s\n",buf);
					
					char ip_port[50];
					sprintf(ip_port,"%s:%d",inet_ntoa(r_addport.sin_addr),ntohs(r_addport.sin_port));
					ip_port[strlen(ip_port)]='\0';
					//printf("Src ip_port: %s\n",ip_port);
					
					char chk[3];
					strncpy(chk,buf,2);
					chk[strlen(chk)]='\0';
					//printf("chk: %s\n",chk);
					if(strcmp(chk,"$$")!=0)
					{
						bzero(src_ip_port,50);
						sprintf(src_ip_port,"%s",ip_port);
						src_ip_port[strlen(src_ip_port)]='\0';					
						
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
						//printf("Dst IP: %s\n",ip);
						//printf("Self IP: %s\n",inet_ntoa(t1_addport.sin_addr));			
						if(strcmp(self_ip,ip)==0)
						{
							
							// send port_number to previous hop					
							char conc_msg[40];
							//sprintf(conc_msg,"$%s$%s",msg,inet_ntoa(s1_addport.sin_addr));						
							sprintf(conc_msg,"%d",data_port1);						
							conc_msg[strlen(conc_msg)]='\0';
							//printf("data_port1: %s\n",conc_msg);
							n = sendto(sock_id1, conc_msg, strlen(conc_msg), 0, (struct sockaddr *) &r_addport, sizeof(r_addport));
							if (n < 0) 
								perror("Error at Server: sendto()!\n");	
							
							char snd[1000];
							strncpy(snd,buf,strlen(buf)-strlen(ip)-1);
							snd[strlen(snd)]='\0';
							printf("Pkt for next Router: %s\n",snd);	
							
							if(strlen(snd)>21)
							{
								// msgSend: a copy of the message to be sent
								char msgSend[1000];
								bzero(msgSend,1000);
								sprintf(msgSend,"%s",snd);
								msgSend[strlen(msgSend)]='\0';
								//printf("msgSend: %s\n",msgSend);

								// ip: holds ip of the next Router
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
								//printf("Next Router IP: %s\n",ip);					

								// t3: addport of next_router_ip:server_port 
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
								//printf("data_port1 of next router: %s\n",buf);
								//printf("Next Router's IP: %s\n",inet_ntoa(t3_addport.sin_addr));
								//printf("Packet's destination IP: %s, Packet's destination Port_Number: %s\n",inet_ntoa(t3_addport.sin_addr), buf);	
								bzero(dst_ip_port,50);
								sprintf(dst_ip_port,"%s:%s",inet_ntoa(t3_addport.sin_addr),buf);
								dst_ip_port[strlen(dst_ip_port)]='\0';
								//printf("Dst_IP_Port: %s\n",dst_ip_port);
								
								// inserting into the table
								printf("src_ip_port: %s\n",src_ip_port);
								printf("dst_ip_port: %s\n",dst_ip_port);
								insert(src_ip_port,dst_ip_port);
								insert(dst_ip_port,src_ip_port);
								printf("\nRouting Table Before Confirmation:\n");
								display();
								gettimeofday(&t1, NULL);
								signal(SIGALRM, timer_handler);
								alarm(30);
								
							}
							else
							{
								printf("I am the last router!\n");
								char msgSend[1000];
								bzero(msgSend,1000);
								sprintf(msgSend,"%s",snd);
								msgSend[strlen(msgSend)]='\0';
								
								char *pch;
								char tgt_ip[50];
								char tgt_port[20];
								pch = strtok (msgSend,"$");	
								sprintf(tgt_ip,"%s",pch);
								tgt_ip[strlen(tgt_ip)]='\0';
								//printf("Target_IP: %s\n",tgt_ip);
								pch = strtok (NULL,"$");	
								sprintf(tgt_port,"%s",pch);
								tgt_port[strlen(tgt_port)]='\0';
								//printf("Target_Port: %s\n",tgt_port);						
								
								sprintf(tgt_ip_port,"%s:%s",tgt_ip,tgt_port);
								tgt_ip_port[strlen(tgt_ip_port)]='\0';
								insert(src_ip_port,tgt_ip_port);
								insert(tgt_ip_port,src_ip_port);
														
								// send $$routerk-IP$data-port-k$ to previous hop					
								char conf_msg[40];			
								sprintf(conf_msg,"$$%s$%d$",self_ip,data_port1);						
								conf_msg[strlen(conf_msg)]='\0';
								printf("Conf_Msg sent: %s\n",conf_msg);
								//printf("Packet's destination IP: %s, Packet's destination Port_Number: %d\n",inet_ntoa(r_addport.sin_addr), ntohs(r_addport.sin_port));	
								r_addport.sin_port = htons(port_no1);
								//printf("Packet's destination IP: %s, Packet's destination Port_Number: %d\n",inet_ntoa(r_addport.sin_addr), ntohs(r_addport.sin_port));	
								n = sendto(sock_id1, conf_msg, strlen(conf_msg), 0, (struct sockaddr *) &r_addport, sizeof(r_addport));
								if (n < 0) 
									perror("Error at Server: sendto()!\n");
								printf("\nRouting Table:\n");
								display();
								flag_1++;
							}
							
						}// end of self_ip=ip
						
					}
					else
					{	
						alarm_flag = 1;
						flag_1++;
						gettimeofday(&t2, NULL);
						printf("\nRouting Table After Confirmation:\n");
						
						char msgSend[1000];
						bzero(msgSend,1000);
						sprintf(msgSend,"%s",buf);
						msgSend[strlen(msgSend)]='\0';
						
						char *pch;
						char tgt_ip[50];
						char tgt_port[20];
						pch = strtok (msgSend,"$");	
						//pch = strtok (NULL,"$");
						sprintf(tgt_ip,"%s",pch);
						tgt_ip[strlen(tgt_ip)]='\0';
						//printf("Target_IP: %s\n",tgt_ip);
						pch = strtok (NULL,"$");	
						sprintf(tgt_port,"%s",pch);
						tgt_port[strlen(tgt_port)]='\0';
						//printf("Target_Port: %s\n",tgt_port);	
						
						//exit(1);
						char key[50];
						sprintf(key,"%s:%s",tgt_ip,tgt_port);
						key[strlen(key)]='\0';
						//printf("key: %s\n",key);
						item = search(key);
						if(item != NULL) {
						   //printf("Element found: %s\n", item->data);
						} else {
						   printf("Element not found\n");
						}
						
						char next_ip_port[50];
						sprintf(next_ip_port,"%s",item->data);
						next_ip_port[strlen(next_ip_port)]='\0';
						
						int i = 0;
						while(next_ip_port[i]!=':')
							i++;		
						char next_ip[i];
						strncpy(next_ip,&next_ip_port[0],i);	
						next_ip[i]='\0';							
						//printf("next_ip: %s, strlen:%zu\n",next_ip,strlen(next_ip));
						char next_port[20];
						strncpy(next_port,&next_ip_port[i+1],strlen(next_ip_port));
						next_port[strlen(next_port)]='\0';
						//printf("next_port: %s\n",next_port);
						
						bzero((char *) &p_addport, sizeof(p_addport));
						//int port_no = atoi(port_no1);	
						p_addport.sin_family = AF_INET;
						p_addport.sin_port = htons(port_no1);
						inet_aton(next_ip,&(p_addport.sin_addr));
						
						// send $$routerk-IP$data-port-k$ to previous hop					
						char conf_msg[40];					
						sprintf(conf_msg,"$$%s$%d$",self_ip,data_port1);						
						conf_msg[strlen(conf_msg)]='\0';
						//printf("Conf_Msg sent: %s\n",conf_msg);
																
						n = sendto(sock_id1, conf_msg, strlen(conf_msg), 0, (struct sockaddr *) &p_addport, sizeof(p_addport));
						if (n < 0) 
							perror("Error at Server: sendto()!\n");
						
						display();
						elapsedTime = ((t2.tv_sec * 1000000 + t2.tv_usec) - (t1.tv_sec * 1000000 + t1.tv_usec));
						printf("Completion Time(seconds): %f\n",((double)elapsedTime/1000000));
						printf("Self_IP: %s\n",self_ip);
					}// end of chk	
				}
				
				else if(flag_2 == 0)
				{
					alarm_flag = 0;
					gettimeofday(&t1, NULL);
					bzero((char *) &r_addport, sizeof(r_addport));
					socklen_t szaddr = sizeof(r_addport);
					n = recvfrom(sock_id1, buf, 1000, 0, (struct sockaddr *) &r_addport, &szaddr);
					if(n<0)
						perror("Error at server: recvfrom()!\n\n");				
					printf("\nMessage received: %s\n",buf);
					
					char ip_port[50];
					sprintf(ip_port,"%s:%d",inet_ntoa(r_addport.sin_addr),ntohs(r_addport.sin_port));
					ip_port[strlen(ip_port)]='\0';
					//printf("Src ip_port: %s\n",ip_port);
					
					char chk[3];
					strncpy(chk,buf,2);
					chk[strlen(chk)]='\0';
					//printf("chk: %s\n",chk);
					if(strcmp(chk,"$$")!=0)
					{
						bzero(src_ip_port,50);
						sprintf(src_ip_port,"%s",ip_port);
						src_ip_port[strlen(src_ip_port)]='\0';					
						
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
						//printf("Dst IP: %s\n",ip);
						//printf("Self IP: %s\n",inet_ntoa(t1_addport.sin_addr));			
						if(strcmp(self_ip,ip)==0)
						{
							
							// send port_number to previous hop					
							char conc_msg[40];
							//sprintf(conc_msg,"$%s$%s",msg,inet_ntoa(s1_addport.sin_addr));						
							sprintf(conc_msg,"%d",data_port2);						
							conc_msg[strlen(conc_msg)]='\0';
							//printf("data_port2: %s\n",conc_msg);
							n = sendto(sock_id1, conc_msg, strlen(conc_msg), 0, (struct sockaddr *) &r_addport, sizeof(r_addport));
							if (n < 0) 
								perror("Error at Server: sendto()!\n");	
							
							char snd[1000];
							strncpy(snd,buf,strlen(buf)-strlen(ip)-1);
							snd[strlen(snd)]='\0';
							printf("Pkt for next Router: %s\n",snd);	
							
							if(strlen(snd)>21)
							{
								// msgSend: a copy of the message to be sent
								char msgSend[1000];
								bzero(msgSend,1000);
								sprintf(msgSend,"%s",snd);
								msgSend[strlen(msgSend)]='\0';
								//printf("msgSend: %s\n",msgSend);

								// ip: holds ip of the next Router
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
								//printf("Next Router IP: %s\n",ip);					

								// t3: addport of next_router_ip:server_port 
								t3_addport.sin_family = AF_INET;
								t3_addport.sin_port = htons(port_no1);
								inet_aton(ip,&(t3_addport.sin_addr));						
								n = sendto(sock_id3, snd, strlen(snd), 0, (struct sockaddr *) &t3_addport, sizeof(t3_addport));
								if (n < 0) 
									perror("Error at Server: sendto()!\n");	
								
								bzero(buf,1000);
								socklen_t szaddr = sizeof(t3_addport);
								n = recvfrom(sock_id3, buf, 1000, 0, (struct sockaddr *) &t3_addport, &szaddr);
								if(n<0)
									perror("Error at server: recvfrom()!\n\n");				
								//printf("data_port2 of next router: %s\n",buf);
								//printf("Next Router's IP: %s\n",inet_ntoa(t3_addport.sin_addr));
								//printf("Packet's destination IP: %s, Packet's destination Port_Number: %s\n",inet_ntoa(t3_addport.sin_addr), buf);	
								bzero(dst_ip_port,50);
								sprintf(dst_ip_port,"%s:%s",inet_ntoa(t3_addport.sin_addr),buf);
								dst_ip_port[strlen(dst_ip_port)]='\0';
								//printf("Dst_IP_Port: %s\n",dst_ip_port);
								
								// inserting into the table
								printf("src_ip_port: %s\n",src_ip_port);
								printf("dst_ip_port: %s\n",dst_ip_port);
								insert(src_ip_port,dst_ip_port);
								insert(dst_ip_port,src_ip_port);
								printf("\nRouting Table Before Confirmation:\n");
								display();
								gettimeofday(&t1, NULL);
								signal(SIGALRM, timer_handler);
								alarm(30);
								
							}
							else
							{
								printf("I am the last router!\n");
								char msgSend[1000];
								bzero(msgSend,1000);
								sprintf(msgSend,"%s",snd);
								msgSend[strlen(msgSend)]='\0';
								
								char *pch;
								char tgt_ip[50];
								char tgt_port[20];
								pch = strtok (msgSend,"$");	
								sprintf(tgt_ip,"%s",pch);
								tgt_ip[strlen(tgt_ip)]='\0';
								//printf("Target_IP: %s\n",tgt_ip);
								pch = strtok (NULL,"$");	
								sprintf(tgt_port,"%s",pch);
								tgt_port[strlen(tgt_port)]='\0';
								//printf("Target_Port: %s\n",tgt_port);						
								
								sprintf(tgt_ip_port,"%s:%s",tgt_ip,tgt_port);
								tgt_ip_port[strlen(tgt_ip_port)]='\0';
								insert(src_ip_port,tgt_ip_port);
								insert(tgt_ip_port,src_ip_port);
														
								// send $$routerk-IP$data-port-k$ to previous hop					
								char conf_msg[40];			
								sprintf(conf_msg,"$$%s$%d$",self_ip,data_port2);						
								conf_msg[strlen(conf_msg)]='\0';
								printf("Conf_Msg sent: %s\n",conf_msg);
								//printf("Packet's destination IP: %s, Packet's destination Port_Number: %d\n",inet_ntoa(r_addport.sin_addr), ntohs(r_addport.sin_port));	
								r_addport.sin_port = htons(port_no1);
								//printf("Packet's destination IP: %s, Packet's destination Port_Number: %d\n",inet_ntoa(r_addport.sin_addr), ntohs(r_addport.sin_port));	
								n = sendto(sock_id1, conf_msg, strlen(conf_msg), 0, (struct sockaddr *) &r_addport, sizeof(r_addport));
								if (n < 0) 
									perror("Error at Server: sendto()!\n");
								printf("\nRouting Table:\n");
								display();
								flag_2++;
							}
							
						}// end of self_ip=ip
						
					}
					else
					{	
						alarm_flag = 1;
						flag_2++;
						gettimeofday(&t2, NULL);
						printf("\nRouting Table After Confirmation:\n");
						
						char msgSend[1000];
						bzero(msgSend,1000);
						sprintf(msgSend,"%s",buf);
						msgSend[strlen(msgSend)]='\0';
						
						char *pch;
						char tgt_ip[50];
						char tgt_port[20];
						pch = strtok (msgSend,"$");	
						//pch = strtok (NULL,"$");
						sprintf(tgt_ip,"%s",pch);
						tgt_ip[strlen(tgt_ip)]='\0';
						//printf("Target_IP: %s\n",tgt_ip);
						pch = strtok (NULL,"$");	
						sprintf(tgt_port,"%s",pch);
						tgt_port[strlen(tgt_port)]='\0';
						//printf("Target_Port: %s\n",tgt_port);	
						
						//exit(1);
						char key[50];
						sprintf(key,"%s:%s",tgt_ip,tgt_port);
						key[strlen(key)]='\0';
						//printf("key: %s\n",key);
						item = search(key);
						if(item != NULL) {
						   //printf("Element found: %s\n", item->data);
						} else {
						   printf("Element not found\n");
						}
						
						char next_ip_port[50];
						sprintf(next_ip_port,"%s",item->data);
						next_ip_port[strlen(next_ip_port)]='\0';
						
						int i = 0;
						while(next_ip_port[i]!=':')
							i++;		
						char next_ip[i];
						strncpy(next_ip,&next_ip_port[0],i);	
						next_ip[i]='\0';							
						//printf("next_ip: %s, strlen:%zu\n",next_ip,strlen(next_ip));
						char next_port[20];
						strncpy(next_port,&next_ip_port[i+1],strlen(next_ip_port));
						next_port[strlen(next_port)]='\0';
						//printf("next_port: %s\n",next_port);
						
						bzero((char *) &p_addport, sizeof(p_addport));
						//int port_no = atoi(port_no1);	
						p_addport.sin_family = AF_INET;
						p_addport.sin_port = htons(port_no1);
						inet_aton(next_ip,&(p_addport.sin_addr));
						
						// send $$routerk-IP$data-port-k$ to previous hop					
						char conf_msg[40];					
						sprintf(conf_msg,"$$%s$%d$",self_ip,data_port2);						
						conf_msg[strlen(conf_msg)]='\0';
						//printf("Conf_Msg sent: %s\n",conf_msg);
																
						n = sendto(sock_id1, conf_msg, strlen(conf_msg), 0, (struct sockaddr *) &p_addport, sizeof(p_addport));
						if (n < 0) 
							perror("Error at Server: sendto()!\n");
						
						display();
						elapsedTime = ((t2.tv_sec * 1000000 + t2.tv_usec) - (t1.tv_sec * 1000000 + t1.tv_usec));
						printf("Completion Time(seconds): %f\n",((double)elapsedTime/1000000));
						printf("Self_IP: %s\n",self_ip);
					}// end of chk	
				}
				
				
				else if(flag_3 == 0)
				{
					gettimeofday(&t1, NULL);
					alarm_flag = 0;
					bzero((char *) &r_addport, sizeof(r_addport));
					socklen_t szaddr = sizeof(r_addport);
					n = recvfrom(sock_id1, buf, 1000, 0, (struct sockaddr *) &r_addport, &szaddr);
					if(n<0)
						perror("Error at server: recvfrom()!\n\n");				
					printf("\nMessage received: %s\n",buf);
					
					char ip_port[50];
					sprintf(ip_port,"%s:%d",inet_ntoa(r_addport.sin_addr),ntohs(r_addport.sin_port));
					ip_port[strlen(ip_port)]='\0';
					//printf("Src ip_port: %s\n",ip_port);
					
					char chk[3];
					strncpy(chk,buf,2);
					chk[strlen(chk)]='\0';
					//printf("chk: %s\n",chk);
					if(strcmp(chk,"$$")!=0)
					{
						bzero(src_ip_port,50);
						sprintf(src_ip_port,"%s",ip_port);
						src_ip_port[strlen(src_ip_port)]='\0';					
						
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
						//printf("Dst IP: %s\n",ip);
						//printf("Self IP: %s\n",inet_ntoa(t1_addport.sin_addr));			
						if(strcmp(self_ip,ip)==0)
						{
							
							// send port_number to previous hop					
							char conc_msg[40];
							//sprintf(conc_msg,"$%s$%s",msg,inet_ntoa(s1_addport.sin_addr));						
							sprintf(conc_msg,"%d",data_port3);						
							conc_msg[strlen(conc_msg)]='\0';
							//printf("data_port3: %s\n",conc_msg);
							n = sendto(sock_id1, conc_msg, strlen(conc_msg), 0, (struct sockaddr *) &r_addport, sizeof(r_addport));
							if (n < 0) 
								perror("Error at Server: sendto()!\n");	
							
							char snd[1000];
							strncpy(snd,buf,strlen(buf)-strlen(ip)-1);
							snd[strlen(snd)]='\0';
							printf("Pkt for next Router: %s\n",snd);	
							
							if(strlen(snd)>21)
							{
								// msgSend: a copy of the message to be sent
								char msgSend[1000];
								bzero(msgSend,1000);
								sprintf(msgSend,"%s",snd);
								msgSend[strlen(msgSend)]='\0';
								//printf("msgSend: %s\n",msgSend);

								// ip: holds ip of the next Router
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
								//printf("Next Router IP: %s\n",ip);					

								// t3: addport of next_router_ip:server_port 
								t3_addport.sin_family = AF_INET;
								t3_addport.sin_port = htons(port_no1);
								inet_aton(ip,&(t3_addport.sin_addr));						
								n = sendto(sock_id4, snd, strlen(snd), 0, (struct sockaddr *) &t3_addport, sizeof(t3_addport));
								if (n < 0) 
									perror("Error at Server: sendto()!\n");	
								
								bzero(buf,1000);
								socklen_t szaddr = sizeof(t3_addport);
								n = recvfrom(sock_id4, buf, 1000, 0, (struct sockaddr *) &t3_addport, &szaddr);
								if(n<0)
									perror("Error at server: recvfrom()!\n\n");				
								//printf("data_port3 of next router: %s\n",buf);
								//printf("Next Router's IP: %s\n",inet_ntoa(t3_addport.sin_addr));
								//printf("Packet's destination IP: %s, Packet's destination Port_Number: %s\n",inet_ntoa(t3_addport.sin_addr), buf);	
								bzero(dst_ip_port,50);
								sprintf(dst_ip_port,"%s:%s",inet_ntoa(t3_addport.sin_addr),buf);
								dst_ip_port[strlen(dst_ip_port)]='\0';
								//printf("Dst_IP_Port: %s\n",dst_ip_port);
								
								// inserting into the table
								printf("src_ip_port: %s\n",src_ip_port);
								printf("dst_ip_port: %s\n",dst_ip_port);
								insert(src_ip_port,dst_ip_port);
								insert(dst_ip_port,src_ip_port);
								printf("\nRouting Table Before Confirmation:\n");
								display();
								gettimeofday(&t1, NULL);
								signal(SIGALRM, timer_handler);
								alarm(30);
								
							}
							else
							{
								printf("I am the last router!\n");
								char msgSend[1000];
								bzero(msgSend,1000);
								sprintf(msgSend,"%s",snd);
								msgSend[strlen(msgSend)]='\0';
								
								char *pch;
								char tgt_ip[50];
								char tgt_port[20];
								pch = strtok (msgSend,"$");	
								sprintf(tgt_ip,"%s",pch);
								tgt_ip[strlen(tgt_ip)]='\0';
								//printf("Target_IP: %s\n",tgt_ip);
								pch = strtok (NULL,"$");	
								sprintf(tgt_port,"%s",pch);
								tgt_port[strlen(tgt_port)]='\0';
								//printf("Target_Port: %s\n",tgt_port);						
								
								sprintf(tgt_ip_port,"%s:%s",tgt_ip,tgt_port);
								tgt_ip_port[strlen(tgt_ip_port)]='\0';
								insert(src_ip_port,tgt_ip_port);
								insert(tgt_ip_port,src_ip_port);
														
								// send $$routerk-IP$data-port-k$ to previous hop					
								char conf_msg[40];			
								sprintf(conf_msg,"$$%s$%d$",self_ip,data_port3);						
								conf_msg[strlen(conf_msg)]='\0';
								printf("Conf_Msg sent: %s\n",conf_msg);
								//printf("Packet's destination IP: %s, Packet's destination Port_Number: %d\n",inet_ntoa(r_addport.sin_addr), ntohs(r_addport.sin_port));	
								r_addport.sin_port = htons(port_no1);
								//printf("Packet's destination IP: %s, Packet's destination Port_Number: %d\n",inet_ntoa(r_addport.sin_addr), ntohs(r_addport.sin_port));	
								n = sendto(sock_id1, conf_msg, strlen(conf_msg), 0, (struct sockaddr *) &r_addport, sizeof(r_addport));
								if (n < 0) 
									perror("Error at Server: sendto()!\n");
								printf("\nRouting Table:\n");
								display();
								flag_3++;
							}
							
						}// end of self_ip=ip
						
					}
					else
					{	
						alarm_flag = 1;
						flag_3++;
						gettimeofday(&t2, NULL);
						printf("\nRouting Table After Confirmation:\n");
						
						char msgSend[1000];
						bzero(msgSend,1000);
						sprintf(msgSend,"%s",buf);
						msgSend[strlen(msgSend)]='\0';
						
						char *pch;
						char tgt_ip[50];
						char tgt_port[20];
						pch = strtok (msgSend,"$");	
						//pch = strtok (NULL,"$");
						sprintf(tgt_ip,"%s",pch);
						tgt_ip[strlen(tgt_ip)]='\0';
						//printf("Target_IP: %s\n",tgt_ip);
						pch = strtok (NULL,"$");	
						sprintf(tgt_port,"%s",pch);
						tgt_port[strlen(tgt_port)]='\0';
						//printf("Target_Port: %s\n",tgt_port);	
						
						//exit(1);
						char key[50];
						sprintf(key,"%s:%s",tgt_ip,tgt_port);
						key[strlen(key)]='\0';
						//printf("key: %s\n",key);
						item = search(key);
						if(item != NULL) {
						   //printf("Element found: %s\n", item->data);
						} else {
						   printf("Element not found\n");
						}
						
						char next_ip_port[50];
						sprintf(next_ip_port,"%s",item->data);
						next_ip_port[strlen(next_ip_port)]='\0';
						
						int i = 0;
						while(next_ip_port[i]!=':')
							i++;		
						char next_ip[i];
						strncpy(next_ip,&next_ip_port[0],i);	
						next_ip[i]='\0';							
						//printf("next_ip: %s, strlen:%zu\n",next_ip,strlen(next_ip));
						char next_port[20];
						strncpy(next_port,&next_ip_port[i+1],strlen(next_ip_port));
						next_port[strlen(next_port)]='\0';
						//printf("next_port: %s\n",next_port);
						
						bzero((char *) &p_addport, sizeof(p_addport));
						//int port_no = atoi(port_no1);	
						p_addport.sin_family = AF_INET;
						p_addport.sin_port = htons(port_no1);
						inet_aton(next_ip,&(p_addport.sin_addr));
						
						// send $$routerk-IP$data-port-k$ to previous hop					
						char conf_msg[40];					
						sprintf(conf_msg,"$$%s$%d$",self_ip,data_port3);						
						conf_msg[strlen(conf_msg)]='\0';
						//printf("Conf_Msg sent: %s\n",conf_msg);
																
						n = sendto(sock_id1, conf_msg, strlen(conf_msg), 0, (struct sockaddr *) &p_addport, sizeof(p_addport));
						if (n < 0) 
							perror("Error at Server: sendto()!\n");
						
						display();
						elapsedTime = ((t2.tv_sec * 1000000 + t2.tv_usec) - (t1.tv_sec * 1000000 + t1.tv_usec));
						printf("Completion Time(seconds): %f\n",((double)elapsedTime/1000000));
						printf("Self_IP: %s\n",self_ip);
					}// end of chk	
				}
							
			}
			
			// handle socket 2 - tunnel as VPN
			if (FD_ISSET(sock_id2, &socks)) 
			{	
				//printf("\ninside sock2!!!\n\n");
				char recv[1000];
				bzero(recv,1000);
				bzero((char *) &r_addport, sizeof(r_addport));
				socklen_t szaddr = sizeof(r_addport);
				n = recvfrom(sock_id2, recv, 1000, 0, (struct sockaddr *) &r_addport, &szaddr);
				if(n<0)
					perror("Error at server: recvfrom()!\n\n");
				printf("Message received from Client: %s\nMessage Length(bytes): %d\n",recv,n);							
																	
														
				char key[50];
				sprintf(key,"%s:%d",inet_ntoa(r_addport.sin_addr),ntohs(r_addport.sin_port));
				key[strlen(key)]='\0';
				//printf("key: %s\n",key);
				item = search(key);
				if(item != NULL) {
				   //printf("Forward to: %s\n", item->data);
				} else {
				   printf("Element not found\n");
				   exit(1);
				}
				
				char next_ip_port[50];
				sprintf(next_ip_port,"%s",item->data);
				next_ip_port[strlen(next_ip_port)]='\0';
				
				int i = 0;
				while(next_ip_port[i]!=':')
					i++;		
				char next_ip[i];
				strncpy(next_ip,&next_ip_port[0],i);	
				next_ip[i]='\0';							
				//printf("next_ip: %s, strlen:%zu\n",next_ip,strlen(next_ip));
				char next_port[20];
				strncpy(next_port,&next_ip_port[i+1],strlen(next_ip_port));
				next_port[strlen(next_port)]='\0';
				//printf("next_port: %s\n",next_port);
				
				bzero((char *) &p_addport, sizeof(p_addport));
				int port_no = atoi(next_port);	
				p_addport.sin_family = AF_INET;
				p_addport.sin_port = htons(port_no);
				inet_aton(next_ip,&(p_addport.sin_addr));
				szaddr = sizeof(p_addport);
				int n2 = sendto(sock_id2, recv, n, 0, (struct sockaddr *) &p_addport, szaddr);
				if (n2 < 0) 
				{
				  perror("Error at Client: sendto()!\n");
				  exit(1);
				}
			
				//flag_1 = 0;
				//printf("connection closed! flag_1:%d \n",flag_1);
									
			}
			
			if (FD_ISSET(sock_id3, &socks)) 
			{	
				//printf("\ninside sock2!!!\n\n");
				char recv[1000];
				bzero(recv,1000);
				bzero((char *) &r_addport, sizeof(r_addport));
				socklen_t szaddr = sizeof(r_addport);
				n = recvfrom(sock_id3, recv, 1000, 0, (struct sockaddr *) &r_addport, &szaddr);
				if(n<0)
					perror("Error at server: recvfrom()!\n\n");
				printf("Message received from Client: %s\nMessage Length(bytes): %d\n",recv,n);							
																	
														
				char key[50];
				sprintf(key,"%s:%d",inet_ntoa(r_addport.sin_addr),ntohs(r_addport.sin_port));
				key[strlen(key)]='\0';
				//printf("key: %s\n",key);
				item = search(key);
				if(item != NULL) {
				   //printf("Forward to: %s\n", item->data);
				} else {
				   printf("Element not found\n");
				   exit(1);
				}
				
				char next_ip_port[50];
				sprintf(next_ip_port,"%s",item->data);
				next_ip_port[strlen(next_ip_port)]='\0';
				
				int i = 0;
				while(next_ip_port[i]!=':')
					i++;		
				char next_ip[i];
				strncpy(next_ip,&next_ip_port[0],i);	
				next_ip[i]='\0';							
				//printf("next_ip: %s, strlen:%zu\n",next_ip,strlen(next_ip));
				char next_port[20];
				strncpy(next_port,&next_ip_port[i+1],strlen(next_ip_port));
				next_port[strlen(next_port)]='\0';
				//printf("next_port: %s\n",next_port);
				
				bzero((char *) &p_addport, sizeof(p_addport));
				int port_no = atoi(next_port);	
				p_addport.sin_family = AF_INET;
				p_addport.sin_port = htons(port_no);
				inet_aton(next_ip,&(p_addport.sin_addr));
				szaddr = sizeof(p_addport);
				int n2 = sendto(sock_id3, recv, n, 0, (struct sockaddr *) &p_addport, szaddr);
				if (n2 < 0) 
				{
				  perror("Error at Client: sendto()!\n");
				  exit(1);
				}
			
				//flag_1 = 0;
				//printf("connection closed! flag_1:%d \n",flag_1);
									
			}
			
			if (FD_ISSET(sock_id4, &socks)) 
			{	
				//printf("\ninside sock2!!!\n\n");
				char recv[1000];
				bzero(recv,1000);
				bzero((char *) &r_addport, sizeof(r_addport));
				socklen_t szaddr = sizeof(r_addport);
				n = recvfrom(sock_id4, recv, 1000, 0, (struct sockaddr *) &r_addport, &szaddr);
				if(n<0)
					perror("Error at server: recvfrom()!\n\n");
				printf("Message received from Client: %s\nMessage Length(bytes): %d\n",recv,n);							
																	
														
				char key[50];
				sprintf(key,"%s:%d",inet_ntoa(r_addport.sin_addr),ntohs(r_addport.sin_port));
				key[strlen(key)]='\0';
				//printf("key: %s\n",key);
				item = search(key);
				if(item != NULL) {
				   //printf("Forward to: %s\n", item->data);
				} else {
				   printf("Element not found\n");
				   exit(1);
				}
				
				char next_ip_port[50];
				sprintf(next_ip_port,"%s",item->data);
				next_ip_port[strlen(next_ip_port)]='\0';
				
				int i = 0;
				while(next_ip_port[i]!=':')
					i++;		
				char next_ip[i];
				strncpy(next_ip,&next_ip_port[0],i);	
				next_ip[i]='\0';							
				//printf("next_ip: %s, strlen:%zu\n",next_ip,strlen(next_ip));
				char next_port[20];
				strncpy(next_port,&next_ip_port[i+1],strlen(next_ip_port));
				next_port[strlen(next_port)]='\0';
				//printf("next_port: %s\n",next_port);
				
				bzero((char *) &p_addport, sizeof(p_addport));
				int port_no = atoi(next_port);	
				p_addport.sin_family = AF_INET;
				p_addport.sin_port = htons(port_no);
				inet_aton(next_ip,&(p_addport.sin_addr));
				szaddr = sizeof(p_addport);
				int n2 = sendto(sock_id4, recv, n, 0, (struct sockaddr *) &p_addport, szaddr);
				if (n2 < 0) 
				{
				  perror("Error at Client: sendto()!\n");
				  exit(1);
				}
			
				//flag_1 = 0;
				//printf("connection closed! flag_1:%d \n",flag_1);
									
			}
			
			
			
		}		
			
	}

	// close server_socket
	close(sock_id1);	
	close(sock_id2);
	close(sock_id3);	
	close(sock_id4);		
	
	return 0;
}


// shouldn't the length be 21bytes?
// should we print after/before confirmation?