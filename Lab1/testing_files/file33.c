#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#define COMMAND "ls"
int main()
{
	int pid = getpid();
	char client_fifo[50];
	sprintf(client_fifo,"cfifo%d",pid);
	//char *client_fifo = "cfifo"+pid;
	if(mkfifo(client_fifo,0666)<0)
	{
		printf("Unable to create a fifo\n");
		exit(-1);
	}	

	char *server_fifo = "cmdfifo";
	int fp = open(server_fifo,O_RDWR);
	if(fp < 0) 
	{
		printf("Error in opening server fifo_file\n");
		exit(-1);
	}
	
	char clientinfo[20];
	sprintf(clientinfo,"$%d$%s",pid,COMMAND);	
	puts(clientinfo);
	
	if(write(fp,clientinfo,sizeof(clientinfo))<0)
	{
		perror("Write Error\n"); //print error for write
		exit(-1);
	}
	
	char buf[10000]; 
	int fpc = open(client_fifo,O_RDONLY);
	if(fpc < 0) 
	{
		printf("Error in opening client fifo_file\n");
		exit(-1);
	}
	
	if(read(fpc,buf,sizeof(buf))<0)
	{
		perror("Read Error\n"); //print error for read
		exit(-1);
	}
	puts(buf);
	//sleep(1);
	close(fp);
	close(fpc);
	unlink(client_fifo);
	return 0;
}
