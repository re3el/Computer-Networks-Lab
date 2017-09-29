#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <byteswap.h>
#include <inttypes.h>

int main(int argc, char* argv[])
{
	int fp1, fp2;
	long size,count=0;
	uint64_t be_chksum,le_chksum,old_chksum,chksum = 0;
	fp2 = open(argv[2],O_RDWR);
	
	FILE *fs;
	fs = fopen(argv[1],"r");
	fseek(fs,0,SEEK_END);
	size = ftell(fs);
	//printf("size: %ld\n",size);
	fclose(fs);

	
	if((fp1 = open(argv[1],O_RDWR))>= 0)
	{
		char r;
		while(count<(size-8) && read(fp1,&r,1)!=0 )
        {
            //putchar(r);
            chksum+=r;
            //write(fp2,&r,1);
			count++;
			//printf("init: %" PRIu64 "\n",chksum);
        }
				
        printf("new_chksum: %" PRIx64 "\n",chksum);
		
		uint64_t be_chksum[8];
		while(read(fp1,&be_chksum,8)!=0)
        {
			//printf("uint: %" PRIu64 "\n",*be_chksum);
            //putchar(r);
            //chksum+=r;
            //write(fp2,&r,1);			
			//old_chksum = (uint64_t)buff;
        }
		
        //be_chksum = bswap_64(chksum);
		//printf("%" PRIu64 "\n",be_chksum);
		le_chksum = bswap_64(*be_chksum);
		printf("old_checksum: %" PRIx64 "\n",le_chksum);
		
		if(chksum == le_chksum)
			printf("Checksums match!\n");
		else
			printf("Checksums DONT match!\n");
		//write(fp2,&be_chksum,8);
	}
	

	return 0;
}
