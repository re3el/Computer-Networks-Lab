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
		
	unsigned int packet_spacing;										// variable to store packet-spacing
	int payload_size;													// variable to store payload-size
	int packet_count;													// variable to store packet-count
	int totBytes = 0;													// variable to store the count of total bytes sent
	
	// check the arguments
	if(argc!=7)
	{
		printf("Usage: traffic_snd IP-address port-number payload-size packet-count packet-spacing src_port\n");
		exit(1);
	}
	
	// assign variables to respective values
	
	hostname_ip = argv[1];
    port_no = atoi(argv[2]);
	payload_size = atoi(argv[3]);
	packet_count = atoi(argv[4]);
	packet_spacing = strtoul(argv[5],NULL,0);	
	int src_prt = atoi(argv[6]);
	// create a buffer with payload-size of lastChar variable values
	char buf[payload_size+1];
	for (i = 0; i < payload_size; ++i) 
	{
		buf[i] = lastChar;
		
    }
    buf[payload_size] = '\0';			
	
	// creating a socket
    sock_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_id < 0) 
	{
		perror("Error opening socket at Client!\n");
		exit(1);
	}
	bzero((char *) &c_addport, sizeof(c_addport));
	c_addport.sin_family = AF_INET;
	c_addport.sin_port = htons(src_prt);
	c_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id, (struct sockaddr *) &c_addport, sizeof(c_addport))== -1) 
	{
		printf("Bind failed!\n");
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
	
	// variable to store the count of total packets sent
	unsigned int totPkts = 0;										
	
	// start the timer before sending the first packet and iterate over the loop packet-count value times
	gettimeofday(&t1, NULL);
	for(i=0;i<packet_count;i++)
	{				
		n = sendto(sock_id, buf, payload_size, 0, (struct sockaddr *) &s_addport, s_len);
		
		//check for error
		if (n < 0) 
		{
		  perror("Error at Client: sendto()!\n");
		  exit(1);
		}
		
		// respectively update the variables
		totPkts += 1;
		totBytes +=n;	

		// put the program to sleep for packet-spacing timeperiod
		usleep(packet_spacing);
	}
	
	// called after the transmission of the last packet 
	gettimeofday(&t2, NULL); 

	// send '3' packets with size length '3' to indicate the receiver that file transmission is complete
	for(i=0;i<3;i++)
	{
		n = sendto(sock_id, "000", 3, 0, (struct sockaddr *) &s_addport, s_len);
	}		
	
	elapsedTime = ((t2.tv_sec * 1000000 + t2.tv_usec) - (t1.tv_sec * 1000000 + t1.tv_usec));
	printf("Completion Time(seconds): %f\n",((double)elapsedTime/1000000));	
	
	// add UDP/Ethernet overhead to the count of the total bytes
	totBytes+=46;
	
	printf("totPkts sent: %d\n",totPkts);
	printf("totBytes sent: %d\n",totBytes);
	printf("Throughput(Mbps):%f\n",(double)(totBytes*8)/elapsedTime);
	printf("Packets per second(pps): %f\n",(double)(totPkts*1000000/elapsedTime));
	
	close(sock_id);
    return 0;
}
