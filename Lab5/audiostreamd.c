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


char global_buf[51];
char input_buffer[100];
char output_buffer[52];
int flag_setup = 0;

char client_udp_port[50];
char path[50];
int fp;

int mode;
double *tau;
double lamda = 0;
int Q_t,Q_star,gamm;
int a = 1;
double delta = 0.5;
double epsilon = 0.1;
double betta = 0.5;

// variables for udp_socket	
int udp_sockd,server_udp_sockd,udp_port,option_val;
struct sockaddr_in s_addport,p_addport,c_addport,s_udp_addport;	
struct hostent *hostp; 
int tcp_done = 0;

void congestion(int mode)
{
	if(mode == 0)
	{
		if(Q_t < Q_star)
		{
			printf("lamda: %f, *tau: %f    ",lamda,*tau);
			lamda = (1000000/(*tau));
			lamda += a;
			*tau = (1/lamda)*1000000;
			printf("lamda: %f, tau: %f\n\n",lamda,*tau);
		}
			
		if(Q_t > Q_star)
		{
			printf("lamda: %f, *tau: %f    ",lamda,*tau);
			lamda = (1000000/(*tau));
			lamda -= a;
			*tau = (1/lamda)*1000000;
			printf("lamda: %f, tau: %f\n\n",lamda,*tau);
		}
			
	}
	
	if(mode == 1)
	{
		if(Q_t < Q_star)
		{
			printf("lamda: %f, *tau: %f    ",lamda,*tau);
			lamda = (1000000/(*tau));
			lamda += a;
			*tau = (1/lamda)*1000000;
			printf("lamda: %f, tau: %f\n\n",lamda,*tau);
		}
			
		if(Q_t > Q_star)
		{
			printf("lamda: %f, *tau: %f    ",lamda,*tau);
			lamda = (1000000/(*tau));
			lamda *= delta;
			*tau = (1/lamda)*1000000;
			printf("lamda: %f, tau: %f\n\n",lamda,*tau);
		}
	}
	
	
	if(mode == 2)
	{
		printf("lamda: %f, *tau: %f    ",lamda,*tau);
		lamda = (1000000/(*tau));
		lamda += epsilon*(Q_star - Q_t);
		*tau = (1/lamda)*1000000;
		printf("lamda: %f, tau: %f\n\n",lamda,*tau);
	}
	
	if(mode == 3)
	{
		printf("lamda: %f, *tau: %f    ",lamda,*tau);
		lamda = (1000000/(*tau));
		lamda = lamda + epsilon*(Q_star - Q_t) - betta*(lamda - gamm);
		*tau = (1/lamda)*1000000;
		printf("lamda: %f, tau: %f\n\n",lamda,*tau);
	}
}



void handler(int signal) 
{    
	// WNOHANG: return 0 if the child process isn't terminated and child process pid if terminated. It's a non blocking call from the parent to reap zombies
	// while loop used to reap more than 1 zombie that may exist
	while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}  
}

void handle_sigpoll (int status)
{	

	//printf("inside sigpoll, tcp_done: %d\n",tcp_done);
	int c_len = sizeof(c_addport);
	int bytes = recvfrom(server_udp_sockd, input_buffer, sizeof(input_buffer), 0, (struct sockaddr *) &c_addport, &c_len);
	printf("string received: %s, %zu\n",input_buffer,strlen(input_buffer));	

	if(input_buffer[0]=='Q')
	{
		char feedback[50];		
		sprintf(feedback,"%s",input_buffer);		
		int i = 0;
		char Qt_buf[20];
		char Qstar_buf[20];
		char gamma_buf[20];
		//char Qt_buf[20];
	
		bzero(Qt_buf,20);
		bzero(Qstar_buf,20);
		bzero(gamma_buf,20);
	
		char *pch;
	
		pch = strtok (feedback," ");		
		pch = strtok (NULL," ");	
		sprintf(Qt_buf,"%s",pch);
		Q_t = atoi(Qt_buf);
	
		pch = strtok (NULL," ");	
		sprintf(Qstar_buf,"%s",pch);
		Q_star = atoi(Qstar_buf);
	
		pch = strtok (NULL," ");	
		sprintf(gamma_buf,"%s",pch);
		gamm = atoi(gamma_buf);			
	}

	printf("Q_t: %d, Q_star: %d, gamm: %d\n",Q_t,Q_star,gamm);	
	congestion(mode);

}

void chopnl(char *s) {              //strip '\n'
    s[strcspn(s,"\n")] = '\0';
}

int main(int argc, char *argv[])
{	    												
	// check the arguments
	if(argc!=7)
	{
		printf("Usage: audiostreamd tcp-port udp-port payload-size packet-spacing mode logfile-s \n");
		exit(1);
	}
	printf("1!\n");					
	int n,payload_size;
	char buf[50];
	unsigned int packet_spacing;										// variable to store packet-spacing
		
	udp_port = atoi(argv[2]);	
	payload_size = atoi(argv[3]);
	packet_spacing = strtoul(argv[4],NULL,0);	
	//tau = (1000*packet_spacing);
	mode = atoi(argv[5]);	
	tau = malloc(sizeof(double));
	*tau = (packet_spacing*1000);

	// TCP stream
	int tcp_sockd,tcp_sockd_new,tcp_port,cmp;
	//struct sockaddr_in s_addport,c_addport;		
	bzero((char *) &s_addport, sizeof(s_addport));	
	
		
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
	printf("\n3!\n");					
	while(1)
	{
		/* int saved_stdout;											
		saved_stdout = dup(1);									// save the current_state for bringing execution back to stdout after dup2() */
		printf("\n4!\n");					
		// full association 
		bzero((char *) &c_addport, sizeof(c_addport));
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
				
		int i = 0;
		bzero(client_udp_port,50);
		bzero(path,50);
		while(buf[i]!=' ')
			i++;		
		strncpy(client_udp_port,&buf[0],i);	
		client_udp_port[strlen(client_udp_port)]='\0';		
		printf("client_udp_port: %s\n",client_udp_port);
		strncpy(path,&buf[i+1],strlen(buf)-i);	
		path[strlen(path)]='\0';
		printf("path: %s\n",path);	
	
		fp = open(path,O_RDONLY);			
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
			printf("\n5!\n");					
			//udp_port+=1;
		}

		close(tcp_sockd_new);

 		// SIGCHLD for creating an asynchronous non blocking call
		struct sigaction sa;
		sa.sa_handler = &handler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
		if (sigaction(SIGCHLD, &sa, 0) == -1) {
			perror(0);			
			exit(1);
		}  

		//struct sigaction sa;	
		//sigemptyset(&sa.sa_mask);
		sigaddset(&sa.sa_mask, SIGPOLL);
		//sigaddset(&sa.sa_mask, SIGALRM);	
		//sa.sa_handler = &handle_alarm;
		//sa.sa_flags = 0;	 

		sa.sa_handler = &handle_sigpoll;	
		if (sigaction(SIGPOLL, &sa, 0) == -1) {
			perror(0);			
			exit(1);
		}	 
		printf("\n6!\n");					
		server_udp_sockd = socket(AF_INET, SOCK_DGRAM, 0);					// UDP Datagram
		if(server_udp_sockd<0)
			perror("Error opening udp_Client_Socket\n");	

		 

		option_val = 1;
		setsockopt(server_udp_sockd, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));

		// UDP Stream	
		s_udp_addport.sin_family = AF_INET;	
		s_udp_addport.sin_addr.s_addr = htonl(INADDR_ANY);
		s_udp_addport.sin_port = htons(udp_port);	
		if(bind(server_udp_sockd, (struct sockaddr *) &s_udp_addport, sizeof(s_udp_addport))== -1) 
		{
			printf("Bind failed!\n");
		}	 

				
		// forking
		int k = fork();			
		
		// child process 
		if (k==0) 
		{	
			tau = malloc(sizeof(double));
			*tau = (packet_spacing*1000);
			
			fcntl(server_udp_sockd,F_SETOWN,getpid());           
			fcntl(server_udp_sockd, F_SETFL, FASYNC);

			printf("\n7!\n");																				

			printf("inside child\n");
			c_addport.sin_port = htons(atoi(client_udp_port));
			//printf("Client IP: %s, Client_UDP_Port_Number: %d\n\n",inet_ntoa(c_addport.sin_addr), ntohs(c_addport.sin_port));
			
 			udp_sockd = socket(AF_INET, SOCK_DGRAM, 0);					
			if(udp_sockd<0)
				perror("Error opening Socket\n");														

			option_val = 1;
			setsockopt(udp_sockd, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));
			
			//udp_port -= 1; 												
			
			char filereader[payload_size+1];
			int nread;
			int totPkts = 0;
			int totBytes = 0;
			while ((nread = read(fp,filereader,payload_size)) > 0)
			{	
				//printf("\n8!\n");					
				//printf("\nInside while!\n");					
				//write(udp_sockd,filereader,nread);
				int c_len = sizeof(c_addport);
				n = sendto(udp_sockd, filereader, nread, 0, (struct sockaddr *) &c_addport, c_len);						
				if (n < 0) 
				{
				  perror("Error at Client: sendto()!\n");
				  exit(1);
				}
			
				// respectively update the variables
				totPkts += 1;
				totBytes += n;	
				
				
				if(totPkts == 10)
					break;
				// put the program to sleep for packet-spacing timeperiod
				usleep(*tau); 				
				printf("Packets sent: %d, tau: %f\n",totPkts,*tau);	
				bzero(filereader,payload_size);				
			}
			
			printf("Packets sent: %d\n",totPkts);
			printf("Bytes sent: %d\n",totBytes);
			
			close(udp_sockd);
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
			printf("hai\n");
			tcp_done = 0;
			memset(buf, 0, sizeof(buf));					// clearing buf memory for serving concurrent requests					
			//close(tcp_sockd_new);
		}
		
		//close(tcp_sockd_new);
	}
		
	// close server_socket
	close(tcp_sockd);	
	
	
	return 0;
}




