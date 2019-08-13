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

int major; //���豸��
static struct class *Buttonv_class; //�豸�ڵ�
static struct class_device	*Button_class_dev;

volatile unsigned long *gpfcon;
volatile unsigned long *gpfdat;

volatile unsigned long *gpgcon;
volatile unsigned long *gpgdat;


typedef struct Pin_Desc
{
  unsigned int pin;
  unsigned int key_val;

}T_Pin_Desc,*PT_Pin_Desc;


static T_Pin_Desc pins_desc[4] =
{
    {S3C2410_GPF0,0x01},
    {S3C2410_GPF2,0x02},
    {S3C2410_GPG3,0x03},
    {S3C2410_GPG11,0x04},
};


static irqreturn_t Button_handler(int irq, void *dev_id)
{
  PT_Pin_Desc pin_desc = (PT_Pin_Desc)dev_id;
  unsigned int pinval;

  pinval = s3c2410_gpio_getpin(pin_desc->pin); //����2410�⺯�� ��ȡio��״̬

  if()
  

  return IRQ_RETVAL(IRQ_HANDLED);

}

//������ʼ��
static int Button_open (struct inode *inode, struct file *file)
{
    //��ʼ���ж�       �жϺ� ������ ������ʽ �ж��� �豸id
	request_irq(IRQ_EINT0, Button_handler,IRQT_BOTHEDGE, "button1", &pins_desc[0]);
	request_irq(IRQ_EINT2, Button_handler,IRQT_BOTHEDGE, "button2", &pins_desc[1]);
	request_irq(IRQ_EINT11, Button_handler,IRQT_BOTHEDGE, "button3", &pins_desc[2]);
	request_irq(IRQ_EINT19, Button_handler,IRQT_BOTHEDGE, "button4", &pins_desc[3]);
	return 0;
}


static ssize_t Read_Button(struct file *file, char __user *buf, size_t size , loff_t * ppos )
{
   

   //�������ݸ��û�
   copy_to_user(buf, key_val, key_val_len);
   
   return key_val_len; //�ɹ���ȡ���ֽ��� �Ǹ�
}



static struct file_operations Button_drv_fops = {
    .owner  =   THIS_MODULE,    /* ����һ���꣬�������ģ��ʱ�Զ�������__this_module���� */
    .open   =   Button_open,     
	.read	=	Read_Button,	   
};
	

//ע���豸ʱִ��
static int Button_init(void)
{
    //����ֵΪϵͳ��������豸��
    major = register_chrdev(0, "Button_drv", &Button_drv_fops);//���豸�� 0-Ϊ�Զ�����   �豸��  file_operations�ṹ��

	//�Զ������豸�ڵ� /dev/buttons
	Buttonv_class = class_create(THIS_MODULE, "Button_drv"); 
	Button_class_dev = class_device_create(Buttonv_class, NULL, MKDEV(major, 0), NULL, "buttons"); /* /dev/buttons */

	//���������ַ 0x56000050��ΪĿ�굥��Ĵ��������ַ 16λ����
    gpfcon = (volatile unsigned long *)ioremap(0x56000050, 16); 
	gpfdat = gpfcon + 1;

	gpgcon = (volatile unsigned long *)ioremap(0x56000060, 16);
	gpgdat = gpgcon + 1;

	return 0;

}


static void Button_exit(void)
{
   unregister_chrdev(major, "Button_drv"); //ȡ��ע��

   //����豸�ڵ�
   class_device_unregister(Button_class_dev);
   class_destroy(Buttonv_class);

   iounmap(gpfcon);
   iounmap(gpgcon);

   return 0;

}



//ע���ʼ���Լ��˳���������
module_init(Button_init);
module_exit(Button_exit);


MODULE_LICENSE("GPL"); //ģ������֤����





