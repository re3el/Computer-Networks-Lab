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


void handler(int signal) 
{    
	// WNOHANG: return 0 if the child process isn't terminated and child process pid if terminated. It's a non blocking call from the parent to reap zombies
	// while loop used to reap more than 1 zombie that may exist
	while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}  
}

int main(int argc, char *argv[])
{
	pid_t k;       												// signed integer for representing process_id
	char buf[1000];  											// buffer array to store input
	int status;    												// for storing the status information of the child process
	int len,n;	   												// buffer length		
	
	// check the arguments
	if(argc<3)
	{
		printf("Usage: %s portnumber secretkey \n",argv[0]);
		exit(1);
	}
	
	// setting up the socket
	char secretkey[20];
	int sock_id,sock_id_new,port_no,cmp;
	struct sockaddr_in s_addport,c_addport;		
	bzero((char *) &s_addport, sizeof(s_addport));	
	bzero((char *) &c_addport, sizeof(c_addport));
	
	sprintf(secretkey,"%s",argv[2]);
	sock_id = socket(AF_INET, SOCK_STREAM, 0);					// TCP stream
	if(sock_id<0)
		perror("Error opening Socket\n");	
	port_no = atoi(argv[1]);
	s_addport.sin_family = AF_INET;
	s_addport.sin_port = htons(port_no);
	s_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id, (struct sockaddr *) &s_addport, sizeof(s_addport))== -1) 
	{
		printf("Bind failed!\n");
	}			
	listen(sock_id, 5);

	while(1)
	{
		int saved_stdout;											
		saved_stdout = dup(1);									// save the current_state for bringing execution back to stdout after dup2()
		
		// full association 
		socklen_t szaddr = sizeof(c_addport);
		sock_id_new = accept(sock_id, (struct sockaddr *) &c_addport, &szaddr);		
		if(sock_id_new < 0)
		{
			perror("Accept failed!\n");
			exit(-1);
		}
		
		// read client_info - $secretkey$comm
		bzero(buf,1000);
		n = read(sock_id_new,buf,sizeof(buf));
		if (n < 0) 
			perror("Error at socket read!\n");
		printf("Message received from Client: %s\n",buf);
		
		// store client_info in two separate buffers: 'skey' for secret_key and 'comm' for command requested
		char skey[50];
		char comm[50];
		int i = 1;
		bzero(skey,50);
		bzero(comm,50);
		while(buf[i]!='$')
			i++;		
		strncpy(skey,&buf[1],i-1);	
		skey[strlen(skey)]='\0';		
		//puts(skey);
		strncpy(comm,&buf[i+1],strlen(buf)-i);	
		comm[strlen(comm)]='\0';
		//puts(comm);				

		// SIGCHLD for creating an asynchronous non blocking call
		struct sigaction sa;
		sa.sa_handler = &handler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
		if (sigaction(SIGCHLD, &sa, 0) == -1) {
			perror(0);			
			exit(1);
		} 
				
		dup2(sock_id_new,1);		
		close(sock_id_new);		
				
		// forking
		k = fork();			
		
		// child process 
		if (k==0) {											
			// child code
			
			// check if the commands match
			int comm_chk = 0;
			if(strcmp(comm,"ls") == 0 || strcmp(comm,"host") == 0 || strcmp(comm,"date") == 0 || strcmp(comm,"cal") == 0)
				comm_chk++;			
			if(comm_chk == 0)
			{
				printf("Sorry! Check with one of these commands - ls,host,date,cal \n");
				exit(1);
			}
			
			// check secret_key 	
			if(strcmp(secretkey,skey)!=0)
			{
				printf("Secret Keys don't match!\n");
				exit(1);
			}					
			
			// if execution failed, terminate child
			if(execlp(comm,comm,NULL) == -1)       			
            {    
				printf("Child Execution Failed\n");
				exit(1); 
			} 			
        }
		
		// fork fails
		else if(k==-1)
		{
			printf("Cannot fork!\n");
			exit(1);
		}
		
		// parent process
		else
		{        	
			memset(buf, 0, sizeof(buf));					// clearing buf memory for serving concurrent requests
			
			// restore stdout from socket back to stdout
			dup2(saved_stdout, 1);
			close(saved_stdout);
		}
	}
		
	// close server_socket
	close(sock_id);	
	
	return 0;
}


// Interleaving is not an issue anymore in this case
