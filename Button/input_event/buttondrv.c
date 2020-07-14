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
���Է�����
���û������QT��
cat /dev/tty1
��:s2,s3,s4
�Ϳ��Եõ�ls

���ߣ�
exec 0</dev/tty1  �ѱ�׼����ĳ�/dev/tty1 �ȴ�tty1��������
Ȼ�����ʹ�ð���������
*/

static struct timer_list buttons_timer; //��ʼ����ʱ��
static struct input_dev* buttons_dev;

typedef struct pin_desc
{
	int irq; //�жϺ�
	char* name;
	unsigned int pin;      //IO��
	unsigned int key_value;//��ֵ

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
	//���ö�ʱ������
	irq_pd = ( PT_pin_desc ) dev_id;

	mod_timer ( &buttons_timer, jiffies+HZ/100 ); //��ʱʱ��10ms
	return IRQ_RETVAL ( IRQ_HANDLED );

}

void button_timer ( unsigned long data )
{
	PT_pin_desc pindesc = irq_pd;
	unsigned int pinval;


	pinval = s3c2410_gpio_getpin ( pindesc->pin ); //��ð���ֵ

	if ( pinval ) //Ϊ�ʹ�����
	{
		/* �ɿ� : ���һ������: 0-�ɿ�, 1-���� */
		input_event ( buttons_dev, EV_KEY, pindesc->key_val, 0 ); //�����������¼� ��ֵΪ0
		input_sync ( buttons_dev );

	}
	else
	{
		/* �ɿ� : ���һ������: 0-�ɿ�, 1-���� */
		input_event ( buttons_dev, EV_KEY, pindesc->key_val, 1 ); //�����������¼� ��ֵΪ1
		input_sync ( buttons_dev );

	}
}


//ģ���ʼ��
static int buttons_init ( void )
{
	int i;


	/* 1. ����һ��input_dev�ṹ�� */
	buttons_dev = input_allocate_device();

	/* 2. ���� */
	/* 2.1 �ܲ��������¼� include\linux\input.h����*/
	set_bit ( EV_KEY, buttons_dev->evbit );
	set_bit ( EV_REP, buttons_dev->evbit );//�����ܹ������ظ��¼� Ч�����ǰ��°�������ϱ���ֵ

	/* 2.2 �ܲ���������������Щ�¼�: L,S,ENTER,LEFTSHIT */
	set_bit ( KEY_L, buttons_dev->keybit );
	set_bit ( KEY_S, buttons_dev->keybit );
	set_bit ( KEY_ENTER, buttons_dev->keybit );
	set_bit ( KEY_LEFTSHIFT, buttons_dev->keybit );

	/* 3. ע�� */
	input_register_device ( buttons_dev );


	/* 4. Ӳ����صĲ��� */
	//��ʱ����ʼ��
	init_timer ( &buttons_timer );
	//���ö�ʱ����ʱ����
	buttons_timer.function = button_timer;
	//��Ӷ�ʱ��
	add_timer ( &buttons_timer );

	//�жϳ�ʼ��
	for ( i=0; i<; i++ )
	{
		request_irq ( g_pindesc[i].irq, button_irq, IRQT_BOTHEDGE, g_pindesc[i].name, &g_pindesc[i].key_val );
	}
}

static int buttons_exit ( void )
{
	int i ;
	//�ͷ��ж�
	for ( i=0; i<; i++ )
	{
		free_irq ( g_pindesc[i].irq, &g_pindesc[i].key_val );
	}
	//�����ʱ��
	del_timer ( &buttons_timer );
	input_unregister_device ( buttons_dev ); //ȡ��input�ṹ��ע��
	input_free_device ( buttons_dev ); //�ͷ�����Ŀռ�


}

module_init ( buttons_init );

module_exit ( buttons_exit );

MODULE_LICENSE ( "GPL" );

