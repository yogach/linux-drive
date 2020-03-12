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
   unsigned int pin;
   unsigned int key_val;
}



static struct pin_desc pins_desc[4];
static struct timer_list buttons_timer;
static int major;
static struct class *button_class;
static struct device *button_class_dev;
struct semaphore button_lock; //�����ź���

volatile unsigned long *gpfcon; 
volatile unsigned long *gpfdat;

volatile unsigned long *gpgcon;
volatile unsigned long *gpgdat;




ssize_t button_drv_read(struct file *, char __user *, size_t, loff_t *)
{


}

int button_drv_open(struct inode *inode, struct file *file)
{

}

int button_drv_release (struct inode *, struct file *)
{

}

int button_drv_fasync (struct file *, loff_t, loff_t, int datasync)
{


}


static struct file_operations buttons_fops =
{
  .owner   =  THIS_MODULE,    /* ����һ���꣬�������ģ��ʱ�Զ�������__this_module���� */
  .open = button_drv_open,
  .read = button_drv_read,
  .fasync = button_drv_fasync,
  .release  = button_drv_release,
  

},

static int buttons_drv_init(void)
{
   //���붨ʱ��
   timer_setup(&buttons_timer,buttons_timer_function,0);
   add_timer(&buttons_timer);

   //ע���豸
   major = register_chrdev(0, "my_button", &buttons_fops);

   //�����豸�ڵ� ����Ϊ/dev/buttons
   button_class = class_create(THIS_MODULE, "my_button");
   button_class_dev = device_create(button_class, NULL, MKDEV(major,0), NULL, "buttons");/* /dev/buttons */

   //�ض�λio
   gpfcon = (volatile unsigned long *)ioremap(0x56000050,16);
   gpfdat =gpfcon + 1; 
   
   gpgcon = (volatile unsigned long *)ioremap(0x56000060,16);
   gpgdat = gpgcon +1;

   //��ʼ���ź���
   sema_init(&button_lock, 1);
   return 0;

}


static int buttons_drv_exit(void)
{
   iounmap(gpfcon);
   iounmap(gpgcon);
   device_destroy(button_class_dev);
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
			printk ( "get irq:%d",pin_desc[i].irq );
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
		.of_match_table = of_match_buttons, //��ʾƥ����Щ�豸���ڵ㣬ƥ��ɹ������probe����
	}
};

static int buttons_init ( void )
{
	platform_driver_register ( &buttons_drv );


}

static void buttons_exit ( void )
{
	platform_driver_unregister ( &buttons_drv );
}

module_init ( buttons_init );
module_exit ( buttons_exit );
MODULE_LICENSE ( "GPL" );



