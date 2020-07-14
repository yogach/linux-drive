#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>


/*
测试方法：
如果没有启动QT：
cat /dev/tty1
按:s2,s3,s4
就可以得到ls

或者：
exec 0</dev/tty1  把标准输入改成/dev/tty1 等待tty1输入数据
然后可以使用按键来输入
*/

static struct timer_list buttons_timer; //初始化定时器
static struct input_dev* buttons_dev;

typedef struct pin_desc
{
	int irq; //中断号
	char* name;
	unsigned int pin;      //IO口
	unsigned int key_value;//键值

} T_pin_desc,*PT_pin_desc;

static T_pin_desc g_pindesc[4]=
{
	{IRQ_EINT0,"S1",S3C2410_GPF0,KEY_L},
	{IRQ_EINT2,"S2",S3C2410_GPF2,KEY_S},
	{IRQ_EINT11,"S3",S3C2410_GPG3,KEY_ENTER},
	{IRQ_EINT19,"S4",S3C2410_GPG11,KEY_LEFTSHIFT},
}

static struct pin_desc* irq_pd;


irqreturn_t button_irq ( int irq, void* dev_id )
{
	//重置定时器计数
	irq_pd = ( PT_pin_desc ) dev_id;

	mod_timer ( &buttons_timer, jiffies+HZ/100 ); //定时时间10ms
	return IRQ_RETVAL ( IRQ_HANDLED );

}

void button_timer ( unsigned long data )
{
	PT_pin_desc pindesc = irq_pd;
	unsigned int pinval;


	pinval = s3c2410_gpio_getpin ( pindesc->pin ); //获得按键值

	if ( pinval ) //为低代表按下
	{
		/* 松开 : 最后一个参数: 0-松开, 1-按下 */
		input_event ( buttons_dev, EV_KEY, pindesc->key_val, 0 ); //产生按键类事件 键值为0
		input_sync ( buttons_dev );

	}
	else
	{
		/* 松开 : 最后一个参数: 0-松开, 1-按下 */
		input_event ( buttons_dev, EV_KEY, pindesc->key_val, 1 ); //产生按键类事件 键值为1
		input_sync ( buttons_dev );

	}
}


//模块初始化
static int buttons_init ( void )
{
	int i;


	/* 1. 分配一个input_dev结构体 */
	buttons_dev = input_allocate_device();

	/* 2. 设置 */
	/* 2.1 能产生哪类事件 include\linux\input.h定义*/
	set_bit ( EV_KEY, buttons_dev->evbit );
	set_bit ( EV_REP, buttons_dev->evbit );//设置能够产生重复事件 效果就是按下按键后会上报键值

	/* 2.2 能产生这类操作里的哪些事件: L,S,ENTER,LEFTSHIT */
	set_bit ( KEY_L, buttons_dev->keybit );
	set_bit ( KEY_S, buttons_dev->keybit );
	set_bit ( KEY_ENTER, buttons_dev->keybit );
	set_bit ( KEY_LEFTSHIFT, buttons_dev->keybit );

	/* 3. 注册 */
	input_register_device ( buttons_dev );


	/* 4. 硬件相关的操作 */
	//定时器初始化
	init_timer ( &buttons_timer );
	//设置定时器超时函数
	buttons_timer.function = button_timer;
	//添加定时器
	add_timer ( &buttons_timer );

	//中断初始化
	for ( i=0; i<; i++ )
	{
		request_irq ( g_pindesc[i].irq, button_irq, IRQT_BOTHEDGE, g_pindesc[i].name, &g_pindesc[i].key_val );
	}
}

static int buttons_exit ( void )
{
	int i ;
	//释放中断
	for ( i=0; i<; i++ )
	{
		free_irq ( g_pindesc[i].irq, &g_pindesc[i].key_val );
	}
	//清除定时器
	del_timer ( &buttons_timer );
	input_unregister_device ( buttons_dev ); //取消input结构体注册
	input_free_device ( buttons_dev ); //释放申请的空间


}

module_init ( buttons_init );

module_exit ( buttons_exit );

MODULE_LICENSE ( "GPL" );

