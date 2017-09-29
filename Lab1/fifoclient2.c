#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#define COMMAND "pwd"
int main()
{
	int pid = getpid();
	char client_fifo[50];
	sprintf(client_fifo,"cfifo%d",pid);
	//char *client_fifo = "cfifo"+pid;
	mkfifo(client_fifo,0666);			

	char *server_fifo = "cmdfifo";
	int fp = open(server_fifo,O_RDWR);
	
	char clientinfo[20];
	sprintf(clientinfo,"$%d$%s",pid,COMMAND);	
	puts(clientinfo);
	
	write(fp,clientinfo,sizeof(clientinfo));
	
	char buf[10000]; 
	int fpc = open(client_fifo,O_RDONLY);
	read(fpc,buf,sizeof(buf));
	puts(buf);
	
	unlink(client_fifo);
	return 0;
}
