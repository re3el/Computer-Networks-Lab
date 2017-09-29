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
#include <errno.h>
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
char logtext[10000000];

struct timespec tim, tim2;

int payload;
double playback_del, integral, fractional;
int gamm;
int mu;
int buf_sz;
int target_buf;
int done = 0;

int fp,fd;
int nread;
char *audio_file = "/dev/audio";
char *readValue;

//char input_audio[payload+1];

// variables for udp_socket	
int udp_sockd,server_udp_sockd,option_val;
struct sockaddr_in s_udp_addport,c_udp_addport,s_addport;
char server_udp_port[50];
//int tcp_done = 0;
int Q_t;
int cnt = 0;

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
    audio_buffer->value = malloc(sizeof(char)*buf_sz);
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
        strncpy(audio_buffer->value+audio_buffer->rear,theItemValue,payload);  
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
        strncpy(theItemValue,&audio_buffer->value[audio_buffer->front+5],payload-5);  
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
        //printf("Element #%d = %c\n", aux, audio_buffer->value[aux]);
        aux=(aux+1)%buf_sz;
        aux1--;
    }

    //printf("string: %s\n",audio_buffer->value);
    return;
}

int getCount(circ_que *audio_buffer)
{
    int aux;   
    aux = audio_buffer->count;   
    if(aux < 0)
    {
    	//write(fd,&logtext,strlen(logtext));		
		close(fd);	
		exit(1);
    }
    	
    //printf("front: %d, rear: %d\n", audio_buffer->front, audio_buffer->rear);
    return aux;
}
int test = 0;

void handle_sigpoll (int status)
{	
	//printf("inside sigpoll\n");	
	int s_len = sizeof(s_udp_addport);
	char filereader[payload+1];
	bzero(filereader,payload+1);
	int bytes = recvfrom(udp_sockd, filereader, sizeof(filereader), 0, (struct sockaddr *) &s_udp_addport, &s_len);
	//printf("string received: %s\n\n",filereader,strlen(filereader));		
	//printf("2!\n");
	
	//write(fp,&readValue2,payload-5);
/*  	if (strcmp(filereader,"THE_END_YOGESH")==0) 
	{	
		char var[10];
		bzero(var,10);
		strncpy(var,filereader+5,9);
		var[strlen(var)]='\0';		
		printf("reached end: %s\n",var);
		
		int n = sendto(udp_sockd, var, 9, 0, (struct sockaddr *) &s_udp_addport, s_len);
		//write(fd,&logtext,strlen(logtext));		
		close(fd);	
		close(fp);
		exit(1);	
	} */ 

	s_udp_addport.sin_port = htons(atoi(server_udp_port));
	pthread_mutex_lock(&mutex);
	putItem(&audio_buffer, filereader, payload);
	pthread_mutex_unlock(&mutex);	
	//gettimeofday(&t2, NULL);
	Q_t = getCount(&audio_buffer);
	//printf("\n\n\n");
	
/* 	elapsedTime = ((t2.tv_sec * 1000000 + t2.tv_usec) - (t1.tv_sec * 1000000 + t1.tv_usec));
	char logpkt[30];
	bzero(logpkt,30);
	sprintf(logpkt,"%d:%f\n",(int)Q_t,elapsedTime);		
	strncpy(logtext+cnt,logpkt,strlen(logpkt));
	cnt += strlen(logpkt);    */
	
/*  	test++;
	if(test == 10)
		exit(1);  */	
	char feedback[50];
	
	//printf("After PUT count: %d\n", Q_t);
	sprintf(feedback,"%s %d %d %d","Q",Q_t,target_buf,mu);	
	//printf("After Put: %s\n",feedback);
	
	int n = sendto(udp_sockd, feedback, bytes, 0, (struct sockaddr *) &s_udp_addport, s_len);
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

	char filename[50];
	payload = atoi(argv[4])+5;
	playback_del = atof(argv[5]);
	gamm = atoi(argv[6]);
	mu = (1000000/gamm);
	target_buf = atoi(argv[8]);
	buf_sz = atoi(argv[7]);		
	
	playback_del =  playback_del/1000;
	fractional = modf(playback_del, &integral);
	
	sprintf(filename,"%s",argv[9]);
	filename[strlen(filename)]='\0';
	printf("filename: %s, %zu\n",filename,strlen(filename));
	
	fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	
	struct timeval t1, t2;
    double elapsedTime;
	
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
	//printf("ip: %s\n",ip);
	
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
	//tcp_done = 1;



	struct sigaction sa;	
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGPOLL);
	//sigaddset(&sa.sa_mask, SIGALRM);	
	//sa.sa_handler = &handle_alarm;
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
	//printf("server_udp_port: %s\n",server_udp_port);	
	
	if (strcmp(msg,"KO")==0) 
	{		
		printf("File not found!\n");		
	}
	

		
	if (strcmp(msg,"OK")==0) 
	{						
		
		initializeQueue(&audio_buffer);
		tim.tv_sec  = (int)integral;
		tim.tv_nsec = (fractional*1000000000);			
		while(nanosleep(&tim , &tim) && errno == EINTR)
		{
			
		}

		fp = open("check.au", O_CREAT | O_EXCL | O_RDWR, 0644);
		
		int totPkts = 0;
		int totBytes = 0;
		
		gettimeofday(&t1, NULL);
		while(1)		
		{ 									
			char readValue2[buf_sz];
			pthread_mutex_lock(&mutex);
			getItem(&audio_buffer,&readValue2[0],payload);
			pthread_mutex_unlock(&mutex);						
			
			char check[10];
			bzero(check,10);
			strncpy(check,readValue2,9);
			//printf("\ncheck: %s\n",check);
			if (strcmp(check,"ND_YOGESH")==0) 
			{
				gettimeofday(&t2, NULL);
				printf("reached end\n");
				//printf("string received: %s\n\n",check,strlen(check));
				//write(fd,&logtext,strlen(logtext));		
				close(fd);	
				close(fp);
				
				printf("Total bytes read: %d\n",totBytes);	
				elapsedTime = ((t2.tv_sec * 1000000 + t2.tv_usec) - (t1.tv_sec * 1000000 + t1.tv_usec));
				printf("Completion Time(micro-seconds): %f\n",elapsedTime);
				printf("Throughput(Mbytes/sec): %f\n",(double)(totBytes/elapsedTime)*8);
				exit(1);	
			}
			
			write(fp,&readValue2,payload-5);	
			totBytes+=payload-5;						
			//printf("After GET: %s\n",readValue2);
			//getCount(&audio_buffer);
			//printf("\n\n\n");
			bzero(readValue2,buf_sz);	

			tim2.tv_sec  = 0;
			tim2.tv_nsec = mu*1000;
			while(nanosleep(&tim2 , &tim2) && errno == EINTR)
			{
				
			}	
			
			/* totPkts += 1;
			if(totPkts == 106)
			{
				close(fp);
				exit(1);  
			}  */
		}
		
		//write(fd,&logtext,strlen(logtext));		
		close(fd);
		
		close(udp_sockd);	
		
	}
	
	return 0;
}


 //   ./listen 128.10.25.101 10024 30026 250 2500 33 40000 20000 logc formatted_resume.txt 10 1