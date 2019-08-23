#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>

int major; //主设备号
static struct class *Buttonv_class; //设备节点
static struct class_device	*Button_class_dev;

volatile unsigned long *gpfcon;
volatile unsigned long *gpfdat;

volatile unsigned long *gpgcon;
volatile unsigned long *gpgdat;




//按键初始化
static int Button_open (struct inode *inode, struct file *file)
{
	/* 配置GPF0,2为输入引脚 */
	*gpfcon &=~ (((0x03) << (0*2)) | ((0x03) << (2*2)));
	
	/* 配置GPG3,11为输入引脚 */
	*gpgcon &=~ (((0x03) << (3*2)) | ((0x03) << (11*2)));
	return 0;



}


static ssize_t Read_Button(struct file *file, char __user *buf, size_t size , loff_t * ppos )
{
   unsigned char key_val[4];
   unsigned int key_val_len = sizeof(key_val)/sizeof(key_val[0]);
   int regval;

   //如果传入的长度不相等返回无效
   if(key_val_len != size)
   	  return -EINVAL;  /* Invalid argument */

   //读IO引脚寄存器
   regval = *gpfdat;
   
   /* 读GPF0,2 */
   key_val[0] = (regval & (1<<0))? 1:0;
   key_val[1] = (regval & (1<<2))? 1:0;

   /* 读GPG3,11 */ 
   regval = *gpgdat;

   key_val[0] = (regval & (1<<3))? 1:0;
   key_val[1] = (regval & (1<<11))? 1:0;

   //拷贝数据给用户
   copy_to_user(buf, key_val, key_val_len);
   
   return key_val_len; //成功读取的字节数 非负
}



static struct file_operations Button_drv_fops = {
    .owner  =   THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
    .open   =   Button_open,     
	.read	=	Read_Button,	   
};
	

static int Button_init(void)
{
    //返回值为系统分配的主设备号
    major = register_chrdev(0, "Button_drv", &Button_drv_fops);//主设备号 0-为自动分配   设备名  file_operations结构体

	//自动生成设备节点 /dev/buttons
	Buttonv_class = class_create(THIS_MODULE, "Button_drv"); 
	Button_class_dev = class_device_create(Buttonv_class, NULL, MKDEV(major, 0), NULL, "buttons"); /* /dev/buttons */

	//设置虚拟地址 0x56000050此为目标单板寄存器物理地址 16位长度
    gpfcon = (volatile unsigned long *)ioremap(0x56000050, 16); 
	gpfdat = gpfcon + 1;

	gpgcon = (volatile unsigned long *)ioremap(0x56000060, 16);
	gpgdat = gpgcon + 1;

	return 0;

}


static void Button_exit(void)
{
   unregister_chrdev(major, "Button_drv"); //取消注册

   //清除设备节点
   class_device_unregister(Button_class_dev);
   class_destroy(Buttonv_class);

   iounmap(gpfcon);
   iounmap(gpgcon);

   return 0;

}



//注册初始化以及退出驱动函数
module_init(Button_init);
module_exit(Button_exit);


MODULE_LICENSE("GPL"); //模块的许可证声明





