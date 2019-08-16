#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>


static int fd;

void my_signal_fun(int signum)
{
   unsigned char key_val;

   read(fd,&key_val,1);
   
   printf("key_val:0x%x\r\n",key_val);
}

int main(int argc, char * argv [ ])
{
    
    unsigned char key_val;
    int Oflags;

	

	fd = open("/dev/buttons", O_RDWR); //打开/dev/buttons驱动
	if(fd < 0)
	{
      printf("can't open \"/dev/buttons\" \r\n ");

	}


	signal(SIGIO, my_signal_fun); //初始化信号处理函数 指向自定义

    
    
    fcntl(fd, F_SETOWN, getpid()); //获取PID 告诉内核 信号应该发给谁
	
	Oflags = fcntl(fd, F_GETFL); 
	
	// 改变fasync标记，最终会调用到驱动的faync > fasync_helper：初始化/释放fasync_struct
	fcntl(fd, F_SETFL, Oflags | FASYNC);


	while(1)
    {
      sleep(5000); //执行休眠
	}

	return 0;
}

