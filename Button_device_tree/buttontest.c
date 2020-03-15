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

	

	fd = open("/dev/buttons", O_RDWR); //��/dev/buttons����
	if(fd < 0)
	{
      printf("can't open \"/dev/buttons\" \r\n ");

	}


	signal(SIGIO, my_signal_fun); //��ʼ���źŴ����� ָ���Զ���

    
    
    fcntl(fd, F_SETOWN, getpid()); //��ȡPID �����ں� �ź�Ӧ�÷���˭
	
	Oflags = fcntl(fd, F_GETFL); 
	
	// �ı�fasync��ǣ����ջ���õ�������faync > fasync_helper����ʼ��/�ͷ�fasync_struct
	fcntl(fd, F_SETFL, Oflags | FASYNC);


	while(1)
    {
      sleep(5000); //ִ������
	}

	return 0;
}

