#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>


int main(int argc, char * argv [ ])
{
    int fd;

    unsigned char key_vals[4];
    unsigned int key_val_len = sizeof(key_vals)/sizeof(key_vals[0]);

	fd = open("/dev/buttons", O_RDWR); //打开/dev/buttons驱动

	if(fd < 0)
	{
      printf("can't open \"/dev/buttons\" \r\n ");

	}


	while(1)
    {
        read(fd, key_vals, key_val_len); //执行驱动的读函数

        //如果有按键按下 打印其对应按键值
        if (!key_vals[0] || !key_vals[1] || !key_vals[2] || !key_vals[3])
		{
			printf("%04d key pressed: %d %d %d %d\n", cnt++, key_vals[0], key_vals[1], key_vals[2], key_vals[3]);
		}


	}



}