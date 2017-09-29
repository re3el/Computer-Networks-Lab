#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define COMMAND "ls"

int main(int argc, char* argv[])
{
	if(argc<4)
	{
		printf("Usage: %s hostname portnumber secretkey\n",argv[0]);
		exit(1);
	}
	
	char buffer[1000];
	int sock_id,port_no,n;
	struct sockaddr_in s_addport;	
	struct hostent *server;
	char secretkey[40];
	char clientinfo[40];								// stores client_info which gets transferred through client_pipe
	sprintf(secretkey,"%s",argv[3]);
	
	// check secret key length
	if(strlen(secretkey)<10 || strlen(secretkey)>20)
	{
		printf("Secret Key's length should be atleast 10 and not more than 20!\n");
		exit(1);
	}
	secretkey[strlen(secretkey)]='\0';
	
	// clientinfo: $secretkey$COMMAND	
	sprintf(clientinfo,"$%s$%s",secretkey,COMMAND);	
	printf("Message sent from client: %s\n",clientinfo);

	// connect to the server socket
	port_no = atoi(argv[2]);
	sock_id = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_id < 0)
		perror("Error opening Socket!\n");
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
		 
	// write to server_socket
	n = write(sock_id,clientinfo,sizeof(clientinfo));
    if (n < 0) 
		perror("Error at client write!\n");
	
	// read from server_socket
    bzero(buffer,1000);
    n = read(sock_id,buffer,sizeof(buffer));
    if (n < 0) 
        perror("Error at client read!\n");
    printf("Reply from Server: %s\n",buffer);
    close(sock_id);
	
	return 0;
}
