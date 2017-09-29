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

int main(int argc, char **argv) 
{
    int sock_id, port_no;												// variable for socket_id and port-number
	int n,s_len,i;														// temp variables						
    
	// socket variable declaration
    struct sockaddr_in s_addport,c_addport;
    struct hostent *server;
    char *hostname_ip;
    	
	char lastChar = 'D';												// lastChar to fill up the payload string 	
	int sk_len;															// variable to store length of socket_address
	int c_len = sizeof(c_addport);										// variable to store length of receiver_address
	
	// variables used in gettimeofday() calculations
	struct timeval t1,t2;
    unsigned int elapsedTime;		
	
	// check the arguments
	if(argc!=5)
	{
		printf("Usage: mytunnel vpn-IP vpn-port server-IP server-port-number \n");
		exit(1);
	}
	
	// assign variables to respective values
	hostname_ip = argv[1];
    port_no = atoi(argv[2]);

	
	// create a buffer with payload-size of lastChar variable values
	char buf[50];
	sprintf(buf,"%s:%s",argv[3],argv[4]);	
	puts(buf);
	
	// creating a socket
    sock_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_id < 0) 
	{
		perror("Error opening socket at Client!\n");
		exit(1);
	}
        
    // resolve server name
    server = gethostbyname(hostname_ip);
    if (server == NULL) {
        printf("Unable to resolve host: %s\n", hostname_ip);
        exit(0);
    }
   
	// setting up a socket
    bzero((char *) &s_addport, sizeof(s_addport));
	bzero((char *) &c_addport, sizeof(c_addport));
    s_addport.sin_family = AF_INET;
    bcopy((char *)server->h_addr,(char *)&s_addport.sin_addr.s_addr, server->h_length);
    s_addport.sin_port = htons(port_no);     
    s_len = sizeof(s_addport);
			
	n = sendto(sock_id, buf, 50, 0, (struct sockaddr *) &s_addport, s_len);			
	if (n < 0) 
	{
	  perror("Error at Client: sendto()!\n");
	  exit(1);
	}
	
	bzero(buf,50);
	n = recvfrom(sock_id, buf, 50, 0, (struct sockaddr *) &s_addport, &s_len);
	if(n<0)
		perror("Error at server: recvfrom()!\n\n");				
	printf("Message received from Client: %s\nMessage Length(bytes): %d\n\n",buf,n);
	
	close(sock_id);
    return 0;
}
