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
	int status;    												// for storing the status information of the child process
	int len,n;	   												// buffer length		
	int option_val;      										// needed for setsockopt
	struct timeval t1,t2;										// gettimeofday() calculations
    unsigned int elapsedTime = 0;                               // to store diff of time values
	int totBytes = 0;                                           // variabe to store the totByte count
	unsigned int totPkts = 0;                                   // variable to store total packets count
	int payload_size;											// variable to store payload_size
	
	// check the arguments
	if(argc!=3)
	{
		printf("Usage: %s portnumber payload_size\n",argv[0]);
		exit(1);
	}
	
	// assign payload_size
	payload_size = atoi(argv[2]);
	
	// buffer array to receive input of size payload_size
	char buf[payload_size+1];  											

	// variables for socket	
	int sock_id,sock_id_new,port_no,cmp;
	struct sockaddr_in s_addport,c_addport;		
	struct hostent *hostp; 
	bzero((char *) &s_addport, sizeof(s_addport));	
	bzero((char *) &c_addport, sizeof(c_addport));
			
	// Setup UDP Socket
	sock_id = socket(AF_INET, SOCK_DGRAM, 0);					// UDP Datagram
	if(sock_id<0)
		perror("Error opening Socket\n");	
	
	// setsockopt: to rerun the server immediately after killing it
	option_val = 1;
	setsockopt(sock_id, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));
	
	// binding the socket
	port_no = atoi(argv[1]);
	s_addport.sin_family = AF_INET;
	s_addport.sin_port = htons(port_no);
	s_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id, (struct sockaddr *) &s_addport, sizeof(s_addport))== -1) 
	{
		printf("Bind failed!\n");
	}			
	socklen_t szaddr = sizeof(c_addport);		
	
	// break the loop when the number of bytes received equals '3'
	while(1)
	{
		// clean the buffer before receiving a new block
		bzero(buf,payload_size+1);		
		n = recvfrom(sock_id, buf, payload_size, 0, (struct sockaddr *) &c_addport, &szaddr);	
		
		// check error condition
		if(n<0)
			perror("Error at server: recvfrom()!\n\n");		
		
		// start timer just after receiving the first packet
		if(totPkts == 0)
			gettimeofday(&t1, NULL);
		
		// stop timer and break the loop when the size of packet received equals '3'
		if(n==3)
		{			
			gettimeofday(&t2, NULL);	
			break;	
		}
		
		// update totBytes and totPkts accordingly
		totBytes+=n;	
		totPkts+=1;				
	}
	
	elapsedTime = ((t2.tv_sec * 1000000 + t2.tv_usec) - (t1.tv_sec * 1000000 + t1.tv_usec));
	printf("Completion Time(seconds): %f\n",((double)elapsedTime/1000000));	
	
	// Add UDP/Ethernet overhead of 46 Bytes to the count of total bytes received
	totBytes +=46;
	
	printf("Throughput(Mbps):%f\n",(double)(totBytes*8)/elapsedTime);
	printf("Packets per second(pps): %f\n",(double)(totPkts*1000000/elapsedTime));
	
	// close server_socket
	close(sock_id);	
	
	return 0;
}
