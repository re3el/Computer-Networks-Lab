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

#include <time.h>
struct timespec tim, tim2;

struct timeval t1, t2;
double elapsedTime;

char logtext[10000000];

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
double betta = 1.1;

unsigned int packet_spacing;										// variable to store packet-spacing

int child = 0;
char filename[50];

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
			//printf("lamda: %f, *tau: %f    ",lamda,*tau);
			lamda = (1000000/(*tau));
			lamda += a;
			*tau = (1/lamda)*1000000;
			//printf("lamda: %f, tau: %f\n\n",lamda,*tau);
		}
			
		if(Q_t > Q_star)
		{
			//printf("lamda: %f, *tau: %f    ",lamda,*tau);
			lamda = (1000000/(*tau));
			lamda -= a;
			*tau = (1/lamda)*1000000;
			//printf("lamda: %f, tau: %f\n\n",lamda,*tau);
		}
			
	}
	
	if(mode == 1)
	{
		if(Q_t < Q_star)
		{
			//printf("lamda: %f, *tau: %f    ",lamda,*tau);
			lamda = (1000000/(*tau));
			lamda += a;
			*tau = (1/lamda)*1000000;
			//printf("lamda: %f, tau: %f\n\n",lamda,*tau);
		}
			
		if(Q_t > Q_star)
		{
			//printf("lamda: %f, *tau: %f    ",lamda,*tau);
			lamda = (1000000/(*tau));
			lamda *= delta;
			*tau = (1/lamda)*1000000;
			//printf("lamda: %f, tau: %f\n\n",lamda,*tau);
		}
	}
	
	
	if(mode == 2)
	{
		//printf("lamda: %f, *tau: %f    ",lamda,*tau);
		lamda = (1000000/(*tau));
		lamda += epsilon*(Q_star - Q_t);
		if(lamda <= 0)
		{
			lamda = 0;
			*tau = packet_spacing*1000;
		}
		else
			*tau = (1/lamda)*1000000;
		//printf("lamda: %f, tau: %f\n\n",lamda,*tau);
	}
	
	if(mode == 3)
	{
		//printf("lamda: %f, *tau: %f    ",lamda,*tau);
		lamda = (1000000/(*tau));
		gamm = (1000000/gamm);
		lamda = lamda + epsilon*(Q_star - Q_t) - betta*(lamda - gamm);
		if(lamda <= 0)
		{
			lamda = 0;
			*tau = packet_spacing*1000;
		}
		else
			*tau = (1/lamda)*1000000;
		//printf("lamda: %f, tau: %f\n\n",lamda,*tau);
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
	int p_len = sizeof(c_addport);
	int bytes = recvfrom(server_udp_sockd, input_buffer, sizeof(input_buffer), 0,(struct sockaddr *) &p_addport, &p_len);
	//printf("string received: %s, %zu\n",input_buffer,strlen(input_buffer));	

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

	//printf("Q_t: %d, Q_star: %d, gamm: %d\n",Q_t,Q_star,gamm);	
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
	
	int n,payload_size;
	char buf[50];	
	
	udp_port = atoi(argv[2]);	
	payload_size = atoi(argv[3]);
	packet_spacing = strtoul(argv[4],NULL,0);	
	//tau = (1000*packet_spacing);
	mode = atoi(argv[5]);	
	tau = malloc(sizeof(double));
	*tau = (packet_spacing*1000);
	
	sprintf(filename,"%s",argv[6]);
	filename[strlen(filename)]='\0';

	//printf("Just after: filename: %s, %zu\n",filename,strlen(filename));
	
/*	int fd;
	fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);		*/
	
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
	//printf("\n3!\n");					
	while(1)
	{
		/* int saved_stdout;											
		saved_stdout = dup(1);									// save the current_state for bringing execution back to stdout after dup2() */
		//printf("\n4!\n");					
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
		//printf("Message received from Client: %s\n",buf);
	
		// store client_info in two separate buffers: 'client_udp_port' for port_number and 'path' for audio file path directory
				
		int i = 0;
		bzero(client_udp_port,50);
		bzero(path,50);
		while(buf[i]!=' ')
			i++;		
		strncpy(client_udp_port,&buf[0],i);	
		client_udp_port[strlen(client_udp_port)]='\0';		
		//printf("client_udp_port: %s\n",client_udp_port);
		strncpy(path,&buf[i+1],strlen(buf)-i);	
		path[strlen(path)]='\0';
		//printf("path: %s\n",path);	
	
		fp = open(path,O_RDONLY);			
		//if( access( path, F_OK ) == -1 )
		if(fp < 0) 
		{
			char msg[] = "KO";			
			write(tcp_sockd_new,msg,2);
			close(tcp_sockd_new);
			//printf("Error opening file directory:%s at Server\n",path);
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
			//printf("\n5!\n");					
			udp_port+=1;
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
		
		child++;
		char ch[20];
		bzero(ch,20);
		sprintf(ch,"%d",child);
		 
		// forking
		int k = fork();			
		
		// child process 
		if (k==0) 
		{	

			char logname[50];
			//printf("before: %s\n",filename);
			sprintf(logname,"%s%d",filename,child);
			logname[strlen(logname)]='\0';
			printf("logname: %s, strlen: %zu\n",logname,strlen(logname));
			

			int fd;
			fd = open(logname, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	

			//printf("\n6!\n");					
			server_udp_sockd = socket(AF_INET, SOCK_DGRAM, 0);					// UDP Datagram
			if(server_udp_sockd<0)
				perror("Error opening udp_Client_Socket\n");	

			option_val = 1;
			setsockopt(server_udp_sockd, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));

			// UDP Stream	
			s_udp_addport.sin_family = AF_INET;	
			s_udp_addport.sin_addr.s_addr = htonl(INADDR_ANY);
			s_udp_addport.sin_port = htons(udp_port-1);	
			if(bind(server_udp_sockd, (struct sockaddr *) &s_udp_addport, sizeof(s_udp_addport))== -1) 
			{
				printf("Bind failed!\n");
			}

			tau = malloc(sizeof(double));
			*tau = (packet_spacing*1000);	

			fcntl(server_udp_sockd,F_SETOWN,getpid());  
			int flags = fcntl(server_udp_sockd, F_GETFL);
			fcntl(server_udp_sockd, F_SETFL, flags | O_ASYNC);
						

			//printf("inside child\n");
			c_addport.sin_port = htons(atoi(client_udp_port));
			///printf("Client IP: %s, Client_UDP_Port_Number: %d\n\n",inet_ntoa(c_addport.sin_addr), ntohs(c_addport.sin_port));
			int c_len = sizeof(c_addport);

			char filereader[payload_size+1];
			bzero(filereader,payload_size+1);
			int nread;
			int totPkts = 0;
			int totBytes = 0;
			int cnt = 0;
			gettimeofday(&t1, NULL);
			while ((nread = read(fp,filereader,payload_size)) > 0)
			{					
				//printf("Client IP: %s, Client_UDP_Port_Number: %d\n\n",inet_ntoa(c_addport.sin_addr), ntohs(c_addport.sin_port));				
				//printf("\nInside while!\n");					
				//write(udp_sockd,filereader,nread);				
				//printf("string sent: %s\n",filereader);
				n = sendto(server_udp_sockd, filereader, nread, 0, (struct sockaddr *) &c_addport, c_len);						
				if (n < 0) 
				{
				  perror("Error at Client: sendto()!\n");
				  exit(1);
				}
				gettimeofday(&t2, NULL);
				elapsedTime = ((t2.tv_sec * 1000000 + t2.tv_usec) - (t1.tv_sec * 1000000 + t1.tv_usec));
				char logpkt[30];
				bzero(logpkt,30);
				sprintf(logpkt,"%d:%f\n",(int)lamda,elapsedTime);		
				strncpy(logtext+cnt,logpkt,strlen(logpkt));
				cnt += strlen(logpkt);   

				// respectively update the variables
				totPkts += 1;
				totBytes += n;	
				
				
				/* if(totPkts == 150)
					break;  */
				// put the program to sleep for packet-spacing timeperiod
				//usleep(*tau); 	
				tim.tv_sec  = 0;
				tim.tv_nsec = (int)(*tau)*1000;	

				while(nanosleep(&tim , &tim) && errno == EINTR)
				{
					
				}			
				//usleep(900000);
				//printf("Packets sent: %d, tau: %f\n",totPkts,*tau);	
				bzero(filereader,payload_size);				
			}
			
			printf("Packets sent: %d\n",totPkts);
			printf("Bytes sent: %d\n",totBytes);
			//printf("logtext: \n%s",logtext);
			
			write(fd,&logtext,strlen(logtext));		
			close(fd);
			
			char end[8];
			bzero(end,payload_size);				
			strncpy(end,"THE_END",7);
			n = sendto(server_udp_sockd, end, 7, 0, (struct sockaddr *) &c_addport, c_len);						
			if (n < 0) 
			{
			  perror("Error at Client: sendto()!\n");
			  exit(1);
			}

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
			//printf("hai\n");
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




