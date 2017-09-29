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
		
	
	// Get blocksize from configfile.dat for read() and write()
	int fpc,blocksize;
	char configFile[50];	
	sprintf(configFile,"%s",argv[1]);
	fpc = open(configFile,O_RDWR);
	if(fpc < 0) 
	{
		printf("Error opening configfile.dat at Server\n");
		exit(-1);
	}
	
	
}