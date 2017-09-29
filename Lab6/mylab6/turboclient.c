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

int end_flag = 0;
int totPkts = 0;
int totBytes = 0;
int buf_sz;
int blocksize;
int fps;

struct timespec tim, tim2;

// setup a socket at client		
int sock_id,port_no,n;
struct sockaddr_in s_addport;	
struct hostent *server;

struct timeval t1, t2;
double elapsedTime;

int retransmit_flag;
int drop_pkt;
int prevSeq = 9999;

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
        printf("\n\n\n\nThe queue is full\n");
        printf("You cannot add items\n\n\n\n\n");
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
    	//write(fd,&logtext,strlen(logtext));		
		//close(fd);	
		gettimeofday(&t2, NULL);	
		close(sock_id);	
		close(fps);		

		printf("Total bytes read in getCount: %d\n",totBytes);	
		elapsedTime = ((t2.tv_sec * 1000000 + t2.tv_usec) - (t1.tv_sec * 1000000 + t1.tv_usec));
		printf("Completion Time(micro-seconds): %f\n",elapsedTime);
		printf("Throughput(Mbytes/sec): %f\n",(double)(totBytes/elapsedTime)*8);
		
		printf("In getcount exit!\n");
		exit(1);
    }
    	
    //printf("front: %d, rear: %d\n", audio_buffer->front, audio_buffer->rear);
    return aux;
}

void handle_alarm()
{
	gettimeofday(&t2, NULL);	
	close(sock_id);	
	close(fps);		

	printf("Total bytes read in getCount: %d\n",totBytes);	
	elapsedTime = (((t2.tv_sec-1) * 1000000 + t2.tv_usec) - (t1.tv_sec * 1000000 + t1.tv_usec));
	printf("Completion Time(micro-seconds): %f\n",elapsedTime);
	printf("Throughput(Mbytes/sec): %f\n",(double)(totBytes/elapsedTime)*8);
	
	printf("In getcount2 exit!\n");
	exit(1);
}

int count = 0;
int lastRead = 99999999;
int lastReadBytes = 0;
int prevRead = 0;
size_t nread = 0;

void handle_sigpoll (int status)
{		
	char filereader[blocksize+1];
	bzero(filereader,blocksize+1);
	int s_len = sizeof(s_addport);
	// read in units of blocksize and write to new_file 
	count++;
	prevRead = nread;
	nread = recvfrom(sock_id, filereader, blocksize, 0, (struct sockaddr *) &s_addport, &s_len);				
	//printf("string received: %s, totPkts: %d\n",filereader,count);
	if(nread==3 && end_flag==0)
	{					
		end_flag = 1;
		printf("\n\n\n\nREACHED END OF FILE: %d!!\n\n\n\n",prevRead);
		lastRead = count;
		lastReadBytes = prevRead;
	}
		
	if(end_flag!=1)
	{
/* 		putItem(&audio_buffer, filereader, nread);
		//write(fps,filereader,nread);			
		bzero(filereader,nread);	
		totPkts++;
		totBytes+=nread;	
		printf("After PUT count: %d\n", getCount(&audio_buffer)); */
	
		
		char seqNow[10];
		bzero(seqNow,10);
		strncpy(seqNow,filereader,5);
		seqNow[strlen(seqNow)] = '\0';
		//printf("seqNow: %s\n",seqNow);
		int seqNum = atoi(seqNow);
		int marSeqNum = seqNum-10000;
		marSeqNum*=blocksize;
		marSeqNum %= buf_sz;

		char availableSeq[6];  
		strncpy(availableSeq,audio_buffer.value+(audio_buffer.front),5);  	
		int firstSeq = atoi(availableSeq);    
		//printf("seqNumber: %d, firstSeq: %d\n\n",seqNum,firstSeq);
		//printf("\nmarSeqNum: %d\n",marSeqNum);

		if(firstSeq <= seqNum)
		{
			//printf("seqNow: %s, prevSeq: %d\n",seqNow,prevSeq);
			if(seqNum%drop_pkt==0 && (audio_buffer.hole[seqNum] == 0) && 0)
			{
				char resend[30];
				bzero(resend,30);
				sprintf(resend,"M %d",prevSeq+1);		
				resend[strlen(resend)] = '\0';				
				printf("After resend: %s\n",resend);
				int n = sendto(sock_id, resend, nread, 0, (struct sockaddr *) &s_addport, s_len);
				if (n < 0) 
				{
				  perror("Error at Client: sendto()!\n");
				  exit(1);
				}
				
				audio_buffer.hole[seqNum] = 1;
				audio_buffer.rear = (audio_buffer.rear+blocksize)%buf_sz;
			}

			else
			{		
				
				putItem(&audio_buffer, filereader, nread, seqNum,marSeqNum);	
				bzero(filereader,nread);	
				totPkts++;
				totBytes+=nread;	
				printf("After PUT count: %d\n", getCount(&audio_buffer));				
			}
			// set prevSeq to current Seq number
			prevSeq = seqNum;	
		}
		
	}
	
}



int main(int argc, char* argv[])
{	
	// check arguments
	if(argc!=7)
	{
		printf("Usage: ./fileclient hostname portnumber secretkey filename configfile.dat lossnum\n",argv[0]);
		exit(1);
	}
	
	// buffer to store temp values
	char buf[1000];
	buf_sz = 40000;
	// variables needed for gettimeofday() calculations
	drop_pkt = atoi(argv[6]);
	
	// Get blocksize from configfile.dat for read() and write()
	int fpc;
	char configFile[50];	
	sprintf(configFile,"%s",argv[5]);
	fpc = open(configFile,O_RDWR);
	if(fpc < 0) 
	{
		printf("Error opening configfile.dat at Client\n");
		exit(-1);
	}
	
	// read from configfile.dat and store the value into 'blocksize' variable
	if(read(fpc,buf,sizeof(buf))<0)
	{
		perror("Read Error\n"); 						//print error for read
		exit(-1);
	}
	blocksize = atoi(buf) + 5;	
	printf("blocksize: %d\n",blocksize);
	close(fpc);
	bzero(buf,1000);	
	
	// check filename length(should be less than 16) 
	char filename[20];	
	sprintf(filename,"%s",argv[4]);
	if(strlen(filename)>16)
	{
		printf("Filename length should not be more than 16 characters!\n");
		exit(1);
	}
	filename[strlen(filename)]='\0';

	// filename check constraints: cannot contain spaces and '\'characters!
	int len = 0;
	while(len!=strlen(filename))
	{
		if(filename[len]==' ' || filename[len]=='/' )
		{
			printf("filename cannot contain spaces or '/' characters!\n");
			exit(1);
		}
		len++;
	}	
	
	// check secretkey length
	char secretkey[40];	
	sprintf(secretkey,"%s",argv[3]);
	if(strlen(secretkey)<10 || strlen(secretkey)>20)
	{
		printf("Secret Key's length should be atleast 10 and not more than 20!\n");
		exit(1);
	}
	secretkey[strlen(secretkey)]='\0';
	
	// clientinfo: $secretkey$filename --> to be sent to server 
	char clientinfo[40];								
	sprintf(clientinfo,"$%s$%s",secretkey,filename);	
	printf("Message sent from client: %s\n",clientinfo);

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

	
	initializeQueue(&audio_buffer);
	
	// connect to the server socket
	port_no = atoi(argv[2]);
	sock_id = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock_id < 0)
		perror("Error opening Socket!\n");	
	
	fcntl(sock_id,F_SETOWN,getpid());           
	fcntl(sock_id, F_SETFL, FASYNC); 	

	int option_val = 1;
	setsockopt(sock_id, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));

	// hostname resolution
	server = gethostbyname(argv[1]);
	if(server == NULL)
	{
		fprintf(stderr,"Unable to resolve host!\n");
		exit(1);
	}
	bzero((char *) &s_addport, sizeof(s_addport));
	s_addport.sin_family = AF_INET;
	s_addport.sin_port = htons(port_no);
	bcopy((char *)server->h_addr, 
         (char *)&s_addport.sin_addr.s_addr,
         server->h_length);
		 
	// requesting the file info: write clientinfo to server_socket	
	n = sendto(sock_id, clientinfo, strlen(clientinfo), 0, (struct sockaddr *) &s_addport, sizeof(s_addport));		
    if (n < 0) 
		perror("Error at client write!\n");
	
	
		
	// read from server_socket
    bzero(buf,1000);	
	int totPktsWr = 0;
	
/* 	tim2.tv_sec  = 0;
	tim2.tv_nsec = 100;
	while(nanosleep(&tim2 , &tim2) && errno == EINTR)
	{
		
	} */
	
	// check if the filename already exisits
	fps = open(filename, O_CREAT | O_EXCL | O_RDWR, 0644);
	if (fps!= -1)
	{			
		gettimeofday(&t1, NULL);			
		while(1)		
		{ 	
			
			char readValue2[blocksize+1];
			//pthread_mutex_lock(&mutex);
			//printf("Before Get Count: ");
			int getC = getCount(&audio_buffer);
			if(getC > 0)
			{				
				totPktsWr++;			
				if(totPktsWr == lastRead)
				{
					getItem(&audio_buffer,&readValue2[0],blocksize);			
					printf("After GET readValue2: %s\n", readValue2);
				
					write(fps,&readValue2,getC-5);					
					printf("\nAfter GET count: %d\n", getCount(&audio_buffer));
					bzero(readValue2,blocksize+1);	
					
					gettimeofday(&t2, NULL);	
					close(sock_id);	
					close(fps);		

					printf("Total bytes read from lastRead: %d\n",totPktsWr);	
					elapsedTime = ((t2.tv_sec * 1000000 + t2.tv_usec) - (t1.tv_sec * 1000000 + t1.tv_usec));
					printf("Completion Time(micro-seconds): %f\n",elapsedTime);
					printf("Throughput(Mbytes/sec): %f\n",(double)(totBytes/elapsedTime)*8);
					exit(1);
				}
				
				getItem(&audio_buffer,&readValue2[0],blocksize);			
				printf("After GET readValue2: %s, totPktsWr:%d\n", readValue2,totPktsWr);
			
				write(fps,&readValue2,blocksize-5);					
				printf("\nAfter GET count: %d\n", getCount(&audio_buffer));
				bzero(readValue2,blocksize+1);	
				
				/* tim.tv_sec  = 0;
				tim.tv_nsec = 50;
				while(nanosleep(&tim , &tim) && errno == EINTR)
				{
					
				}  */ 
				alarm(1);
			}
			
			/* tim.tv_sec  = 0;
			tim.tv_nsec = 1;
			while(nanosleep(&tim , &tim) && errno == EINTR)
			{
				
			}    
			if(totPktsWr == 1000000)
				break; */
				
		}
		
		
		 
		gettimeofday(&t2, NULL);	
		close(sock_id);	
		close(fps);				
	}
	else
	{
		printf("Filename already exists!\n");
		exit(1);
	}	
	
	// if no data got transferred from server
	if(totBytes==0)
	{
		printf("No data transferred from Server!\n");
		exit(1);
	}
	
	printf("Total pkts read: %d\n",totPktsWr);	
	printf("Total bytes read in while: %d\n",totBytes);	
	//elapsedTime = ((t2.tv_sec * 1000000 + t2.tv_usec) - (t1.tv_sec * 1000000 + t1.tv_usec));
	//printf("Completion Time(micro-seconds): %f\n",elapsedTime);
    //printf("Throughput(Mbytes/sec): %f\n",(double)(totBytes/elapsedTime)*8);
    
	
	return 0;
}
