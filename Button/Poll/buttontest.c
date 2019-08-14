#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>


int main(int argc, char * argv [ ])
{
    int fd,ret;

    unsigned char key_vals;
    struct pollfd fds[1];

	fd = open("/dev/buttons", O_RDWR); //打开/dev/buttons驱动

	if(fd < 0)
	{
      printf("can't open \"/dev/buttons\" \r\n ");

	}

    fds[0].fd = fd;
	fds[0].events = POLLIN;//POLLIN-当事件处于可以读取并不阻塞时 poll函数会返回 

	while(1)
    {
       //调用驱动程序内的poll函数--如果在延时时间内发生了指定事件返回非0值
   	   ret = poll(fds,1,5000);//   1-监控一个事件 5000-延时5000ms

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

