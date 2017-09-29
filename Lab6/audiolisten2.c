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
int retransmit_flag;
double playback_del, integral, fractional;
int gamm;
int mu;
int buf_sz;
int target_buf;
int done = 0;
int drop_pkt;
int k_drop = 0;

int fp,fd;
int nread;
char *audio_file = "/dev/audio";
char *readValue;
int prevSeq = 9999;
//char input_audio[payload+1];

// variables for udp_socket	
int udp_sockd,server_udp_sockd,option_val;
struct sockaddr_in s_udp_addport,c_udp_addport,s_addport;
char server_udp_port[50];
//int tcp_done = 0;
int Q_t;
int cnt = 0;
int test = 0;

typedef struct circ_que_struct
{
    int front;
    int rear;
    int count;
    char *value;
    char *hole;
} circ_que;
circ_que audio_buffer;

void initializeQueue(circ_que *audio_buffer)
{
    int i;
    audio_buffer->count =  0;
    audio_buffer->front =  0;
    audio_buffer->rear  =  0;
    audio_buffer->value = malloc(sizeof(char)*buf_sz);
    audio_buffer->hole = malloc(sizeof(char)*buf_sz);
    bzero(audio_buffer->value,buf_sz);
    bzero(audio_buffer->hole,buf_sz);
    return;
}

int isEmpty(circ_que *audio_buffer)
{
    if(audio_buffer->count==0)
        return(1);
    else
        return(0);
}

int putItem(circ_que *audio_buffer, char theItemValue[], int payload, int seqNum, int marSeqNum)
{
    if(audio_buffer->count>=buf_sz)
    {
        printf("The queue is full\n");
        printf("You cannot add items\n");
        return(-1);
    }
    else
    {    	    	    	
    	if(audio_buffer->hole[seqNum] == 0)
    	{
    		//printf("n\n");
	        strncpy(audio_buffer->value+audio_buffer->rear,theItemValue,payload);  
	        audio_buffer->count += payload;	        
	        audio_buffer->rear = (audio_buffer->rear+payload)%buf_sz;
	    }
	    else 
	    {
			printf("hole marSeqNum: %d\n",marSeqNum);
	    	//printf("inside hole =1!\n");
	    	strncpy(audio_buffer->value+marSeqNum,theItemValue,payload);  
	        audio_buffer->count += payload;	        		        
	    }		
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
    	write(fd,&logtext,strlen(logtext));		
		close(fd);	
		exit(1);
    }
    	
    //printf("front: %d, rear: %d\n", audio_buffer->front, audio_buffer->rear);
    return aux;
}

void handle_sigpoll (int status)
{	
	test++;
	//printf("inside sigpoll\n");	
	int s_len = sizeof(s_udp_addport);
	char filereader[payload+1];
	bzero(filereader,payload+1);
	int bytes = recvfrom(udp_sockd, filereader, sizeof(filereader), 0, (struct sockaddr *) &s_udp_addport, &s_len);
	//printf("\n\nstring received: %s, %zu\n",filereader,strlen(filereader));		

	//printf("2!\n");
	if (strcmp(filereader,"THE_END")==0) 
	{
		printf("reached end\n");
		write(fd,&logtext,strlen(logtext));		
		close(fd);	
		exit(1);	
	}

	/*if(test == 180)
		exit(1);*/
	s_udp_addport.sin_port = htons(atoi(server_udp_port));		
	
	char seqNow[10];
	bzero(seqNow,10);
	strncpy(seqNow,filereader,5);
	seqNow[strlen(seqNow)] = '\0';
	//printf("seqNow: %s\n",seqNow);
	int seqNum = atoi(seqNow);
	int marSeqNum = seqNum-10000;
	marSeqNum*=payload;
    marSeqNum %= buf_sz;

	char availableSeq[6];  
	strncpy(availableSeq,audio_buffer.value+(audio_buffer.front),5);  	
	int firstSeq = atoi(availableSeq);    
    //printf("seqNumber: %d, firstSeq: %d\n\n",seqNum,firstSeq);
    //printf("\nmarSeqNum: %d\n",marSeqNum);

    if(firstSeq <= seqNum)
    {
    	//printf("seqNow: %s, prevSeq: %d\n",seqNow,prevSeq);
		if(seqNum%drop_pkt==0 && (audio_buffer.hole[seqNum] == 0) && retransmit_flag == 1)
		{
			char resend[30];
			bzero(resend,30);
			sprintf(resend,"M %d",prevSeq+1);		
			resend[strlen(resend)] = '\0';				
			printf("After resend: %s\n",resend);
			int n = sendto(udp_sockd, resend, bytes, 0, (struct sockaddr *) &s_udp_addport, s_len);
			if (n < 0) 
			{
			  perror("Error at Client: sendto()!\n");
			  exit(1);
			}
			
			audio_buffer.hole[seqNum] = 1;
			audio_buffer.rear = (audio_buffer.rear+payload)%buf_sz;
		}

		else
		{				
			pthread_mutex_lock(&mutex);
			putItem(&audio_buffer, filereader, bytes, seqNum,marSeqNum);
			pthread_mutex_unlock(&mutex);	

			gettimeofday(&t2, NULL);
			Q_t = getCount(&audio_buffer);
			
			elapsedTime = ((t2.tv_sec * 1000000 + t2.tv_usec) - (t1.tv_sec * 1000000 + t1.tv_usec));
			char logpkt[30];
			bzero(logpkt,30);
			sprintf(logpkt,"%d:%f\n",(int)Q_t,elapsedTime);		
			strncpy(logtext+cnt,logpkt,strlen(logpkt));
			cnt += strlen(logpkt);   
					
			char feedback[50];	
			//printf("After PUT count: %d\n", Q_t);
			sprintf(feedback,"%s %d %d %d","Q",Q_t,target_buf,mu);	
			printf("After Put: %s\n",feedback);
			
			int n = sendto(udp_sockd, feedback, bytes, 0, (struct sockaddr *) &s_udp_addport, s_len);
			if (n < 0) 
			{
			  perror("Error at Client: sendto()!\n");
			  exit(1);
			}
		}
		// set prevSeq to current Seq number
		prevSeq = seqNum;	
	}
	
	
}


int main(int argc, char* argv[])
{
	if(argc<13)
	{
		printf("Usage: audiolisten server-ip server-tcp-port client-udp-port payload-size playback-del gamm buf-sz target-buf logfile-c path_filename K retransmit_flag\n");
		exit(1);
	}

	char filename[50];
	payload = atoi(argv[4])+5;
	playback_del = atof(argv[5]);
	playback_del =  playback_del/1000;
	gamm = atoi(argv[6]);
	mu = (1000000/gamm);
	target_buf = atoi(argv[8]);
	buf_sz = atoi(argv[7]);		
	drop_pkt = atoi(argv[11]);
	retransmit_flag = atoi(argv[12]);
	fractional = modf(playback_del, &integral);
	
	sprintf(filename,"%s",argv[9]);
	filename[strlen(filename)]='\0';
	printf("filename: %s, %zu\n",filename,strlen(filename));
	
	fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

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
	printf("server_udp_port: %s\n",server_udp_port);	
	
	if (strcmp(msg,"KO")==0) 
	{		
		printf("File not found!\n");		
	}
	

		
	if (strcmp(msg,"OK")==0) 
	{						
		
		initializeQueue(&audio_buffer);
		gettimeofday(&t1, NULL);			
		//printf("%d\n", (int)integral);
    	//printf("%d\n", (int)(fractional*1000));
		tim.tv_sec  = (int)integral;
		tim.tv_nsec = (fractional*1000000000);			
		while(nanosleep(&tim , &tim) && errno == EINTR)
		{
			
		}
		//gettimeofday(&t2, NULL);
		//elapsedTime = ((t2.tv_sec * 1000000 + t2.tv_usec) - (t1.tv_sec * 1000000 + t1.tv_usec));
		//printf("Completion Time(seconds): %f\n",((double)elapsedTime/1000000));	
		//exit(1);
				
		//sleep(2);
		//gettimeofday(&t1, NULL);			
		fp = open("/dev/audio", O_WRONLY, 0);
		
		int totPkts = 0;
		while(1)		
		{ 									
			char readValue2[buf_sz];
			pthread_mutex_lock(&mutex);
			getItem(&audio_buffer,&readValue2[0],payload);
			pthread_mutex_unlock(&mutex);
			write(fp,&readValue2,payload-5);		
			printf("After GET count: %d\n", getCount(&audio_buffer));	
			printf("After GET readValue2: %s\n", readValue2);
			bzero(readValue2,buf_sz);	

			tim2.tv_sec  = 0;
			tim2.tv_nsec = 30000000;
			while(nanosleep(&tim2 , &tim2) && errno == EINTR)
			{
				
			}	
			/*totPkts += 1;
			if(totPkts == 10)
				break; */
		}
		
		write(fd,&logtext,strlen(logtext));		
		close(fd);
		
		close(udp_sockd);	
		
	}
	
	return 0;
}
