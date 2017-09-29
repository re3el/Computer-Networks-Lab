#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <poll.h>
#include <errno.h>

#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h>

int j = 0;
volatile sig_atomic_t print_flag = false;
volatile int STOP = 0; 
int wait_flag = 1;                    /* TRUE while no signal received */
static int read_flag = 0;
char global_buf[51];
char input_buffer[1024];
char output_buffer[52];
int flag_setup = 0;

// variables for socket	
int udp_sockd,udp_port,option_val;
struct sockaddr_in s_addport,p_addport,c_addport;	
struct hostent *hostp; 
struct termios oldtermios;

void handler(int signal) 
{    
	// WNOHANG: return 0 if the child process isn't terminated and child process pid if terminated. It's a non blocking call from the parent to reap zombies
	// while loop used to reap more than 1 zombie that may exist
	while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}  
}

/* int ttyraw(int fd)
{
	struct termios newtermios;
	if(tcgetattr(fd, &oldtermios) < 0)
		return(-1);
	newtermios = oldtermios;

	newtermios.c_lflag &= ~(ICANON);
	//newtermios.c_cc[VERASE] = 127;
	
	if(tcsetattr(fd, TCSAFLUSH, &newtermios) < 0)
		return(-1);
	return(0);
}

int ttyreset(int fd)
{
	if(tcsetattr(fd, TCSAFLUSH, &oldtermios) < 0)
		return(-1);

	return(0);
}

void handle_alarm( int sig ) {    
	printf("Timeout!\n");	
	printf("?\n");		
}

void handle_sigpoll (int status)
{	
	int c_len = sizeof(c_addport);
    int bytes = recvfrom(udp_sockd, input_buffer, sizeof(input_buffer), 0, (struct sockaddr *) &c_addport, &c_len);
	//printf("string received: %s, %zu\n",input_buffer,strlen(input_buffer));	

	if (strcmp(input_buffer,"KO")==0) 
	{
		//printf("before zero\n");
		alarm(0);
		printf("| doesn't want to chat\n");
		printf("?\n");	
	}
	
	if (strcmp(input_buffer,"OK")==0) 
	{	
		printf("Connection Established! \n");
		alarm(0);
		flag_setup = 1;
		printf(">");
		fflush(stdout);			
	}
	
	if (strcmp(input_buffer,"E")==0) 
	{
		printf("\n| chat terminated\n");
		printf("?\n");		
		flag_setup = 0;
		if(flag_setup == 0)
		{
			tcgetattr(0, &oldtermios);			
		}
	}
	
	if(input_buffer[0]=='D')
	{		
		printf("\n| %.*s\n", (int)strlen(input_buffer)-1, input_buffer + 1);	
		printf(">");						
		fflush(stdout);
	}
	
	fflush(stdin);	
	fflush(stdout);	
	fflush(stdout);	

	if(sizeof(global_buf)>0)
	{
		//printf("\n");
		write(1,global_buf,sizeof(global_buf));	
	}
}

void chopnl(char *s) {              //strip '\n'
    s[strcspn(s,"\n")] = '\0';
}*/

int main(int argc, char *argv[])
{	    												
	// check the arguments
	if(argc!=7)
	{
		printf("Usage: audiostreamd tcp-port udp-port payload-size packet-spacing mode logfile-s \n");
		exit(1);
	}
	
	int n;
	char buf[50];
	udp_port = atoi(argv[2]);
	
/*  	struct sigaction sa;	
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGPOLL);
	sigaddset(&sa.sa_mask, SIGALRM);	
	sa.sa_handler = &handle_alarm;
	sa.sa_flags = 0;	 
	
 	if (sigaction(SIGALRM, &sa, 0) == -1) {
		perror(0);			
		exit(1);
	} 
	
	sa.sa_handler = &handle_sigpoll;	
	if (sigaction(SIGPOLL, &sa, 0) == -1) {
		perror(0);			
		exit(1);
	}	 */
		
	
	// TCP stream
	int tcp_sockd,tcp_sockd_new,tcp_port,cmp;
	struct sockaddr_in s_addport,c_addport;		
	bzero((char *) &s_addport, sizeof(s_addport));	
	bzero((char *) &c_addport, sizeof(c_addport));
		
	tcp_sockd = socket(AF_INET, SOCK_STREAM, 0);					
	if(tcp_sockd<0)
		perror("Error opening TCP Socket\n");	
	tcp_port = atoi(argv[1]);
	s_addport.sin_family = AF_INET;
	s_addport.sin_port = htons(tcp_port);
	s_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(tcp_sockd, (struct sockaddr *) &s_addport, sizeof(s_addport))== -1) 
	{
		printf("Bind failed!\n");
	}			
	listen(tcp_sockd, 5);

	while(1)
	{
		/* int saved_stdout;											
		saved_stdout = dup(1);									// save the current_state for bringing execution back to stdout after dup2() */
		
		// full association 
		socklen_t szaddr = sizeof(c_addport);
		tcp_sockd_new = accept(tcp_sockd, (struct sockaddr *) &c_addport, &szaddr);		
		if(tcp_sockd_new < 0)
		{
			perror("Accept failed!\n");
			exit(-1);
		}
		
		// read client_info - 'portnumber audiofile_directory'
		bzero(buf,1000);
		n = read(tcp_sockd_new,buf,sizeof(buf));
		if (n < 0) 
			perror("Error at socket read!\n");
		printf("Message received from Client: %s\n",buf);
		
		// store client_info in two separate buffers: 'client_udp_port' for port_number and 'path' for audio file path directory
		char client_udp_port[50];
		char path[50];		
		int i = 0;
		bzero(client_udp_port,50);
		bzero(path,50);
		while(buf[i]!=' ')
			i++;		
		strncpy(client_udp_port,&buf[0],i-1);	
		client_udp_port[strlen(client_udp_port)]='\0';		
		printf("client_udp_port: %s\n",client_udp_port);
		strncpy(path,&buf[i+1],strlen(buf)-i);	
		path[strlen(path)]='\0';
		printf("path: %s\n",path);	
		
		int fp = open(path,O_RDONLY);			
		//if( access( path, F_OK ) == -1 )
		if(fp < 0) 
		{
			char msg[] = "KO";			
			write(tcp_sockd_new,msg,2);
			close(tcp_sockd_new);
			printf("Error opening file directory:%s at Server\n",path);
			continue;
		}
		
		else
		{
			char suc_msg[10];			
			sprintf(suc_msg,"OK %d",udp_port);	
			suc_msg[strlen(suc_msg)]='\0';
			puts(suc_msg);
			write(tcp_sockd_new,suc_msg,sizeof(suc_msg));
			printf("File found. Forking underway!\n");
			bzero(suc_msg,10);
			udp_port+=1;
		}

 		// SIGCHLD for creating an asynchronous non blocking call
		struct sigaction sa;
		sa.sa_handler = &handler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
		if (sigaction(SIGCHLD, &sa, 0) == -1) {
			perror(0);			
			exit(1);
		}  
				
		// forking
		int k = fork();			
		
		// child process 
		if (k==0) {											
			// child code
			close(tcp_sockd_new);
			printf("inside child\n");
			
			udp_sockd = socket(AF_INET, SOCK_DGRAM, 0);					
			if(udp_sockd<0)
				perror("Error opening Socket\n");	
					
			fcntl(udp_sockd,F_SETOWN,getpid());           /* allow the process to receive SIGIO */
			fcntl(udp_sockd, F_SETFL, FASYNC); 	
			
			/* setsockopt: to rerun the server immediately after killing it */
			option_val = 1;
			setsockopt(udp_sockd, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));
			
			udp_port = atoi(argv[1]);
			s_addport.sin_family = AF_INET;
			s_addport.sin_port = htons(udp_port);
			s_addport.sin_addr.s_addr = htonl(INADDR_ANY);
			if(bind(udp_sockd, (struct sockaddr *) &s_addport, sizeof(s_addport))== -1) 
			{
				printf("Bind failed!\n");
			}				
			
			printf("Enter input in IPAddress:PortNmber format!\n");
			fflush(stdout);
			printf("?\n");
			
			
			
			
			
		
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
		}
		
		close(tcp_sockd_new);
	}
		
	// close server_socket
	close(tcp_sockd);	
	
	
	return 0;
}
