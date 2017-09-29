#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h> 

int main(int argc, char* argv[])
{	
	// check arguments
	if(argc<6)
	{
		printf("Usage: ./fileclient hostname portnumber secretkey filename configfile.dat\n",argv[0]);
		exit(1);
	}
	
	// buffer to store temp values
	char buf[1000];
	
	// variables needed for gettimeofday() calculations
	struct timeval t1, t2;
    double elapsedTime;
	
	// Get blocksize from configfile.dat for read() and write()
	int fpc,blocksize;
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
	blocksize = atoi(buf);	
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
		
	// setup a socket at client		
	int sock_id,port_no,n;
	struct sockaddr_in s_addport;	
	struct hostent *server;		
	
	// connect to the server socket
	port_no = atoi(argv[2]);
	sock_id = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_id < 0)
		perror("Error opening Socket!\n");	

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
	if (connect(sock_id,(struct sockaddr *) &s_addport,sizeof(s_addport)) < 0) 
        perror("Error at Connect!\n");
		 
	// requesting the file info: write clientinfo to server_socket	
	n = write(sock_id,clientinfo,sizeof(clientinfo));
    if (n < 0) 
		perror("Error at client write!\n");
	
	// shutdown write part of socket as no more write operations are necessary
	shutdown(sock_id,SHUT_WR);
		
	// read from server_socket
    bzero(buf,1000);
	int totBytes = 0;	
	
	// check if the filename already exisits
	int fps = open(filename, O_CREAT | O_EXCL | O_RDWR, 0644);
	if (fps!= -1)
	{				
		size_t nread;		
		char filereader[blocksize+1];
		
		// read in units of blocksize and write to new_file 
		gettimeofday(&t1, NULL);			
		while((nread = read(sock_id,filereader,blocksize))>0)		
		{
			write(fps,filereader,nread);			
			bzero(filereader,blocksize);			
			totBytes+=nread;			
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
	
	printf("Total bytes read: %d\n",totBytes);	
	elapsedTime = ((t2.tv_sec * 1000000 + t2.tv_usec) - (t1.tv_sec * 1000000 + t1.tv_usec));
	printf("Completion Time(micro-seconds): %f\n",elapsedTime);
    printf("Throughput(Mbytes/sec): %f\n",(double)(totBytes/elapsedTime)*8);
    
	
	return 0;
}
