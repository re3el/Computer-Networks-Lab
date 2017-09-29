#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
 
int main(void)
{
    const char *ip = "128.10.25.102";
    struct sockaddr_in s1_addport;
    printf("ip: %s, strlen:%zu\n",ip,strlen(ip));
    inet_aton(ip,&s1_addport.sin_addr);
    printf("Server IP: %s\n",inet_ntoa(s1_addport.sin_addr));
    return 0;
}