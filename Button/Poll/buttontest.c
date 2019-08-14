#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>


int main(int argc, char * argv [ ])
{
    int fd,ret;

    unsigned char key_vals;
    struct pollfd fds[1];

	fd = open("/dev/buttons", O_RDWR); //��/dev/buttons����

	if(fd < 0)
	{
      printf("can't open \"/dev/buttons\" \r\n ");

	}

    fds[0].fd = fd;
	fds[0].events = POLLIN;//POLLIN-���¼����ڿ��Զ�ȡ��������ʱ poll�����᷵�� 

	while(1)
    {
       //�������������ڵ�poll����--�������ʱʱ���ڷ�����ָ���¼����ط�0ֵ
   	   ret = poll(fds,1,5000);//   1-���һ���¼� 5000-��ʱ5000ms

	   if (ret == 0)
	   {
			printf("time out\n");
	   }
       else
       {
         read(fd, &key_vals, 1);
		 printf("key_val = 0x%x\n", key_val);
	   
	   }


	}



}

