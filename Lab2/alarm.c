#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/time.h>
#include <signal.h>

static void timer_handler(int sig)
{
  //signal(SIGALRM, SIG_IGN); /* ignore this signal */
  printf("timer_handler\n");
  //signal(SIGALRM, timer_handler); /* reinstall the handler */
  exit(1);
}

int main()
{
  printf("test_timer\n");

  signal(SIGALRM, timer_handler);
  ualarm(500000,0); 
	sleep(3);
  return 0;
}