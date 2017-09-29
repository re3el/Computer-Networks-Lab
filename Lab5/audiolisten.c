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
#include <math.h>

#include <pthread.h>

#include <time.h>

#define MAX_ITEMS 40000

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct timeval t1, t2;
double elapsedTime;

struct timespec tim, tim2;

int payload;
int playback_del;
int gamm;
int mu;
int buf_sz;
int target_buf;
int done = 0;

int fp;
int nread;
char *audio_file = "/dev/audio";
char *readValue;

//char input_audio[payload+1];

// variables for udp_socket	
int udp_sockd,server_udp_sockd,option_val;
struct sockaddr_in s_udp_addport,c_udp_addport,s_addport;
char server_udp_port[50];
int tcp_done = 0;

typedef struct circ_que_struct
{
    int front;
    int rear;
    int count;
    char *value;
} circ_que;
circ_que audio_buffer;

void initializeQueue(circ_que *audio_buffer)
{
    int i;
    audio_buffer->count =  0;
    audio_buffer->front =  0;
    audio_buffer->rear  =  0;
    audio_buffer->value = malloc(sizeof(char)*payload);
    bzero(audio_buffer->value,buf_sz);
    return;
}

int isEmpty(circ_que *audio_buffer)
{
    if(audio_buffer->count==0)
        return(1);
    else
        return(0);
}

int putItem(circ_que *audio_buffer, char theItemValue[], int payload)
{
    if(audio_buffer->count>=buf_sz)
    {
        printf("The queue is full\n");
        printf("You cannot add items\n");
        return(-1);
    }
    else
    {
        strncpy(audio_buffer->value+audio_buffer->front,theItemValue,payload);  
        audio_buffer->count += payload;
        //audio_buffer->value[audio_buffer->rear] = theItemValue;
        audio_buffer->rear = (audio_buffer->rear+payload)%buf_sz;
    }
}

int getItem(circ_que *audio_buffer, char *theItemValue, int payload)
{
    if(isEmpty(audio_buffer))
    {
        //printf("done playing\n");
        return(-1);
    }
    else
    {
        strncpy(theItemValue,&audio_buffer->value[audio_buffer->front],payload);  
        //*theItemValue=audio_buffer->value[audio_buffer->front];
        audio_buffer->front=(audio_buffer->front+payload)%buf_sz;
        audio_buffer->count -= payload;
        return(0);
    }
}

void printQueue(circ_que *audio_buffer)
{
    int aux, aux1;
    aux  = audio_buffer->front;
    aux1 = audio_buffer->count;
    while(aux1>0)
    {
        printf("Element #%d = %c\n", aux, audio_buffer->value[aux]);
        aux=(aux+1)%buf_sz;
        aux1--;
    }

    printf("string: %s\n",audio_buffer->value);
    return;
}

int getCount(circ_que *audio_buffer)
{
    int aux;   
    aux = audio_buffer->count;   
    printf("front: %d, rear: %d\n", audio_buffer->front, audio_buffer->rear);
    return aux;
}

void handle_alarm( int sig ) 
{    
	ualarm(90000,0);					
	//printf("1!\n");					

	char readValue2[buf_sz];
	//pthread_mutex_lock(&mutex);
	//int i;

/*	for(i=0; i<payload; i++)
	{
		getItem(&audio_buffer, &readValue);
		if(done == 1)
		{
			printf("The End!\n");					
			break;
		}
		//printf("readValue = %d\n", readValue);
		write(fp,&readValue,1);
	}	*/
	getItem(&audio_buffer,&readValue2[0],payload);
	//pthread_mutex_unlock(&mutex);
	write(fp,&readValue2,payload);
	printf("After GET count: %d\n", getCount(&audio_buffer));
	bzero(readValue2,buf_sz);
	//gettimeofday(&t2, NULL);
	//exit(1);
	//printf("2\n");		
}

void handle_sigpoll (int status)
{	
	//printf("inside sigpoll\n");	
	int s_len = sizeof(s_udp_addport);
	char filereader[payload+1];
	
	int bytes = recvfrom(udp_sockd, filereader, sizeof(filereader), 0, (struct sockaddr *) &s_udp_addport, &s_len);
	//printf("string received: %s, %zu\n",filereader,strlen(filereader));		
	//printf("2!\n");
	s_udp_addport.sin_port = htons(atoi(server_udp_port));
	pthread_mutex_lock(&mutex);
/*	int i;
	for(i=0; i<bytes; i++)
	{
	    putItem(&audio_buffer, filereader[i]);
	}*/
	putItem(&audio_buffer, filereader, bytes);
	pthread_mutex_unlock(&mutex);	
	//printf("written something!\n");
	char feedback[50];
	int Q_t = getCount(&audio_buffer);
	printf("After PUT count: %d\n", Q_t);
	sprintf(feedback,"%s %d %d %d","Q",Q_t,target_buf,mu);	
	printf("feedback: %s\n",feedback);
	
	//	int s_len = sizeof(s_udp_addport);

	server_udp_sockd = socket(AF_INET, SOCK_DGRAM, 0);					// UDP Datagram
	if(server_udp_sockd<0)
		perror("Error opening udp_Client_Socket\n");

	//printf("client IP: %s, client Port_Number: %d\n\n",inet_ntoa(s_udp_addport.sin_addr), ntohs(s_udp_addport.sin_port));
	int n = sendto(server_udp_sockd, feedback, bytes, 0, (struct sockaddr *) &s_udp_addport, s_len);
	if (n < 0) 
	{
	  perror("Error at Client: sendto()!\n");
	  exit(1);
	}	
}


int main(int argc, char* argv[])
{
	if(argc<11)
	{
		printf("Usage: audiolisten server-ip server-tcp-port client-udp-port payload-size playback-del gamm buf-sz target-buf logfile-c path_filename\n");
		exit(1);
	}

	payload = atoi(argv[4]);
	playback_del = atoi(argv[5]);
	//gamm = atoi(argv[6]);
	mu = atoi(argv[6])*1000;
	target_buf = atoi(argv[8]);
	buf_sz = atoi(argv[7]);		

	char udp_port[10];
	sprintf(udp_port,"%s",argv[3]);
	udp_port[strlen(udp_port)]='\0';		

	readValue = malloc(sizeof(char)*payload);
	//audio_buffer->value = malloc(sizeof(char)*payload);

	char buffer[1000];
	int tcp_sockd,port_no,n;
	struct sockaddr_in s_tcp_addport;	
	struct hostent *server;
	
	// stores client_info which gets transferred through client_pipe
	char clientinfo[60];								
	char directory[50];
	sprintf(directory,"%s",argv[10]);
	directory[strlen(directory)]='\0';
	
	
	// clientinfo: 'udp_port directory'	
	sprintf(clientinfo,"%s %s",udp_port,directory);	
	printf("Message sent from client: %s\n",clientinfo);

	// connect to the server socket
	port_no = atoi(argv[2]);
	tcp_sockd = socket(AF_INET, SOCK_STREAM, 0);
	if(tcp_sockd < 0)
		perror("Error opening Socket!\n");

	char ip[20];
	sprintf(ip,"%s",argv[1]);
	ip[strlen(ip)]='\0';
	printf("ip: %s\n",ip);
	
	bzero((char *) &s_tcp_addport, sizeof(s_tcp_addport));
	s_tcp_addport.sin_family = AF_INET;
	s_tcp_addport.sin_port = htons(port_no);
	inet_aton(ip,&(s_tcp_addport.sin_addr));
	printf("Server IP: %s, Server Port_Number: %d\n\n",inet_ntoa(s_tcp_addport.sin_addr), ntohs(s_tcp_addport.sin_port));
		 
	if (connect(tcp_sockd,(struct sockaddr *) &s_tcp_addport,sizeof(s_tcp_addport)) < 0) 
        perror("Error at Connect!\n");
		 
	// write to server_socket
	n = write(tcp_sockd,clientinfo,sizeof(clientinfo));
    if (n < 0) 
		perror("Error at client write!\n");
	
	// read from server_socket
    bzero(buffer,1000);
    n = read(tcp_sockd,buffer,sizeof(buffer));
    if (n < 0) 
        perror("Error at client read!\n");
    printf("Reply from Server: %s\n",buffer);
    close(tcp_sockd);	
	tcp_done = 1;



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

	udp_sockd = socket(AF_INET, SOCK_DGRAM, 0);					// UDP Datagram
	if(udp_sockd<0)
		perror("Error opening udp_Client_Socket\n");	

	fcntl(udp_sockd,F_SETOWN,getpid());           
	fcntl(udp_sockd, F_SETFL, FASYNC); 	

	option_val = 1;
	setsockopt(udp_sockd, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));
	
	// binding the client_udp_socket
	c_udp_addport.sin_family = AF_INET;
	c_udp_addport.sin_port = htons(atoi(udp_port));
	c_udp_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(udp_sockd, (struct sockaddr *) &c_udp_addport, sizeof(c_udp_addport))== -1) 
	{
		printf("Bind failed!\n");
	} 	



	// store client_info in two separate buffers: 'server_udp_port' for server_port_number and 'msg' for OK/KO	
	char msg[4];		
	int i = 0;
	bzero(server_udp_port,50);
	bzero(msg,4);
	while(buffer[i]!=' ')
		i++;		
	strncpy(msg,&buffer[0],i);	
	msg[strlen(msg)]='\0';		
	printf("msg: %s\n",msg);
	strncpy(server_udp_port,&buffer[i+1],strlen(buffer)-i);	
	server_udp_port[strlen(server_udp_port)]='\0';
	printf("server_udp_port: %s\n",server_udp_port);	
	
	if (strcmp(msg,"KO")==0) 
	{		
		printf("File not found!\n");		
	}
	

		
	if (strcmp(msg,"OK")==0) 
	{						
		
		initializeQueue(&audio_buffer);
		
		tim.tv_sec  = 2;
		tim.tv_nsec = 500000000L;	

		while(1)
		{
			if(nanosleep(&tim , &tim) == 0)
				break;
			//printf("mybad!\n");
			//nanosleep(&tim2 , &tim2);	
		}
		
		/*	
		usleep(1000000);
		usleep(1000000);
		usleep(1000000);
		usleep(1000000);*/
		
		// TODO: check if the time delay is 2.5 secs
		//sleep(100);
		//usleep(900000);	
				
		//sleep(2);
		//gettimeofday(&t1, NULL);			
		//fp = open("/dev/audio", O_WRONLY, 0);
		
		while(1)		
		{ 		
			usleep(10000);	
			//ualarm(90000,0);					
			//printf("1!\n");					

			char readValue2[buf_sz];
			pthread_mutex_lock(&mutex);
			//int i;

		/*	for(i=0; i<payload; i++)
			{
				getItem(&audio_buffer, &readValue);
				if(done == 1)
				{
					printf("The End!\n");					
					break;
				}
				//printf("readValue = %d\n", readValue);
				write(fp,&readValue,1);
			}	*/
			getItem(&audio_buffer,&readValue2[0],payload);
			pthread_mutex_unlock(&mutex);
			//write(fp,&readValue2,payload);
			printf("After GET count: %d\n", getCount(&audio_buffer));
			bzero(readValue2,buf_sz);					
		}
		close(udp_sockd);	
		
	}
	
	return 0;
}
