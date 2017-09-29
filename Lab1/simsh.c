// simple shell example using fork() and execlp()

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
pid_t k;       // signed integer for representing process_id
char buf[100]; // buffer array to store input
int status;    // for storing the status information of the child process
int len;	   // buffer length

  while(1) {

        // print prompt
        fprintf(stdout,"[%d]$ ",getpid());

        // read command from stdin
        fgets(buf, 100, stdin);
        len = strlen(buf);
		//printf("len: %d\n",len);
        if(len == 1)                            // only return key pressed
          continue;
        buf[len-1] = '\0';						// null terminate the buffer
	
        k = fork();								// create a child process; on success child process pid is returned in the parent and '0' is returned in the child
        if (k==0) {								// k = 0 represents that it is inside child process 
        // child code	
           if(execlp(buf,buf,NULL) == -1)       // if execution failed, terminate child
                exit(1); 
			//execlp(buf,buf,NULL);
        }
        else {
		// parent code		  
          waitpid(k, &status, 0);               // waits to know the status of the child process whose process_id is k
        }
  }
}



