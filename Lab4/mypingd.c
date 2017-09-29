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


int main(int argc, char *argv[])
{
	pid_t k;       												// signed integer for representing process_id
	char buf[1000];  											// buffer array to store input
	int status;    												// for storing the status information of the child process
	int len,n;	   												// buffer length		
	int option_val;
	
	// check the arguments
	if(argc!=3)
	{
		printf("Usage: %s portnumber secretkey \n",argv[0]);
		exit(1);
	}
	
	// variables for socket
	char secretkey[20];
	int sock_id,sock_id_new,port_no,cmp;
	struct sockaddr_in s_addport,c_addport;		
	struct hostent *hostp; 
	bzero((char *) &s_addport, sizeof(s_addport));	
	bzero((char *) &c_addport, sizeof(c_addport));
	
	sprintf(secretkey,"%s",argv[2]);
	
	// Setup UDP Socket
	sock_id = socket(AF_INET, SOCK_DGRAM, 0);					// UDP Datagram
	if(sock_id<0)
		perror("Error opening Socket\n");	
	
	// setsockopt: to rerun the server immediately after killing it
	option_val = 1;
	setsockopt(sock_id, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));
	
	port_no = atoi(argv[1]);
	s_addport.sin_family = AF_INET;
	s_addport.sin_port = htons(port_no);
	s_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id, (struct sockaddr *) &s_addport, sizeof(s_addport))== -1) 
	{
		printf("Bind failed!\n");
	}			
	
	// counter to ignore every 4th request
	int counter = 0;
	while(1)
	{			
		bzero(buf,1000);
		socklen_t szaddr = sizeof(c_addport);
		n = recvfrom(sock_id, buf, 1000, 0, (struct sockaddr *) &c_addport, &szaddr);
		if(n<0)
			perror("Error at server: recvfrom()!\n\n");
		printf("Message received from Client: %s\nMessage Length(bytes): %d\n\n",buf,n);

		// check if it's the 4th request
		counter++;
		if(counter%4==0)
		{
			printf("Request Ignored!\n");
			continue;
		}
		
		hostp = gethostbyaddr((const char *)&c_addport.sin_addr.s_addr, sizeof(c_addport.sin_addr.s_addr), AF_INET);
		if(hostp == NULL)
		{
			perror("Error at gethostbyaddr()!\n");
			exit(1);
		}
		
		// 'skey' for secret_key 
		char skey[50];		
		int i = 1;
		bzero(skey,50);		
		while(buf[i]!='$')
			i++;		
		strncpy(skey,&buf[1],i-1);	
		skey[strlen(skey)]='\0';		
		puts(skey);				
		
		// check secret_key 	
		if(strcmp(secretkey,skey)!=0)
		{
			printf("Secret Keys don't match!\n\n");
			continue;
		}		
		
		// check length of buffer
		if(strlen(buf)!=1000)
		{
			printf("Request length from message is not 1000 bytes!\n\n");
			continue;
		}		
		
		printf("Recepient's IP: %s, Recepient's Port_Number: %d\n\n",inet_ntoa(c_addport.sin_addr), ntohs(c_addport.sin_port));				
		// send message terve back to client
		char msg[] = "terve";
		n = sendto(sock_id, msg, strlen(msg), 0, (struct sockaddr *) &c_addport, sizeof(c_addport));
		if (n < 0) 
			perror("Error at Server: sendto()!\n");
			
	}
	
		
	// close server_socket
	close(sock_id);	
	
	return 0;
}
