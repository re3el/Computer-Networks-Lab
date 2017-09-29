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
//#define max(a,b) (((a) > (b)) ? (a) : (b))

int flag_1 = 0, flag_2 = 0, flag_3 = 0, flag_4 = 0;

int max(int a, int b, int c, int d, int e)
{
	int res;
	int tmp1 = (a > b?a:b);
	int tmp2 = (c > d?c:d);	
	int tmp3 = tmp1>tmp2?tmp1:tmp2;
	res = tmp3>e?tmp3:e;
	return res;
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
	
	// assign payload_size
	//payload_size = atoi(argv[2]);
	
	// buffer array to receive input of size payload_size
	char buf[BUF_SIZE];  											

	// variables for socket	
	int sock_id1,sock_id2,sock_id3,sock_id4,sock_id5,port_no1,port_no2,port_no3,cmp;
	struct sockaddr_in t1_addport,t2_addport,t3_addport,t4_addport,t5_addport,s1_addport,s2_addport,s3_addport,s4_addport,c1_addport,c2_addport,c3_addport,c4_addport,r_addport;		
	struct hostent *hostp; 
	bzero((char *) &t1_addport, sizeof(t1_addport));	
	bzero((char *) &t2_addport, sizeof(t2_addport));
	bzero((char *) &t3_addport, sizeof(t3_addport));	
	bzero((char *) &t4_addport, sizeof(t4_addport));
	bzero((char *) &t5_addport, sizeof(t5_addport));
	bzero((char *) &s1_addport, sizeof(s1_addport));
	bzero((char *) &r_addport, sizeof(r_addport));
			
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
	
	sock_id5 = socket(AF_INET, SOCK_DGRAM, 0);					// UDP Datagram
	if(sock_id5<0)
		perror("Error opening Socket\n");
	
	// setsockopt: to rerun the server immediately after killing it
	option_val = 1;
	setsockopt(sock_id1, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));
	setsockopt(sock_id2, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));
	setsockopt(sock_id3, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));
	setsockopt(sock_id4, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));
	setsockopt(sock_id5, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));
	
	// binding the socket
	port_no1 = atoi(argv[1]);	
	t1_addport.sin_family = AF_INET;
	t1_addport.sin_port = htons(port_no1);
	t1_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id1, (struct sockaddr *) &t1_addport, sizeof(t1_addport))== -1) 
	{
		printf("Bind failed!\n");
	}
	
	port_no2 = 20012; 	
	t2_addport.sin_family = AF_INET;
	t2_addport.sin_port = htons(port_no2);
	t2_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id2, (struct sockaddr *) &t2_addport, sizeof(t2_addport))== -1) 
	{
		printf("Bind failed!\n");
	}
	
	port_no2 = 20013; 	
	t3_addport.sin_family = AF_INET;
	t3_addport.sin_port = htons(port_no2);
	t3_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id3, (struct sockaddr *) &t3_addport, sizeof(t3_addport))== -1) 
	{
		printf("Bind failed!\n");
	}
	
	port_no2 = 20014; 	
	t4_addport.sin_family = AF_INET;
	t4_addport.sin_port = htons(port_no2);
	t4_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id4, (struct sockaddr *) &t4_addport, sizeof(t4_addport))== -1) 
	{
		printf("Bind failed!\n");
	}
	
	port_no2 = 20015; 	
	t5_addport.sin_family = AF_INET;
	t5_addport.sin_port = htons(port_no2);
	t5_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id5, (struct sockaddr *) &t5_addport, sizeof(t5_addport))== -1) 
	{
		printf("Bind failed!\n");
	}	
	
		
	while(1)
	{		
		fd_set socks;
		FD_ZERO(&socks);
		FD_SET(sock_id1, &socks);
		FD_SET(sock_id2, &socks);
		FD_SET(sock_id3, &socks);
		FD_SET(sock_id4, &socks);
		FD_SET(sock_id5, &socks);
		
		int nsocks = max(sock_id1,sock_id2,sock_id3,sock_id4,sock_id5) + 1;
		
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
				printf("Message received from Client: %s\n",buf);
				printf("Packet's IP: %s\n",inet_ntoa(r_addport.sin_addr));
				
				
				char port[10];
				int i =0;
				while(buf[i]!=':')
					i++;		
				char ip[i];
				strncpy(ip,&buf[0],i);	
				ip[i]='\0';							
				//printf("ip: %s, strlen:%zu\n",ip,strlen(ip));
				strncpy(port,&buf[i+1],strlen(buf));
				port[strlen(port)]='\0';
				//puts(port);
								
				// server_info
				if(flag_1 == 0)
				{
					//printf("inside client 1\n");
					port_no3 = atoi(port);	
					s1_addport.sin_family = AF_INET;
					s1_addport.sin_port = htons(port_no3);
					inet_aton(ip,&(s1_addport.sin_addr));
					printf("Server IP: %s, Server Port_Number: %d\n\n",inet_ntoa(s1_addport.sin_addr), ntohs(s1_addport.sin_port));
					
					// client_info
					port_no2 = 20012;
					c1_addport.sin_family = AF_INET;
					c1_addport.sin_port = htons(port_no2);
					//memcpy(&s2, &s1, sizeof(sockaddr_storage));
					c1_addport.sin_addr.s_addr = r_addport.sin_addr.s_addr;	

					hostp = gethostbyaddr((const char *)&r_addport.sin_addr.s_addr, sizeof(r_addport.sin_addr.s_addr), AF_INET);
					if(hostp == NULL)
					{
						perror("Error at gethostbyaddr()!\n");
						exit(1);
					}
					
					// send port_number to client
					char msg[] = "20012";
					char conc_msg[40];
					sprintf(conc_msg,"$%s$%s",msg,inet_ntoa(s1_addport.sin_addr));	
					//puts(buf);
					n = sendto(sock_id1, conc_msg, strlen(conc_msg), 0, (struct sockaddr *) &r_addport, sizeof(r_addport));
					if (n < 0) 
						perror("Error at Server: sendto()!\n");	
					
					flag_1++;
					
				}
				
				else if(flag_2 == 0)
				{
					//printf("inside client 2\n");
					port_no3 = atoi(port);	
					s2_addport.sin_family = AF_INET;
					s2_addport.sin_port = htons(port_no3);
					inet_aton(ip,&(s2_addport.sin_addr));
					printf("Server IP: %s, Server Port_Number: %d\n\n",inet_ntoa(s2_addport.sin_addr), ntohs(s2_addport.sin_port));
					
					// client_info		
					port_no2 = 20013;					
					c2_addport.sin_family = AF_INET;
					c2_addport.sin_port = htons(port_no2);
					//memcpy(&s2, &s1, sizeof(sockaddr_storage));
					c2_addport.sin_addr.s_addr = r_addport.sin_addr.s_addr;	

					hostp = gethostbyaddr((const char *)&r_addport.sin_addr.s_addr, sizeof(r_addport.sin_addr.s_addr), AF_INET);
					if(hostp == NULL)
					{
						perror("Error at gethostbyaddr()!\n");
						exit(1);
					}
					
					// send port_number to client
					char msg[] = "20013";
					char conc_msg[40];
					sprintf(conc_msg,"$%s$%s",msg,inet_ntoa(s2_addport.sin_addr));	
					//puts(buf);
					n = sendto(sock_id1, conc_msg, strlen(conc_msg), 0, (struct sockaddr *) &r_addport, sizeof(r_addport));
					if (n < 0) 
						perror("Error at Server: sendto()!\n");	
					
					flag_2++;
					
				}
				
				else if(flag_3 == 0)
				{
					//printf("inside client 1\n");
					port_no3 = atoi(port);	
					s3_addport.sin_family = AF_INET;
					s3_addport.sin_port = htons(port_no3);
					inet_aton(ip,&(s3_addport.sin_addr));
					printf("Server IP: %s, Server Port_Number: %d\n\n",inet_ntoa(s3_addport.sin_addr), ntohs(s3_addport.sin_port));
					
					// client_info	
					port_no2 = 20014;					
					c3_addport.sin_family = AF_INET;
					c3_addport.sin_port = htons(port_no2);
					//memcpy(&s2, &s1, sizeof(sockaddr_storage));
					c3_addport.sin_addr.s_addr = r_addport.sin_addr.s_addr;	

					hostp = gethostbyaddr((const char *)&r_addport.sin_addr.s_addr, sizeof(r_addport.sin_addr.s_addr), AF_INET);
					if(hostp == NULL)
					{
						perror("Error at gethostbyaddr()!\n");
						exit(1);
					}
					
					// send port_number to client
					char msg[] = "20014";
					char conc_msg[40];
					sprintf(conc_msg,"$%s$%s",msg,inet_ntoa(s3_addport.sin_addr));	
					//puts(buf);
					n = sendto(sock_id1, conc_msg, strlen(conc_msg), 0, (struct sockaddr *) &r_addport, sizeof(r_addport));
					if (n < 0) 
						perror("Error at Server: sendto()!\n");	
					
					flag_3++;
					
				}
				
				else if(flag_4 == 0)
				{
					//printf("inside client 1\n");
					port_no3 = atoi(port);	
					s4_addport.sin_family = AF_INET;
					s4_addport.sin_port = htons(port_no3);
					inet_aton(ip,&(s4_addport.sin_addr));
					printf("Server IP: %s, Server Port_Number: %d\n\n",inet_ntoa(s4_addport.sin_addr), ntohs(s4_addport.sin_port));
					
					// client_info				
					c4_addport.sin_family = AF_INET;
					c4_addport.sin_port = htons(port_no2);
					//memcpy(&s2, &s1, sizeof(sockaddr_storage));
					c4_addport.sin_addr.s_addr = r_addport.sin_addr.s_addr;	

					hostp = gethostbyaddr((const char *)&r_addport.sin_addr.s_addr, sizeof(r_addport.sin_addr.s_addr), AF_INET);
					if(hostp == NULL)
					{
						perror("Error at gethostbyaddr()!\n");
						exit(1);
					}
					
					// send port_number to client
					char msg[] = "20015";
					char conc_msg[40];
					sprintf(conc_msg,"$%s$%s",msg,inet_ntoa(s4_addport.sin_addr));	
					//puts(buf);
					n = sendto(sock_id1, conc_msg, strlen(conc_msg), 0, (struct sockaddr *) &r_addport, sizeof(r_addport));
					if (n < 0) 
						perror("Error at Server: sendto()!\n");	
					
					flag_4++;
					
				}
				else
				{
					printf("Handles upto 4 Clients only!\n");
					continue;
				}
				
							
			}
			
			// handle socket 2 - tunnel as VPN
			if (FD_ISSET(sock_id2, &socks)) 
			{	
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
				
				flag_1 = 0;
				//printf("connection closed! flag_1:%d \n",flag_1);
									
			}
			
			//
			if (FD_ISSET(sock_id3, &socks)) 
			{				
				socklen_t szaddr = sizeof(r_addport);
				bzero(buf,1000);
				n = recvfrom(sock_id3, buf, 10000, 0, (struct sockaddr *) &r_addport, &szaddr);
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
				if(s2_addport.sin_addr.s_addr != r_addport.sin_addr.s_addr) 
				{		
					//printf("Packet's destination IP: %s, Packet's destination Port_Number: %d\n",inet_ntoa(s2_addport.sin_addr), ntohs(s2_addport.sin_port));				
					//printf("send messages through tunnel-server from client!\n");
					c2_addport.sin_port = r_addport.sin_port;
					szaddr = sizeof(s2_addport);
					int n2 = sendto(sock_id3, buf, n, 0, (struct sockaddr *) &s2_addport, szaddr);
					if (n2 < 0) 
					{
					  perror("Error at Client: sendto()!\n");
					  exit(1);
					}
				}
				
				// send messages through tunnel-client from server
				if(s2_addport.sin_addr.s_addr == r_addport.sin_addr.s_addr) 
				{
					//printf("Packet's destination IP: %s, Packet's destination Port_Number: %d\n",inet_ntoa(c2_addport.sin_addr), ntohs(c2_addport.sin_port));				
					//printf("send messages through tunnel-client from server!\n");
					socklen_t szaddr = sizeof(c2_addport);
					int n2 = sendto(sock_id3, buf, n, 0, (struct sockaddr *) &c2_addport, szaddr);
					if (n2 < 0) 
					{
					  perror("Error at Client: sendto()!\n");
					  exit(1);
					}
				}		
				
				flag_2 = 0;
				//printf("connection closed! flag_2:%d \n",flag_2);
									
			}
			
			if (FD_ISSET(sock_id4, &socks)) 
			{		
				bzero(buf,1000);
				socklen_t szaddr = sizeof(r_addport);
				n = recvfrom(sock_id4, buf, 10000, 0, (struct sockaddr *) &r_addport, &szaddr);
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
				if(s3_addport.sin_addr.s_addr != r_addport.sin_addr.s_addr) 
				{		
					//printf("Packet's destination IP: %s, Packet's destination Port_Number: %d\n",inet_ntoa(s3_addport.sin_addr), ntohs(s3_addport.sin_port));				
					//printf("send messages through tunnel-server from client!\n");
					c3_addport.sin_port = r_addport.sin_port;
					szaddr = sizeof(s3_addport);
					int n2 = sendto(sock_id4, buf, n, 0, (struct sockaddr *) &s3_addport, szaddr);
					if (n2 < 0) 
					{
					  perror("Error at Client: sendto()!\n");
					  exit(1);
					}
				}
				
				// send messages through tunnel-client from server
				if(s3_addport.sin_addr.s_addr == r_addport.sin_addr.s_addr) 
				{
					//printf("Packet's destination IP: %s, Packet's destination Port_Number: %d\n",inet_ntoa(c3_addport.sin_addr), ntohs(c3_addport.sin_port));				
					//printf("send messages through tunnel-client from server!\n");
					socklen_t szaddr = sizeof(c3_addport);
					int n2 = sendto(sock_id4, buf, n, 0, (struct sockaddr *) &c3_addport, szaddr);
					if (n2 < 0) 
					{
					  perror("Error at Client: sendto()!\n");
					  exit(1);
					}
				}	
				
				flag_3 = 0;
				//printf("connection closed! flag_3:%d \n",flag_3);					
			}
			
			if (FD_ISSET(sock_id5, &socks)) 
			{
				bzero(buf,1000);
				socklen_t szaddr = sizeof(r_addport);
				n = recvfrom(sock_id5, buf, 10000, 0, (struct sockaddr *) &r_addport, &szaddr);
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
				if(s4_addport.sin_addr.s_addr != r_addport.sin_addr.s_addr) 
				{		
					//printf("Packet's destination IP: %s, Packet's destination Port_Number: %d\n",inet_ntoa(s4_addport.sin_addr), ntohs(s4_addport.sin_port));				
					//printf("send messages through tunnel-server from client!\n");
					c4_addport.sin_port = r_addport.sin_port;
					szaddr = sizeof(s4_addport);
					int n2 = sendto(sock_id5, buf, n, 0, (struct sockaddr *) &s4_addport, szaddr);
					if (n2 < 0) 
					{
					  perror("Error at Client: sendto()!\n");
					  exit(1);
					}
				}
				
				// send messages through tunnel-client from server
				if(s4_addport.sin_addr.s_addr == r_addport.sin_addr.s_addr) 
				{
					//printf("Packet's destination IP: %s, Packet's destination Port_Number: %d\n",inet_ntoa(c4_addport.sin_addr), ntohs(c4_addport.sin_port));				
					//printf("send messages through tunnel-client from server!\n");
					socklen_t szaddr = sizeof(c4_addport);
					int n2 = sendto(sock_id5, buf, n, 0, (struct sockaddr *) &c4_addport, szaddr);
					if (n2 < 0) 
					{
					  perror("Error at Client: sendto()!\n");
					  exit(1);
					}
				}
				
				flag_4 = 0;
				//printf("connection closed! flag_4:%d \n",flag_4);
									
			}
			
		}		
			
	}

	// close server_socket
	close(sock_id1);	
	close(sock_id2);	
	close(sock_id3);	
	close(sock_id4);	
	close(sock_id5);	
	
	return 0;
}
