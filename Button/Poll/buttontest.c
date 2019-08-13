#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>


int main(int argc, char * argv [ ])
{
    int fd;

    unsigned char key_vals;

	fd = open("/dev/buttons", O_RDWR); //打开/dev/buttons驱动

	if(fd < 0)
	{
      printf("can't open \"/dev/buttons\" \r\n ");

	}


	while(1)
    {
        read(fd, &key_vals, 1); //执行驱动的读函数

        
		printf("keyval = 0x%x\r\n",key_vals );
		


	}



}

