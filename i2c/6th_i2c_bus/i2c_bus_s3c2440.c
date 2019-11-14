#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of_i2c.h>
#include <linux/of_gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>

#include <asm/irq.h>

#include <plat/regs-iic.h>
#include <plat/iic.h>

//��ǰ״̬
#define PRINTK(...) 
enum s3c24xx_i2c_state
{
	STATE_IDLE,
	STATE_START,
	STATE_READ,
	STATE_WRITE,
	STATE_STOP
};


struct s3c2440_i2c_regs
{
	unsigned int iiccon;
	unsigned int iicstat;
	unsigned int iicadd;
	unsigned int iicds;
	unsigned int iiclc;
};

struct s3c2440_i2c_xfer_data
{
	struct i2c_msg* msgs;//msgs����ͷ
	int msn_num; //msg�ܹ�����
	int cur_msg; //��ǰ�ڴ���ڼ���
	int cur_ptr; //��ǰ����msg�еĵڼ����ֽ�
	int state; //��ǰ״̬
	int err;
	wait_queue_head_t wait;
};

static struct s3c2440_i2c_xfer_data s3c2440_i2c_xfer_data;


static struct s3c2440_i2c_regs* s3c2440_i2c_regs;

static void s3c2440_i2c_start ( void )
{

	s3c2440_i2c_xfer_data.state = STATE_START;

	//����msg��flag�Ƕ�����д �����豸��ַ���д״̬
	if ( s3c2440_i2c_xfer_data.msgs->flags & I2C_M_RD ) //��
	{
		s3c2440_i2c_regs->iicds	   = s3c2440_i2c_xfer_data.msgs->addr << 1;
		s3c2440_i2c_regs->iicstat  = 0xb0;	  // �������գ�����

	}
	else
	{
		s3c2440_i2c_regs->iicds	   = s3c2440_i2c_xfer_data.msgs->addr << 1;
		s3c2440_i2c_regs->iicstat  = 0xf0;		  // �������ͣ�����

	}
}

static void s3c2440_i2c_stop ( int err )
{
	s3c2440_i2c_xfer_data.state = STATE_STOP;
	s3c2440_i2c_xfer_data.err =err;

	//����msg��flag�Ƕ�����д �ж�Ӧ���͵�����
    PRINTK("STATE_STOP, err = %d\n", err);
	if ( s3c2440_i2c_xfer_data.msgs->flags & I2C_M_RD ) //��
	{
		// �������лָ�I2C����������P�ź�
		s3c2440_i2c_regs->iicstat =0x90;
		s3c2440_i2c_regs->iiccon = 0xaf;
		ndelay ( 50 );
	}
	else
	{
		// �������������ָ�I2C����������P�ź�
		s3c2440_i2c_regs->iicstat = 0xd0;
		s3c2440_i2c_regs->iiccon  = 0xaf;
		ndelay ( 50 ); // �ȴ�һ��ʱ���Ա�P�ź��Ѿ�����

	}

	//i2c�����������߶���
	wake_up ( &s3c2440_i2c_xfer_data.wait );

}



//i2c�豸�Ķ�д���ջ���õ��˺���
static  int s3c2440_i2c_xfer ( struct i2c_adapter* adap, struct i2c_msg* msgs,int num )
{
	unsigned long time_out;

	/* ��num��msg��I2C���ݷ��ͳ�ȥ/������ */
	s3c2440_i2c_xfer_data.msgs = msgs;
	s3c2440_i2c_xfer_data.msn_num = num;
	s3c2440_i2c_xfer_data.cur_msg = 0;
	s3c2440_i2c_xfer_data.cur_ptr = 0;
	s3c2440_i2c_xfer_data.err = -ENODEV;

	s3c2440_i2c_start();


	/* ����5��
	   ��(s3c2440_i2c_xfer_data.state == STATE_STOP)ʱ�᷵�أ���ʱҲ�᷵��
	*/
	time_out = wait_event_timeout ( s3c2440_i2c_xfer_data.wait, ( s3c2440_i2c_xfer_data.state == STATE_STOP ),HZ * 5 );
	//������Ϊ0ʱ �����ǳ�ʱ�˳�
	if ( time_out == 0 )
	{
		printk ( "s3c2440_i2c_xfer time out\n" );
		return -ETIMEDOUT;
	}
	else
	{
		return s3c2440_i2c_xfer_data.err;
	}

}


//���ر���������֧�ֵĹ���
static u32 s3c2440_i2c_func ( struct i2c_adapter* adap )
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_PROTOCOL_MANGLING;
}


static const struct i2c_algorithm s3c2440_i2c_algo =
{
	.master_xfer =  s3c2440_i2c_xfer,
	.functionality = s3c2440_i2c_func,
};

//�ж��Ƿ������һ����Ϣ
static int isLastMsg ( void )
{
	return ( s3c2440_i2c_xfer_data.cur_msg == s3c2440_i2c_xfer_data.msn_num - 1 );
}

//�жϵ�ǰ��Ϣ�Ƿ����
static int isEndData ( void )
{
	return ( s3c2440_i2c_xfer_data.cur_ptr >= s3c2440_i2c_xfer_data.msgs->len );
}

//�жϵ�ǰ��Ϣ�����һ������
static int isLastData ( void )
{
	return ( s3c2440_i2c_xfer_data.cur_ptr == s3c2440_i2c_xfer_data.msgs->len - 1 );
}


static irqreturn_t s3c2440_i2c_xfer_irq ( int irq, void* dev_id )
{
	unsigned int iicSt;
	iicSt  = s3c2440_i2c_regs->iicstat;

	//�����ж�i2c�����Ƿ�����ʧ��
	if ( iicSt & 0x8 )
	{
		printk ( "Bus arbitration failed\n\r" );
	}


	switch ( s3c2440_i2c_xfer_data.state )
	{
		case STATE_START:/* ����S���豸��ַ��,�����ж� */
		{
			PRINTK("Start\n");
			/* �жϷ�����start�ź����豸��ַ����û���յ�ack�����û��ACK, ���ش��� */
			if ( iicSt & S3C2410_IICSTAT_LASTBIT )
			{
				s3c2440_i2c_stop ( -ENODEV );
				break;
			}

			if ( isLastMsg() &&isEndData() )
			{
				s3c2440_i2c_stop ( 0 );
				break;
			}

			/* ������һ��״̬ */
			if ( s3c2440_i2c_xfer_data.msgs->flags & I2C_M_RD ) /* �� */
			{
				s3c2440_i2c_xfer_data.state = STATE_READ;
				goto next_read;
			}
			else
			{
				s3c2440_i2c_xfer_data.state = STATE_WRITE;
			}
		}

		case STATE_WRITE:
		{
			PRINTK("STATE_WRITE\n");
			/*ÿдһ�����ݻ����һ���ж�
			  �ж�i2c�豸�Ƿ�ظ�ack�ź�
			  ���û��ACK, ���ش���
			 */
			if ( iicSt & S3C2410_IICSTAT_LASTBIT )
			{
				s3c2440_i2c_stop ( -ENODEV );
				break;
			}

			if ( !isEndData() ) /* �����ǰmsg��������Ҫ���� */
			{
				s3c2440_i2c_regs->iicds = s3c2440_i2c_xfer_data.msgs->buf[s3c2440_i2c_xfer_data.cur_ptr];
				s3c2440_i2c_xfer_data.cur_ptr++;

				// ������д��IICDS����Ҫһ��ʱ����ܳ�����SDA����
				ndelay ( 50 );

				s3c2440_i2c_regs->iiccon = 0xaf;		  // �ָ�I2C���� ������֮��ᴥ���ж�
				break;

			}
			else if ( !isLastMsg() )
			{
				/* ��ʼ������һ����Ϣ */
				s3c2440_i2c_xfer_data.msgs++;
				s3c2440_i2c_xfer_data.cur_msg++;
				s3c2440_i2c_xfer_data.cur_ptr = 0;
				s3c2440_i2c_xfer_data.state = STATE_START;
				/* ����START�źźͷ����豸��ַ */
				s3c2440_i2c_start();
				break;

			}
			else
			{

				/* �����һ����Ϣ�����һ������ */
				s3c2440_i2c_stop ( 0 );
				break;

			}

			break;

		}

		case STATE_READ:
		{
			PRINTK("STATE_READ\n");
			/* �������� */
			s3c2440_i2c_xfer_data.msgs->buf[s3c2440_i2c_xfer_data.cur_ptr]  = s3c2440_i2c_regs->iicds;
			s3c2440_i2c_xfer_data.cur_ptr++;
next_read: //��һ�ν���next_readʱ��ֻ�Ƿ�����һ���豸��ַ����û�����ݿɶ�
			/* �������û��д, �������������
			*/

			if ( !isEndData() ) /* �����ǰmsg��������Ҫ�� */
			{
				if ( isLastData() ) /* ��������������������һ��, ����ack */
				{
					s3c2440_i2c_regs->iiccon = 0x2f;   // �ָ�I2C���䣬���յ���һ����ʱ��ACK

				}
				else
				{
					s3c2440_i2c_regs->iiccon = 0xaf;   // �ָ�I2C���䣬���յ���һ����ʱ����ACK

				}
				break;
			}
			else if ( !isLastMsg() )
			{
				/* ��ʼ������һ����Ϣ */
				s3c2440_i2c_xfer_data.msgs++;
				s3c2440_i2c_xfer_data.cur_msg++;
				s3c2440_i2c_xfer_data.cur_ptr = 0;
				s3c2440_i2c_xfer_data.state = STATE_START;
				/* ����START�źźͷ����豸��ַ */
				s3c2440_i2c_start();
				break;

			}
			else
			{

				/* �����һ����Ϣ�����һ������ */
				s3c2440_i2c_stop ( 0 );
				break;

			}
			break;
		}
		default: break;

	}

	/* ���ж� */
	s3c2440_i2c_regs->iiccon &= ~ ( S3C2410_IICCON_IRQPEND );


	return IRQ_HANDLED;
}


//i2c��ʼ��
static void s3c2440_i2c_init ( void )
{
	struct clk* clk;
	//ʹ��i2cʱ��
	clk =clk_get ( NULL,"i2c" );
	clk_enable ( clk );

	// �������Ź��ܣ�GPE15:IICSDA, GPE14:IICSCL
	s3c_gpio_cfgpin ( S3C2410_GPE ( 14 ),S3C2410_GPE14_IICSCL );
	s3c_gpio_cfgpin ( S3C2410_GPE ( 15 ),S3C2410_GPE15_IICSDA );


	/* bit[7] = 1, ʹ��ACK
	 * bit[6] = 0, IICCLK = PCLK/16
	 * bit[5] = 1, ʹ���ж�
	 * bit[3:0] = 0xf, Tx clock = IICCLK/16
	 * PCLK = 50MHz, IICCLK = 3.125MHz, Tx Clock = 0.195MHz
	 */
	s3c2440_i2c_regs->iiccon = ( 1<<7 ) | ( 0<<6 ) | ( 1<<5 ) | ( 0xf );
	s3c2440_i2c_regs->iicadd  = 0x10;	  // S3C24xx slave address = [7:1]
	s3c2440_i2c_regs->iicstat = 0x10;// I2C�������ʹ��(Rx/Tx)

}


/* 1. ����/����i2c_adapter
 */
static struct i2c_adapter s3c2440_i2c_adapter =
{
	.name = "s3c2440_100ask",
	.algo = &s3c2440_i2c_algo,//���÷������ߵ��㷨
	.owner  = THIS_MODULE,

};

static int i2c_bus_s3c2440_init ( void )
{

	//���������ַ
	s3c2440_i2c_regs = ioremap ( 0x54000000, sizeof ( struct s3c2440_i2c_regs ) );

	s3c2440_i2c_init();
	//��ָ���жϵĴ�����ʽ ֻҪ�ܲ����жϾͻᴥ��
	request_irq ( IRQ_IIC, s3c2440_i2c_xfer_irq, 0, "s3c2440-i2c", NULL );

	//����һ���ȴ�����
	init_waitqueue_head ( &s3c2440_i2c_xfer_data.wait );

	/* 2. ע��i2c_adapter */
	i2c_add_adapter ( &s3c2440_i2c_adapter );

	return 0;

}

static void i2c_bus_s3c2440_exit ( void )
{

	//�ͷ��ж�
	free_irq ( IRQ_IIC,NULL );
	//ɾ��i2c_adapter
	i2c_del_adapter ( &s3c2440_i2c_adapter );
	iounmap ( s3c2440_i2c_regs );

}

module_init ( i2c_bus_s3c2440_init );
module_exit ( i2c_bus_s3c2440_exit );
MODULE_LICENSE ( "GPL" );
