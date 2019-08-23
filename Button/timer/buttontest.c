#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>


int main(int argc, char * argv [ ])
{
    int fd;

    unsigned char key_vals;
    unsigned int cnt = 0;

	fd = open("/dev/buttons", O_RDWR ); //�Է�������ʽ ��/dev/buttons���� 

	if(fd < 0)
	{
      printf("can't open \"/dev/buttons\" \r\n ");
	  return -1; 
	}


	while(1)
    {
        read(fd, &key_vals, 1); //ִ�������Ķ�����

        
		printf("keyval = 0x%x  cnt = %d \r\n",key_vals,cnt++ );
		
        
	}

    return 0;

}

