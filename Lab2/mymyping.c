#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <signal.h>
#include <sys/time.h>

void random_string(char *str, const int length) {
    static const char alpha_num[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

	int i=0;
    for (i = 0; i < length; ++i) {
        str[i] = alpha_num[rand() % (sizeof(alpha_num) - 1)];
    }

    str[length] = 0;
}

void timer_handler(int signal)
{
	printf("no response from ping server\n");
	exit(1);
}

int main(int argc, char **argv)
{
	WORD wVersionRequested = MAKEWORD(1,1);
	WSADATA wsaData;
    int sock_id, port_no;
	int n,s_len;

    struct sockaddr_in s_addport,c_addport;
    struct hostent *server;
    char *hostname_ip;
    char buf[1000];
	char secretkey[40];
	int sk_len;
	int c_len = sizeof(c_addport);
	struct timeval t1, t2;
    double elapsedTime;

	if(argc!=4)
	{
		printf("Usage: %s ip_address portnumber secretkey \n",argv[0]);
		exit(1);
	}

	WSAStartup(wVersionRequested, &wsaData);
	sprintf(secretkey,"%s",argv[3]);
	sk_len = strlen(secretkey);
	if(sk_len < 10 || sk_len > 20)
	{
		printf("Secret Key's length should be atleast 10 and not more than 20!\n");
		exit(1);
	}

	//struct sigaction act;
    //act.sa_handler = signal_handler;
    //sigaction (SIGALRM, & act, 0);

	secretkey[sk_len]='\0';
	int pad_len = 1000 - (sk_len + 2);
	char pad[pad_len];
	random_string(pad,pad_len);

	memset((char *) &buf, 0, sizeof(buf));
	sprintf(buf,"$%s$%s",secretkey,pad);
	printf("Message sent from client: %s\nMessage Length(bytes): %d\n\n",buf,strlen(buf));

    hostname_ip = argv[1];
    port_no = atoi(argv[2]);

    sock_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_id < 0)
	{
		perror("Error opening socket at Client!\n");
		exit(1);
	}

    // resolve server name
    server = gethostbyname(hostname_ip);
    if (server == NULL) {
        printf("Unable to resolve host: %s\n", hostname_ip);
        exit(0);
    }

    memset((char *) &s_addport, 0, sizeof(s_addport));
	memset((char *) &c_addport, 0, sizeof(c_addport));
    s_addport.sin_family = AF_INET;
    //bcopy((char *)server->h_addr,(char *)&s_addport.sin_addr.s_addr, server->h_length);
    s_addport.sin_addr.s_addr = inet_addr((char *)server->h_addr);
    s_addport.sin_port = htons(port_no);

    s_len = sizeof(s_addport);

	//gettimeofday(&t1, NULL);
    n = sendto(sock_id, buf, strlen(buf), 0, (struct sockaddr *) &s_addport, s_len);
    if (n < 0)
	{
      perror("Error at Client: sendto()!\n");
	  exit(1);
	}

	char msg[20];
    n = recvfrom(sock_id, msg, strlen(msg), 0, (struct sockaddr *) &c_addport, &c_len);
	//gettimeofday(&t2, NULL);

	//printf("Server IP: %s, Server Port_Number: %d\n\n",inet_ntoa(c_addport.sin_addr), ntohs(c_addport.sin_port));

    if (n < 0)
	{
      perror("Error at Client: recvfrom()!\n");
	  exit(1);
	}
	//elapsedTime = (t2.tv_usec - t1.tv_usec)/1000.0;
	//printf("Elapsed Time(milli-seconds): %f\n\n",elapsedTime);
    printf("Ping from Server: %s\n\n", msg);

	closesocket(sock_id);
    WSACleanup();
    return 0;
}
