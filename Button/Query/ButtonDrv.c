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




//������ʼ��
static int Button_open (struct inode *inode, struct file *file)
{
	/* ����GPF0,2Ϊ�������� */
	*gpfcon &=~ (((0x03) << (0*2)) | ((0x03) << (2*2)));
	
	/* ����GPG3,11Ϊ�������� */
	*gpgcon &=~ (((0x03) << (3*2)) | ((0x03) << (11*2)));
	return 0;



}


static ssize_t Read_Button(struct file *file, char __user *buf, size_t size , loff_t * ppos )
{
   unsigned char key_val[4];
   unsigned int key_val_len = sizeof(key_val)/sizeof(key_val[0]);
   int regval;

   //�������ĳ��Ȳ���ȷ�����Ч
   if(key_val_len != size)
   	  return -EINVAL;  /* Invalid argument */

   //��IO���żĴ���
   regval = *gpfdat;
   
   /* ��GPF0,2 */
   key_val[0] = (regval & (1<<0))? 1:0;
   key_val[1] = (regval & (1<<2))? 1:0;

   /* ��GPG3,11 */ 
   regval = *gpgdat;

   key_val[0] = (regval & (1<<3))? 1:0;
   key_val[1] = (regval & (1<<11))? 1:0;

   //�������ݸ��û�
   copy_to_user(buf, key_val, key_val_len);
   
   return key_val_len; //�ɹ���ȡ���ֽ��� �Ǹ�
}



static struct file_operations Button_drv_fops = {
    .owner  =   THIS_MODULE,    /* ����һ���꣬�������ģ��ʱ�Զ�������__this_module���� */
    .open   =   Button_open,     
	.read	=	Read_Button,	   
};
	

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





