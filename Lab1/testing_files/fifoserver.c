#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
//TODO: Null terminate the buffers

void handler(int signal) {  
  
  // WNOHANG: return 0 if the child process isn't terminated and child process pid if terminated. It's a non blocking call from the parent to reap zombies
  // while loop used to reap more than 1 zombie that may exist
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
  
}

int main()
{
	pid_t k;       // signed integer for representing process_id
	char buf[100];  // buffer array to store input
	int status;    // for storing the status information of the child process
	int len;	   // buffer length		
		
	// create cmdfifo pipe
	// TODO:error
	char *server_fifo = "cmdfifo";	
	if(mkfifo(server_fifo,0666)<0)
	{
		printf("Unable to create a fifo\n");
		exit(-1);
	}
	
	int fps;
	fps = open(server_fifo,O_RDWR);
	if(fps < 0) 
	{
		printf("Error in opening file");
		exit(-1);
	}

	while(1)
	{
		int saved_stdout;
		saved_stdout = dup(1);
		// read info sent by client from server_fifo pipe		
		if(read(fps,buf,sizeof(buf))<0)
		{
			perror("Read Error"); //print error for read
			exit(-1);
		}
		puts(buf);		
		
		// store info in two separate buffers: cpid for clientPID and comm for command requested
		char cpid[50];
		char comm[50];
		int i = 1;
		while(buf[i]!='$')
			i++;
		
		strncpy(cpid,&buf[1],i-1);	
		cpid[strlen(cpid)]='\0';
		//printf("%s\n",cpid);

		strncpy(comm,&buf[i+1],strlen(buf)-i);	
		comm[strlen(comm)]='\0';
		//printf("%s\n",comm);
		
		// modifying pipe name to match the respective client 
		int npid = atoi(cpid);	
		char client_fifo[50];
		sprintf(client_fifo,"cfifo%d",npid);

		
		struct sigaction sa;
		sa.sa_handler = &handler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
		if (sigaction(SIGCHLD, &sa, 0) == -1) {
			perror(0);
			printf("chucked\n");
			exit(1);
		} 
			
		
		// connecting to the respective client_fifo pipe for running the requested command through dup2()
		int fpc;
		fpc = open(client_fifo,O_RDWR);
		if(fpc < 0) {
			printf("Error in opening file");
			exit(-1);
		} 
		dup2(fpc,1);
		close(fpc);
		
		
		k = fork();	
		
		if (k==0) {								// k = 0 represents that it is inside child process 
			// child code	
			if(execlp(comm,comm,NULL) == -1)       // if execution failed, terminate child
            {    
				printf("Child Execution Failed\n");
				exit(1); 
			} 
			//execlp(buf,buf,NULL);
        }
        
		//execlp(comm,comm,NULL);
		memset(buf, 0, sizeof(buf));
		
		// Restore stdout
		dup2(saved_stdout, 1);
		close(saved_stdout);
	}
	
	
	// unlinking the server_fifo pipe
	close(fps);
	unlink(server_fifo);
	

	return 0;
}
