#include<stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/signal.h>
#define MAX_ITEMS  20
typedef struct circ_que_struct
{
    int front;
    int rear;
    int count;
    char value[MAX_ITEMS];
} circ_que;

void initializeQueue(circ_que *audio_buffer)
{
    int i;
    audio_buffer->count =  0;
    audio_buffer->front =  0;
    audio_buffer->rear  =  0;
    bzero(audio_buffer->value,MAX_ITEMS);
    return;
}

int isEmpty(circ_que *audio_buffer)
{
    if(audio_buffer->count==0)
        return(1);
    else
        return(0);
}

int putItem(circ_que *audio_buffer, char theItemValue[], int payload)
{
    if(audio_buffer->count>=MAX_ITEMS)
    {
        printf("The queue is full\n");
        printf("You cannot add items\n");
        return(-1);
    }
    else
    {
        strncpy(audio_buffer->value+audio_buffer->front,theItemValue,payload);  
        audio_buffer->count += payload;
        //audio_buffer->value[audio_buffer->rear] = theItemValue;
        audio_buffer->rear = (audio_buffer->rear+payload)%MAX_ITEMS;
    }
}

int getItem(circ_que *audio_buffer, char *theItemValue, int payload)
{
    if(isEmpty(audio_buffer))
    {
        printf("done playing\n");
        return(-1);
    }
    else
    {
        strncpy(theItemValue,&audio_buffer->value[audio_buffer->front],payload);  
        //*theItemValue=audio_buffer->value[audio_buffer->front];
        audio_buffer->front=(audio_buffer->front+payload)%MAX_ITEMS;
        audio_buffer->count -= payload;
        return(0);
    }
}

void printQueue(circ_que *audio_buffer)
{
    int aux, aux1;
    aux  = audio_buffer->front;
    aux1 = audio_buffer->count;
    while(aux1>0)
    {
        printf("Element #%d = %c\n", aux, audio_buffer->value[aux]);
        aux=(aux+1)%MAX_ITEMS;
        aux1--;
    }

    printf("string: %s\n",audio_buffer->value);
    return;
}

int getCount(circ_que *audio_buffer)
{
    int aux;   
    aux = audio_buffer->count;   
    printf("front: %d, rear: %d\n", audio_buffer->front, audio_buffer->rear);
    return aux;
}

int main(void)
{
    int i= 1;
    char readValue[5];
	circ_que myQueue;
    int payload = 4;
    char input[] ="aaaa";

    initializeQueue(&myQueue);
/*    for(i=0; i<MAX_ITEMS+1; i++)
    {
        putItem(&myQueue, 'a',payload);
    }*/
    putItem(&myQueue,input,payload);
    printQueue(&myQueue);
	
	int temp = getCount(&myQueue);
    printf("\n\nAfter Put count: %d\n\n",temp);    
    /*for(i=0; i<MAX_ITEMS/2; i++)
    {
        getItem(&myQueue, &readValue);
        printf("readValue = %c\n", readValue);
    }*/
    
    getItem(&myQueue, &readValue[0], payload);
    printf("readValue = %s\n", readValue);
    printQueue(&myQueue);
    //printf("Element #%d = %c\n", aux, audio_buffer->value[aux]);
	
	temp = getCount(&myQueue);
    printf("\n\nAfter Get, count: %d\n\n",getCount(&myQueue));
	
	/*puts("\n");
	for(i=0; i<MAX_ITEMS/2; i++)
    {
        putItem(&myQueue, 'a');
    }*/
    char input2[] ="bbbb";    
    putItem(&myQueue,input2,payload);
	printQueue(&myQueue);	
	
	temp = getCount(&myQueue);
    printf("\n\nAfter put count: %d\n\n",temp);
	
    exit(EXIT_SUCCESS);  
	
	/* char str[20];
	sprintf(str,"%s","directory");	 */
	

	
	return 0;
} 


//	char filereader[100001];
	//char *audio_file = "/dev/audio";
	//int fp,player;
	//player = open("/dev/audio", O_WRONLY, 0);
    //if (player < 0) 
/*    {
     perror("Opening /dev/audio failed\n");
     exit(1);
    }

	
	int fp2 = open("/u/data/u3/park/pub/kline-jarrett.au",O_RDONLY);
	//int fp2 = open("anderson.au",O_RDONLY);
	read(fp2,filereader,100000);
	sleep(2);

	while(1)
	{
		//printf("readValue = %d\n", readValue);
		write(player,filereader,100000);
		//bzero(filereader,100000);
	}
*/

//./audiolisten 128.10.25.101 10025 20026 250 2 30 40000 20000 logfile-c /u/data/u3/park/pub/kline-jarrett.au
