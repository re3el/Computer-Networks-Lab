#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/time.h>
#include <signal.h>
#include <arpa/inet.h>

// 		gcc overlaybuild.c -o overlaybuild;	./overlaybuild 128.10.25.101 10024 128.10.25.102 128.10.25.103 128.10.25.104 128.10.25.105

int main(int argc, char **argv) 
{
    int sock_id, dst_port;												// variable for socket_id and port-number
	int n,s_len,i;														// temp variables						
    
	// socket variable declaration
    struct sockaddr_in s_addport,c_addport;
    struct hostent *server;
    char *dst_ip;
    	
	int sk_len;															// variable to store length of socket_address
	int c_len = sizeof(c_addport);										// variable to store length of receiver_address
	
	// variables used in gettimeofday() calculations
	struct timeval t1,t2;
    unsigned int elapsedTime;		
	
	// check the arguments
	if(argc < 3)
	{
		printf("Usage: overlaybuild dst-IP dst-port routerk-IP ... router2-IP router1-IP  \n");
		exit(1);
	}

	int nRouters = (argc - 3);
	printf("nRouters: %d\n",nRouters);
	

	char *rt_ip[nRouters];
	int rt_port[nRouters];
	// assign variables to respective values
	dst_ip = argv[1];
    dst_port = atoi(argv[2]);
        
    for(i=0;i<nRouters;i++)
    {
    	rt_ip[i] = argv[i+3]; 
    }

    char buf[10000];
	sprintf(buf,"$%s$%s$",argv[1],argv[2]);	
	buf[strlen(buf)] = '\0';
	//puts(buf);

	char temp[1000];
    for(i=0;i<nRouters;i++)
    {
    	bzero(temp,1000);
    	sprintf(temp,"%s$",rt_ip[i]);
    	//printf("strlen: %zu\n",strlen(temp));
    	strncpy(buf+strlen(buf),temp,strlen(temp));	
    }
    buf[strlen(buf)]='\0';
    printf("buf: %s\n",buf);
	
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
	printf("Sent to Router IP: %s\n",ip);
	
	int router_port = 10024;
	//printf("dst_ip: %s\n",dst_ip);
    //exit(1);
	
	// creating a socket
    sock_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_id < 0) 
	{
		perror("Error opening socket at Client!\n");
		exit(1);
	}          
   
	// setting up a socket
    bzero((char *) &s_addport, sizeof(s_addport));
	bzero((char *) &c_addport, sizeof(c_addport));
    s_addport.sin_family = AF_INET;
    //bcopy((char *)server->h_addr,(char *)&s_addport.sin_addr.s_addr, server->h_length);
    s_addport.sin_port = htons(router_port);     
    inet_aton(ip,&(s_addport.sin_addr));
    s_len = sizeof(s_addport);

    //printf("Server IP: %s, Server Port_Number: %d\n\n",inet_ntoa(s_addport.sin_addr), ntohs(s_addport.sin_port));
			
	n = sendto(sock_id, buf, strlen(buf), 0, (struct sockaddr *) &s_addport, s_len);			
	if (n < 0) 
	{
	  perror("Error at Client: sendto()!\n");
	  exit(1);
	}
	
	char recv[100];
	bzero(recv,100);
	n = recvfrom(sock_id, recv, 10, 0, (struct sockaddr *) &s_addport, &s_len);
	if(n<0)
		perror("Error at client: recvfrom()!\n\n");				
	printf("Message received from Client: %s\nMessage Length(bytes): %d\n\n",recv,n);
	
	close(sock_id);
    return 0;
}


/* 	char *pch,*pch1,*pch2,*pch3;
	char ran[50];
	char ip[50];
	pch1 = strtok (msgCopy,"$");		
	pch2 = strtok (NULL,"$");	
	pch3 = strtok (NULL,"$");	
	sprintf(ran,"$%s$%s$",pch1,pch2);
	printf("string: %s, len: %zu",ran,strlen(ran));
	exit(1); */