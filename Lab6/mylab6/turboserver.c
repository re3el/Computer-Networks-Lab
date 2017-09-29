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
#include <time.h>
#include <errno.h>

struct timespec tim, tim2;
int blocksize;
int buf_sz = 100;
int sock_id;

struct sockaddr_in p_addport;	

void handler(int signal) 
{    
	// WNOHANG: return 0 if the child process isn't terminated and child process pid if terminated. It's a non blocking call from the parent to reap zombies
	// while loop used to reap more than 1 zombie that may exist
	while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}  
}


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
    audio_buffer->value = malloc(sizeof(char)*blocksize+5);
    bzero(audio_buffer->value,blocksize+5);
    return;
}

int isEmpty(circ_que *audio_buffer)
{
    if(audio_buffer->count==0)
        return(1);
    else
        return(0);
}

int putItem(circ_que *audio_buffer, char theItemValue[], int payload_size)
{
    strncpy(audio_buffer->value+audio_buffer->front,theItemValue,payload_size);  
    audio_buffer->count += payload_size;  
    audio_buffer->rear = (audio_buffer->rear+payload_size)%buf_sz;    
}

int getItem(circ_que *audio_buffer, int diffSeqNumber, char *theItemValue, int payload_size)
{
    if(isEmpty(audio_buffer))
    {
        //printf("done playing\n");
        return(-1);
    }
    else
    {
    	int position = diffSeqNumber*payload_size;
        strncpy(theItemValue,&audio_buffer->value[audio_buffer->front+position],payload_size);          
        audio_buffer->front=(audio_buffer->front+payload_size)%buf_sz;
        audio_buffer->count -= payload_size;
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

/*int getCount(circ_que *audio_buffer)
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
}*/

void handle_sigpoll (int status)
{	

	//printf("inside sigpoll, tcp_done: %d\n",tcp_done);
	char input_buffer[1000];
	bzero((char *) &p_addport, sizeof(p_addport));
	int p_len = sizeof(p_addport);
	bzero(input_buffer,sizeof(input_buffer));
	int bytes = recvfrom(sock_id, input_buffer, sizeof(input_buffer), 0,(struct sockaddr *) &p_addport, &p_len);
	printf("\n\nstring received: %s, %zu\n",input_buffer,strlen(input_buffer));	

	if(input_buffer[0]=='M')
	{
		//printf("inside M\n");
		char feedback[50];		
		char retransmit[blocksize];
		sprintf(feedback,"%s",input_buffer);	
		
		int i = 0;
		char seqNumber[20];
		char *pch;
		pch = strtok (feedback," ");		
		pch = strtok (NULL," ");	
		//strncpy(seqNumber,pch,5);
		sprintf(seqNumber,"%s",pch);
		int seq = atoi(seqNumber);

		//printQueue(&audio_buffer);
		char availableSeq[6];  
    	strncpy(availableSeq,audio_buffer.value+(audio_buffer.front),5);  	
    	int firstSeq = atoi(availableSeq);
    	//printf("firstSeq: %d\n",firstSeq);
    	printf("seqNumber: %d, firstSeq: %d\n\n",seq,firstSeq);
    	
    	if(seq >= firstSeq && seq <= firstSeq+buf_sz)
    	{
    		int diff = seq-firstSeq;
    		getItem(&audio_buffer,diff,&retransmit[0],blocksize+5);
    		//printf("\n\nretransimtted: %s\n\n",retransmit);
    		int n = sendto(sock_id, retransmit, blocksize+5, 0, (struct sockaddr *) &p_addport, p_len);						
			if (n < 0) 
			{
			  perror("Error at Client: sendto()!\n");
			  exit(1);
			}	
    	}
			
	}
	

}



int main(int argc, char *argv[])
{
	pid_t k;       												// signed integer for representing process_id
	char buf[1000];  											// buffer array to store input
	int status;    												// for storing the status information of the child process
	int len,n;	   												// buffer length		
	
	// check the arguments
	if(argc!=5)
	{
		printf("Usage: fileserver portnumber secretkey configfile.dat lossnum \n");
		exit(1);
	}
	
	// get blocksize from configfile.dat for read() and write()
	int fpc;
	char configFile[50];	
	sprintf(configFile,"%s",argv[3]);
	fpc = open(configFile,O_RDWR);
	if(fpc < 0) 
	{
		printf("Error opening configfile.dat at Server\n");
		exit(-1);
	}
	
	// read from configfile.dat and store the value in 'blocksize' variable
	if(read(fpc,buf,sizeof(buf))<0)
	{
		perror("Read Error\n"); 						//print error for read
		exit(-1);
	}
	blocksize = atoi(buf);	
	close(fpc);
	bzero(buf,1000);	
	
	// store secret key into variable
	char secretkey[20];
	sprintf(secretkey,"%s",argv[2]);
	
	// socket variable declaration
	int sock_id_new,port_no,cmp;
	struct sockaddr_in s_addport,c_addport;		
	struct hostent *hostp; 
	bzero((char *) &s_addport, sizeof(s_addport));	
	bzero((char *) &c_addport, sizeof(c_addport));
			
	// Setup UDP Socket
	sock_id = socket(AF_INET, SOCK_DGRAM, 0);					// UDP Datagram
	if(sock_id<0)
		perror("Error opening Socket\n");	
	
	// setsockopt: to rerun the server immediately after killing it
	int option_val = 1;
	setsockopt(sock_id, SOL_SOCKET, SO_REUSEADDR, (const void*)&option_val, sizeof(int));
	
	// binding the socket
	port_no = atoi(argv[1]);
	s_addport.sin_family = AF_INET;
	s_addport.sin_port = htons(port_no);
	s_addport.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock_id, (struct sockaddr *) &s_addport, sizeof(s_addport))== -1) 
	{
		printf("Bind failed!\n");
	}			
	socklen_t szaddr = sizeof(c_addport);

	// infinite loop for server to run continuosly till break point returned
	while(1)
	{		

		// read client_info - $secretkey$filename
		bzero(buf,1000);
		bzero(buf,blocksize+1);		
		n = recvfrom(sock_id, buf, blocksize, 0, (struct sockaddr *) &c_addport, &szaddr);
		printf("Message received from Client: %s\n",buf);
		
		// store client_info in two separate buffers: 'skey' for secret_key and 'filename' for filename requested
		char skey[50];
		char filename[50];
		int i = 1;
		bzero(skey,50);
		bzero(filename,50);
		while(buf[i]!='$')
			i++;		
		strncpy(skey,&buf[1],i-1);	
		skey[strlen(skey)]='\0';		
		puts(skey);
		strncpy(filename,&buf[i+1],strlen(buf)-i);	
		filename[strlen(filename)]='\0';
		puts(filename);				

		// SIGCHLD for creating an asynchronous non blocking call
		struct sigaction sa;
		sa.sa_handler = &handler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
		if (sigaction(SIGCHLD, &sa, 0) == -1) {
			perror(0);			
			exit(1);
		} 		
			
		// check secret_key 	
		if(strcmp(secretkey,skey)!=0)
		{
			printf("Secret Keys don't match!\n");
			exit(1);
		}	
		
		// check for filename in ./filedeposit
		int fp;
		char relPath[50];	
		sprintf(relPath,"filedeposit/%s",filename);					
		fp = open(relPath,O_RDWR);			
		if(fp < 0) 
		{
			printf("Error opening file:%s at Server\n",filename);
			exit(-1);
		}
		
		printf("Client IP: %s, Client_UDP_Port_Number: %d\n\n",inet_ntoa(c_addport.sin_addr), ntohs(c_addport.sin_port));
		// read from filename in units of blocksize and write to socket
		char filereader[blocksize+1];
		int nread;
		int totPkts = 0;
		int totBytes = 0;
		int seqNumber = 10000;
		initializeQueue(&audio_buffer);
		while ((nread = read(fp,filereader,blocksize)) > 0)
		{											
			//totBytes +=n; 
			//usleep(1);
			
			char tempSeq[5];
			bzero(tempSeq,5);
			sprintf(tempSeq,"%d",seqNumber);
			//tempSeq[strlen(tempSeq)] = '\0';
			//printf("tempSeq: %s\n",tempSeq);
			//exit(1);
			
			char outBuffer[blocksize+6];				
			bzero(outBuffer,blocksize+6);				
			sprintf(outBuffer,"%s%s",tempSeq,filereader);
			outBuffer[strlen(outBuffer)] = '\0';
			printf("outBuffer: %s\n",outBuffer);
			
			n = sendto(sock_id, outBuffer, nread+5, 0, (struct sockaddr *) &c_addport, szaddr);					
			if (n < 0) 
			{
			  perror("Error at Client: sendto()!\n");
			  exit(1);
			}
			
			seqNumber++;				
			putItem(&audio_buffer, outBuffer, nread+5);
			bzero(filereader,nread);		
			totPkts += 1;				
			totBytes+=n;
			
			//usleep(1);
			
			tim.tv_sec  = 0;
			tim.tv_nsec = 100;
			while(nanosleep(&tim , &tim) && errno == EINTR)
			{
				
			}  
						
		}
		
		printf("Total packets sent: %d\n",totPkts);	
		printf("Total bytes read: %d\n",totBytes);	
		
		for(i=0;i<3;i++)
		{
			n = sendto(sock_id, "000", 3, 0, (struct sockaddr *) &c_addport, szaddr);
		}

		// close socket,file descriptors
		close(fp);			
		close(sock_id);		
		//exit(1);
	}
		
	// close server_socket
	close(sock_id);	
		
	return 0;
}

 // gcc tempServer.c -o server