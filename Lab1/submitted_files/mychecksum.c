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
	uint64_t be_chksum,le_chksum,chksum = 0;
	fp2 = open(argv[2],O_RDWR);

	if((fp1 = open(argv[1],O_RDWR))>= 0)
	{
		char r;
		while(read(fp1,&r,1)!=0)
        {
            //putchar(r);
            chksum+=r;
            write(fp2,&r,1);
			//printf("init: %" PRIu64 "\n",chksum);
        }
        printf("checksum: %" PRIx64 "\n",chksum);
        be_chksum = bswap_64(chksum);
		//printf("%" PRIu64 "\n",be_chksum);
		//le_chksum = bswap_64(be_chksum);
		//printf("%" PRIu64 "\n",le_chksum);
		write(fp2,&be_chksum,8);
	}

	return 0;
}
