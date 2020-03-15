#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
//#include <asm/arch/regs-gpio.h>
//#include <asm/hardware.h>
#include <linux/device.h>
//#include <mach/gpio.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <mach/regs-gpio.h>
#include <mach/gpio-samsung.h>
#include <plat/gpio-cfg.h>

#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

struct pin_desc
{
   int irq;
   char *name;
   unsigned int pin;
   unsigned int key_val;
};

/* 键值: 按下时, 0x01, 0x02, 0x03, 0x04 */
/* 键值: 松开时, 0x81, 0x82, 0x83, 0x84 */
static unsigned char key_val;



static struct pin_desc pins_desc[4]=
{
	{.name = "S2",.key_val=0x01},
	{.name = "S3",.key_val=0x02},
	{.name = "S4",.key_val=0x03},
	{.name = "S5",.key_val=0x04},
};
static struct timer_list buttons_timer;
static int major;
static struct class *button_class;
static struct device *button_class_dev;
struct semaphore button_lock; //定义信号量

volatile unsigned long *gpfcon; 
volatile unsigned long *gpfdat;

volatile unsigned long *gpgcon;
volatile unsigned long *gpgdat;

static struct pin_desc *irq_pd;

static struct fasync_struct *button_async;

static irqreturn_t buttons_irq(int irq, void *dev_id)
{
   //启动定时器进行消抖处理 定时时间10ms
   irq_pd = (struct pin_desc *)dev_id;
   mod_timer(&buttons_timer,jiffies+1); //1等于10ms
   return IRQ_RETVAL(IRQ_HANDLED);
}


static ssize_t button_drv_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
   if(size!=1)
   	 return -EINVAL;
  
   copy_to_user(buf,&key_val,1);

   return 1;//返回读取到的数据长度
}

static int button_drv_open(struct inode *inode, struct file *file)
{ 
  int res,i;
  //申请中断
  for ( i=0; i< ( sizeof ( pins_desc ) /sizeof ( pins_desc[0] ) ); i++ )
  {
  	  res = request_irq(pins_desc[i].irq, buttons_irq, 0, pins_desc[i].name, &pins_desc[i].pin);
	  if(res)
	  {
	  	printk("request irq error index :%d",i);
	  }	
  }

  return 0;
  
  
}

static int button_drv_release (struct inode *inode, struct file *file)
{
   int i;
   //释放中断
   for (i=0; i< ( sizeof ( pins_desc ) /sizeof ( pins_desc[0] ) ); i++ )
   {
     free_irq(pins_desc[i].irq, &pins_desc[i].pin);
   }
   
   return 0;
}

static int button_drv_fasync (int fd, struct file *filp, int on)
{
   printk("driver: button_drv_fasync\n");
   
   return fasync_helper (fd, filp, on, &button_async);
}


static struct file_operations buttons_fops =
{
  .owner   =  THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
  .open = button_drv_open,
  .read = button_drv_read,
  .fasync = button_drv_fasync,
  .release  = button_drv_release,
  

};

static void buttons_timer_function(struct timer_list *t)
{
	struct pin_desc * pindesc = irq_pd;
    unsigned int pin_val;
	
	if(!pindesc)
	 return ;

	pin_val = gpio_get_value(pindesc->pin);
	if(pin_val)
	{
	   //松开
       key_val = 0x80 | pindesc->key_val; 

	}
	else
	{
       key_val = pindesc->key_val;

	}
	
	kill_fasync (&button_async, SIGIO, POLL_IN);
}



static int buttons_drv_init(void)
{
   //申请定时器
   timer_setup(&buttons_timer,buttons_timer_function,0);
   add_timer(&buttons_timer);

   //注册设备
   major = register_chrdev(0, "my_button", &buttons_fops);

   //创建设备节点 名字为/dev/buttons
   button_class = class_create(THIS_MODULE, "my_button");
   button_class_dev = device_create(button_class, NULL, MKDEV(major,0), NULL, "buttons");/* /dev/buttons */

   //重定位io
   gpfcon = (volatile unsigned long *)ioremap(0x56000050,16);
   gpfdat =gpfcon + 1; 
   
   gpgcon = (volatile unsigned long *)ioremap(0x56000060,16);
   gpgdat = gpgcon +1;

   //初始化信号量
   sema_init(&button_lock, 1);
   return 0;

}


static void buttons_drv_exit(void)
{
   iounmap(gpfcon);
   iounmap(gpgcon);
   device_destroy(button_class,MKDEV(major, 0));
   class_destroy(button_class);
   unregister_chrdev(major, "my_button");
   del_timer(&buttons_timer);
   
}



static int buttons_probe ( struct platform_device* pdev )
{
	struct device dev = pdev->dev;
	struct device_node* dev_node = dev.of_node;
	struct resource* res;
	int i;

	for ( i=0; i< ( sizeof ( pins_desc ) /sizeof ( pins_desc[0] ) ); i++ )
	{
		res = platform_get_resource ( pdev, IORESOURCE_IRQ, i );
		if ( res )
		{
			pins_desc[i].irq = res->start;
			printk ( "get irq:%d",pins_desc[i].irq );
		}
		else
		{
			printk ( "can't get irq:%d",i );
		}
        pins_desc[i].pin = of_get_named_gpio(dev_node,"eint-pins",i);
		printk("get gpio:%d",pins_desc[i].pin);	
	}
	return buttons_drv_init();

}


static int buttons_remove ( struct platform_device* pdev )
{
   buttons_drv_exit();
   return 0;

}

static const struct of_device_id of_match_buttons[] =
{
	{.compatible = "jz2440_button", .data= NULL},
	{/* sentinel */},

};


static struct platform_driver buttons_drv =
{
	.probe = buttons_probe,
	.remove = buttons_remove,
	.driver ={
		.name = "myButtons",
		.of_match_table = of_match_buttons, //表示匹配哪些设备树节点，匹配成功后调用probe函数
	}
};

static int buttons_init ( void )
{
	platform_driver_register ( &buttons_drv );
    return 0;
}

static void buttons_exit ( void )
{
	platform_driver_unregister ( &buttons_drv );
}

module_init ( buttons_init );
module_exit ( buttons_exit );
MODULE_LICENSE ( "GPL" );



