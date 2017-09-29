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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define BUF_SIZE 10000
#define max(a,b) (((a) > (b)) ? (a) : (b))

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
	
	// assign payload_size
	//payload_size = atoi(argv[2]);
	
	// buffer array to receive input of size payload_size
	char buf[BUF_SIZE];  											

	// variables for socket	
	int sock_id1,sock_id2,port_no1,port_no2,port_no3,cmp;
	struct sockaddr_in s1_addport,s2_addport,s3_addport,s4_addport,r_addport;		
	struct hostent *hostp; 
	bzero((char *) &s1_addport, sizeof(s1_addport));	
	bzero((char *) &s2_addport, sizeof(s2_addport));
	bzero((char *) &s3_addport, sizeof(s3_addport));
	bzero((char *) &r_addport, sizeof(r_addport));
			
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
	setsockopt(sock_id2, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));
	
	// binding the socket
	port_no1 = atoi(argv[1]);	
	s1_addport.sin_family = AF_INET;
	s1_addport.sin_port = htons(port_no1);
	s1_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id1, (struct sockaddr *) &s1_addport, sizeof(s1_addport))== -1) 
	{
		printf("Bind failed!\n");
	}
	
	port_no2 = 20012; 	
	s2_addport.sin_family = AF_INET;
	s2_addport.sin_port = htons(port_no2);
	s2_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id2, (struct sockaddr *) &s2_addport, sizeof(s2_addport))== -1) 
	{
		printf("Bind failed!\n");
	}
	
	
	
	
	while(1)
	{		
		fd_set socks;
		FD_ZERO(&socks);
		FD_SET(sock_id1, &socks);
		FD_SET(sock_id2, &socks);
		
		int nsocks = max(sock_id1, sock_id2) + 1;
		if (select(nsocks, &socks, (fd_set *)0, (fd_set *)0, 0) >= 0) 
		{
			bzero(buf,10000);
			
			// handle socket 1 - tunnel setup
			if (FD_ISSET(sock_id1, &socks)) 
			{
				socklen_t szaddr = sizeof(r_addport);
				n = recvfrom(sock_id1, buf, 10000, 0, (struct sockaddr *) &r_addport, &szaddr);
				if(n<0)
					perror("Error at server: recvfrom()!\n\n");				
				printf("Message received from Client: %s\nMessage Length(bytes): %d\n\n",buf,n);
				printf("Packet's IP: %s, Packet's Port_Number: %d\n\n",inet_ntoa(r_addport.sin_addr), ntohs(r_addport.sin_port));
				
				
				char port[10];
				int i =0;
				while(buf[i]!=':')
					i++;		
				char ip[i];
				strncpy(ip,&buf[0],i);	
				ip[i]='\0';							
				printf("ip: %s, strlen:%zu\n",ip,strlen(ip));
				strncpy(port,&buf[i+1],strlen(buf));
				port[strlen(port)]='\0';
				puts(port);
								
				// server_info
				port_no3 = atoi(port);	
				s3_addport.sin_family = AF_INET;
				s3_addport.sin_port = htons(port_no3);
				inet_aton(ip,&(s3_addport.sin_addr));
				printf("Server IP: %s, Server Port_Number: %d\n\n",inet_ntoa(s3_addport.sin_addr), ntohs(s3_addport.sin_port));
				
				// client_info				
				s4_addport.sin_family = AF_INET;
				s4_addport.sin_port = htons(port_no2);
				//memcpy(&s2, &s1, sizeof(sockaddr_storage));
				s4_addport.sin_addr.s_addr = r_addport.sin_addr.s_addr;				
				
				hostp = gethostbyaddr((const char *)&r_addport.sin_addr.s_addr, sizeof(r_addport.sin_addr.s_addr), AF_INET);
				if(hostp == NULL)
				{
					perror("Error at gethostbyaddr()!\n");
					exit(1);
				}
				
				// send port_number to client
				char msg[] = "20012";
				char conc_msg[40];
				sprintf(conc_msg,"$%s$%s",msg,inet_ntoa(s3_addport.sin_addr));	
				puts(buf);
				n = sendto(sock_id1, conc_msg, strlen(conc_msg), 0, (struct sockaddr *) &r_addport, sizeof(r_addport));
				if (n < 0) 
					perror("Error at Server: sendto()!\n");				
			}
			
			// handle socket 2 - tunnel as VPN
			if (FD_ISSET(sock_id2, &socks)) 
			{				
				socklen_t szaddr = sizeof(r_addport);
				n = recvfrom(sock_id2, buf, 10000, 0, (struct sockaddr *) &r_addport, &szaddr);
				if(n<0)
					perror("Error at server: recvfrom()!\n\n");
				printf("Message received from Client: %s\nMessage Length(bytes): %d\n",buf,n);
				
				hostp = gethostbyaddr((const char *)&r_addport.sin_addr.s_addr, sizeof(r_addport.sin_addr.s_addr), AF_INET);
				if(hostp == NULL)
				{
					perror("Error at gethostbyaddr()!\n");
					exit(1);
				}			
				
				printf("Server IP: %s, Server Port_Number: %d\n",inet_ntoa(s3_addport.sin_addr), ntohs(s3_addport.sin_port));
				printf("Client IP: %s, Client Port_Number: %d\n",inet_ntoa(s4_addport.sin_addr), ntohs(s4_addport.sin_port));				
																	
				// send messages through tunnel-server from client
				if(s3_addport.sin_addr.s_addr != r_addport.sin_addr.s_addr) 
				{		
					printf("Packet's destination IP: %s, Packet's destination Port_Number: %d\n",inet_ntoa(s3_addport.sin_addr), ntohs(s3_addport.sin_port));				
					printf("send messages through tunnel-server from client!\n\n");
					s4_addport.sin_port = r_addport.sin_port;
					szaddr = sizeof(s3_addport);
					int n2 = sendto(sock_id2, buf, n, 0, (struct sockaddr *) &s3_addport, szaddr);
					if (n2 < 0) 
					{
					  perror("Error at Client: sendto()!\n");
					  exit(1);
					}
				}
				
				// send messages through tunnel-client from server
				if(s3_addport.sin_addr.s_addr == r_addport.sin_addr.s_addr) 
				{
					printf("Packet's destination IP: %s, Packet's destination Port_Number: %d\n",inet_ntoa(s4_addport.sin_addr), ntohs(s4_addport.sin_port));				
					printf("send messages through tunnel-client from server!\n\n");
					socklen_t szaddr = sizeof(s4_addport);
					int n2 = sendto(sock_id2, buf, n, 0, (struct sockaddr *) &s4_addport, szaddr);
					if (n2 < 0) 
					{
					  perror("Error at Client: sendto()!\n");
					  exit(1);
					}
				}				
									
			}
			
		}		
			
	}

	// close server_socket
	close(sock_id1);	
	close(sock_id2);	
	
	return 0;
}
