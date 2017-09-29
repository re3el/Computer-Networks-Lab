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

#include <pthread.h>
#include <curses.h>

WINDOW *top;
WINDOW *bottom;
int line=1; // Line position of top
int input=1; // Line position of top
int maxx,maxy; // Screen dimensions
pthread_mutex_t mutexsum = PTHREAD_MUTEX_INITIALIZER;  

volatile sig_atomic_t print_flag = false;
volatile int STOP = 0; 
int wait_flag = 1;                    /* TRUE while no signal received */
static int read_flag = 0;
char global_buf[51];
char input_buffer[1024];
char output_buffer[52];
int flag_setup = 0;

// variables for socket	
int sock_id,port_no,option_val;
struct sockaddr_in s_addport,p_addport,c_addport;	
struct hostent *hostp; 
struct termios oldtermios;


int ttyraw(int fd)
{
	struct termios newtermios;
	if(tcgetattr(fd, &oldtermios) < 0)
		return(-1);
	newtermios = oldtermios;

	newtermios.c_lflag &= ~(ICANON);
	newtermios.c_cc[VERASE] = 127;
	
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
    int bytes = recvfrom(sock_id, input_buffer, sizeof(input_buffer), 0, (struct sockaddr *) &c_addport, &c_len);	

	if (strcmp(input_buffer,"KO")==0) 
	{		
		alarm(0);
		printf("| doesn't want to chat\n");
		printf("?\n");	
	}
	
	if (strcmp(input_buffer,"OK")==0) 
	{	
		printf("Connection Established! \n");
		alarm(0);
		flag_setup = 1;
		
		initscr();		
		getmaxyx(stdscr,maxy,maxx);

		top = newwin(maxy/2,maxx,0,0);
		bottom= newwin(maxy/2,maxx,maxy/2,0);

		scrollok(top,TRUE);
		scrollok(bottom,TRUE);
		box(top,'|','=');
		box(bottom,'|','-');

		wsetscrreg(top,1,maxy/2-2);
		wsetscrreg(bottom,1,maxy/2-2);
		wrefresh(top);
        wrefresh(bottom);
				
		printf("\n >");
		fflush(stdout);			
	}
	
	if (strcmp(input_buffer,"E")==0) 
	{
		flag_setup = 0;
		if(flag_setup == 0)
		{
			tcgetattr(0, &oldtermios);			
		}
		
		endwin();      
        pthread_mutex_destroy(&mutexsum);
		
		printf("\n| chat terminated\n");
		printf("?\n");		
				
	}
	
	if(input_buffer[0]=='D')
	{	
		char sub1[52];
		strncpy(sub1, input_buffer + 1, (int)strlen(input_buffer)-1);
		sub1[(int)strlen(input_buffer)-1]='\0';
		wrefresh(top);
		wrefresh(bottom);		
		
		mvwprintw(bottom,input,2,">");
		mvwprintw(bottom,input,3,sub1);
		
		pthread_mutex_lock (&mutexsum);

		if(line!=maxy/2-2)
			line++;
		else
		scroll(top);

		// scroll the bottom if the line number exceed height
		if(input!=maxy/2-2)
			input++;
		else
		scroll(bottom);

		pthread_mutex_unlock (&mutexsum);
	}
	
	fflush(stdin);	
	fflush(stdout);	
	fflush(stdout);	

	if(sizeof(global_buf)>0)
	{		
		global_buf[sizeof(global_buf)-1]='\0';
		wrefresh(top);
		wrefresh(bottom);
		mvwprintw(top,line,3,global_buf);
	}
}

void chopnl(char *s) {              //strip '\n'
    s[strcspn(s,"\n")] = '\0';
}

int main(int argc, char *argv[])
{	    												
	// check the arguments
	if(argc!=2)
	{
		printf("Usage: %s portnumber \n",argv[0]);
		exit(1);
	}
	
 	struct sigaction sa;	
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
	}	
	
	bzero((char *) &s_addport, sizeof(s_addport));	
	bzero((char *) &p_addport, sizeof(p_addport));		
	
	// Setup UDP Socket
	sock_id = socket(AF_INET, SOCK_DGRAM, 0);					
	if(sock_id<0)
		perror("Error opening Socket\n");	
			
	fcntl(sock_id,F_SETOWN,getpid());           /* allow the process to receive SIGIO */
	fcntl(sock_id, F_SETFL, FASYNC); 	
	
	/* setsockopt: to rerun the server immediately after killing it */
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
	
	printf("Enter input in IPAddress:PortNmber format!\n");
	fflush(stdout);
	printf("?\n");
		
	int ret;
	ssize_t bytes;
	char buf[51];
	struct pollfd fds[2];

	/* Descriptor zero is stdin */
	fds[0].fd = 0;
	fds[1].fd = sock_id;
	fds[0].events = POLLIN | POLLPRI;
	fds[1].events = POLLIN | POLLPRI;
	
	//printf("\n-2\n");
	fflush(stdout);
	
	while (1) 
	{
		if(flag_setup == 1)
			ttyraw(0);
		
		if(flag_setup == 0)
		{
			if(tcgetattr(0, &oldtermios) < 0)
				return(-1);			
		}
		
		/* Call poll() */
		ret = poll(fds, 2, -1);		
		fflush(stdout);
				
		if (ret > 0) 
		{
			/* Regardless of requested events, poll() can always return these */
			if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) 
			{
				printf("Poll Error @stdin\n");
				break;
			}
			if (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) 
			{
				printf("Poll Error @socket\n");
				break;
			}

			/* Data to be read from stdin */
			if (fds[0].revents & (POLLIN | POLLPRI)) 
			{				
				bzero(output_buffer,52);
				bzero(input_buffer,1024);
				bzero(buf,51);							
				
				fflush(stdout);				

				if(flag_setup == 0)
				{												
					fgets(buf,51,stdin);
					chopnl(buf);
				}
										
				if(flag_setup==1)				
				{																		
					mvwprintw(top,line,2,">");
						
					int i = 0;
					char c_in;
					fflush(stdin);
					fflush(stdout);
					
					wrefresh(top);
					wrefresh(bottom);
					int x_index = 3;
					while(1)
					{	
						if(i!=50)
						{
							c_in =  mvwgetch(top, line, x_index);							
							
							if((c_in == '\n'))
							{															
								global_buf[i] = '\0';							
								break;
							}
							
							if(c_in == '\b')
							{
								printf("in here!\n");
								global_buf[i] = '\0';
								i--;
							}
																					
							global_buf[i] = c_in;		
							global_buf[i+1] = '\0';						
							fflush(stdin);							
							i++;	
							x_index++;							
						}
						
						if(i==50)
						{
							global_buf[i] = '\0';							
							break;
						}
					}
					
					pthread_mutex_lock (&mutexsum);

					if(line!=maxy/2-2)
						line++;
					else
						scroll(top);

					// scroll the bottom if the line number exceed height
					if(input!=maxy/2-2)
						input++;
					else
						scroll(bottom);

					pthread_mutex_unlock (&mutexsum);
					
					global_buf[strlen(global_buf)] = '\0';														
						
					strcpy(buf,global_buf);					
					bzero(global_buf,51);										
				}
				
				if(strcmp(buf,"q")!=0 && strcmp(buf,"e")!=0 && flag_setup==0)
				{
					//printf("entered setup phase!\n\n");
					int i=0;
					int port_no;
					char port[10];
					while(buf[i]!=':')
						i++;		
					char ip[i];
					strncpy(ip,&buf[0],i);	
					ip[i]='\0';												
					strncpy(port,&buf[i+1],strlen(buf));
					port[strlen(port)]='\0';
														
					// server_info
					port_no = atoi(port);	
					p_addport.sin_family = AF_INET;
					p_addport.sin_port = htons(port_no);
					inet_aton(ip,&(p_addport.sin_addr));
					//printf("Peer IP: %s, Peer Port_Number: %d\n",inet_ntoa(p_addport.sin_addr), ntohs(p_addport.sin_port));
					
					bzero(buf,51);
					strcpy(output_buffer,"wannatalk");
					//printf("Sending: %s\n", output_buffer);
					bytes = sendto(sock_id, output_buffer, 10, 0,(struct sockaddr *)&p_addport, sizeof(struct sockaddr_in));
					if (bytes < 0) 
					{
						printf("Error - sendto error: %s\n", strerror(errno));
						break;
					}
					
					//signal( SIGALRM, handle_alarm ); 
					alarm(7);

				}
				
				if(strcmp(buf,"q")==0 && flag_setup == 0 )
				{	
					bzero(buf,51);
					break;				
				} 
				
				if(strcmp(buf,"e")==0 && flag_setup == 1 )
				{					
					//printf("entered termination phase!\n");		
					strcpy(output_buffer,"E");					
					bytes = sendto(sock_id, output_buffer, 2, 0,(struct sockaddr *)&p_addport, sizeof(struct sockaddr_in));
					if (bytes < 0) 
					{
						printf("Error - sendto error: %s\n", strerror(errno));
						break;
					}					
										
					flag_setup = 0;								
					if(flag_setup == 0)
					{
						if(tcgetattr(0, &oldtermios) < 0)
							return(-1);
					}
					
					
					endwin();      
					pthread_mutex_destroy(&mutexsum);
					printf("Connection Terminated!\n",buf);	
					
					printf("?\n");
					fflush(stdout);
					bzero(buf,51);
				} 
				
  				if(strcmp(buf,"e")!=0 && flag_setup == 1 )
				{
					//printf("entered chat phase!\n");		
					strcpy(output_buffer,"D");
					strcat(output_buffer,buf);					
					//mvwprintw(top,line,2,buf);
					bytes = sendto(sock_id, output_buffer, 52, 0,(struct sockaddr *)&p_addport, sizeof(struct sockaddr_in));
					if (bytes < 0) 
					{
						printf("Error - sendto error: %s\n", strerror(errno));
						break;
					}	
 					bzero(buf,51);
					//printf("%s\n",buf);					
				}  
								
			}

			/* Data to be read from socket */
			if (fds[1].revents & (POLLIN | POLLPRI)) 
			{							
				
				if (strcmp(input_buffer,"wannatalk")==0) 
				{
					printf("| chat request from %s %d\n",inet_ntoa(c_addport.sin_addr), ntohs(c_addport.sin_port));
					printf("?\n");
					
					char buf[4];
					fgets(buf,4,stdin);
					chopnl(buf);				
					//printf("inside buffer: %s\n",buf);
					
					if(strcmp(buf,"c")==0)
					{
						bzero(output_buffer,20);
						strcpy(output_buffer,"OK");
						bytes = sendto(sock_id, output_buffer, 3, 0,(struct sockaddr *)&c_addport, sizeof(struct sockaddr_in));
						if (bytes < 0) 
						{
							printf("Error - sendto error: %s\n", strerror(errno));
							break;
						}
						printf("Connection Establised!\n");
						
						
						initscr(); 
						cbreak();						
						getmaxyx(stdscr,maxy,maxx);

						top = newwin(maxy/2,maxx,0,0);
						bottom= newwin(maxy/2,maxx,maxy/2,0);

						scrollok(top,TRUE);
						scrollok(bottom,TRUE);
						box(top,'|','=');
						box(bottom,'|','-');

						wsetscrreg(top,1,maxy/2-2);
						wsetscrreg(bottom,1,maxy/2-2);
						wrefresh(top);
						wrefresh(bottom);
						
						printf("\n >");						
						fflush(stdout);
						flag_setup = 1;
												
						p_addport.sin_family = AF_INET;
						p_addport.sin_port = c_addport.sin_port;
						p_addport.sin_addr.s_addr = c_addport.sin_addr.s_addr;
						//printf("Peer IP: %s, Peer Port_Number: %d\n",inet_ntoa(p_addport.sin_addr), ntohs(p_addport.sin_port));
						
					}
					
					if(strcmp(buf,"n")==0)
					{
						bzero(output_buffer,20);
						strcpy(output_buffer,"KO");
						bytes = sendto(sock_id, output_buffer, 3, 0,(struct sockaddr *)&c_addport, sizeof(struct sockaddr_in));
						if (bytes < 0) 
						{
							printf("Error - sendto error: %s\n", strerror(errno));
							break;
						}
						printf("?\n");						
						fflush(stdout);
					}
					
				}

				bzero(input_buffer,1024);
			}
			
			//printf("\n9\n");
			//fflush(stdout);
		}
		
		//printf("\n10\n");
		//fflush(stdout);
	}
	ttyreset(0);
	
	// close server_socket
	close(sock_id);	
	
	return 0;
}
