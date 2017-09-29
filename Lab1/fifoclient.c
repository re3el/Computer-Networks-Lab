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
	int pid = getpid();									// extract client_pid
	
	// creating client pipe(cfifoPID)
	char client_fifo[50];	
	sprintf(client_fifo,"cfifo%d",pid);	
	if(mkfifo(client_fifo,0666)<0)
	{
		printf("Unable to create a fifo!\n");
		exit(-1);
	}	
	
	// opening cmdfifo pipe to send client_info 
	char *server_fifo = "cmdfifo";
	int fp = open(server_fifo,O_RDWR);
	if(fp < 0) 
	{
		printf("Error in opening server fifo_file\n");
		exit(-1);
	}
	
	// clientinfo: $PID$COMMAND
	char clientinfo[20];								// stores client_info which gets transferred through client_pipe
	sprintf(clientinfo,"$%d$%s",pid,COMMAND);	
	printf("Message sent from client: %s\n",clientinfo);
	
	// write client_info to the cmdfifo pipe
	if(write(fp,clientinfo,sizeof(clientinfo))<0)
	{
		perror("Write Error\n"); 						//print error for write
		exit(-1);
	}
	
	// opening the respective client_fifo pipe
	char buf[10000]; 
	int fpc = open(client_fifo,O_RDONLY);
	if(fpc < 0) 
	{
		printf("Error in opening client_fifo pipe!\n");
		exit(-1);
	}
	
	// read from the respective client_fifo pipe
	if(read(fpc,buf,sizeof(buf))<0)
	{
		perror("Read Error\n"); 						//print error for read
		exit(-1);
	}
	
	printf("Reply from server:\n%s",buf);	
	
	// close the respective client_fifo pipe
	close(fp);
	close(fpc);
	unlink(client_fifo);
	return 0;
}
