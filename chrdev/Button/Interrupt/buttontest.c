#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>


int main(int argc, char * argv [ ])
{
    int fd;

    unsigned char key_vals;

	fd = open("/dev/buttons", O_RDWR); //��/dev/buttons����

	if(fd < 0)
	{
      printf("can't open \"/dev/buttons\" \r\n ");

	}


	while(1)
    {
        read(fd, &key_vals, 1); //ִ�������Ķ�����

        
		printf("keyval = 0x%x\r\n",key_vals );
		


	}



}

