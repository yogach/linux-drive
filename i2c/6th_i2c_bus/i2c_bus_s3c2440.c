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

enum s3c24xx_i2c_state
{


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
	struct i2c_msg* msgs;//msgs数组头
	int msn_num; //msg总共条数
	int cur_msg; //当前在处理第几条
	int cur_ptr; //当前处理msg中的第几个字节
	int state; //当前状态
	int err;
	wait_queue_head_t wait;
};

static struct s3c2440_i2c_xfer_data s3c2440_i2c_xfer_data;


static struct s3c2440_i2c_regs* s3c2440_i2c_regs;

static void s3c2440_i2c_start ( void )
{

	s3c2440_i2c_xfer_data.state = STATE_START;


	//根据msg中flag是读还是写 发送设备地址与读写状态
	if ( s3c2440_i2c_xfer_data.msgs->flags & I2C_M_RD ) //读
	{
		s3c2440_i2c_regs->iicds	   = s3c2440_i2c_xfer_data.msgs->addr << 1;
		s3c2440_i2c_regs->iicstat  = 0xb0;	  // 主机接收，启动

	}
	else
	{
		s3c2440_i2c_regs->iicds	   = s3c2440_i2c_xfer_data.msgs->addr << 1;
		s3c2440_i2c_regs->iicstat  = 0xf0;		  // 主机发送，启动

	}



}

static void s3c2440_i2c_stop ( int err )
{
  s3c2440_i2c_xfer_data.state = 



}



//i2c设备的读写最终会调用到此函数
int s3c2440_i2c_xfer ( struct i2c_adapter* adap, struct i2c_msg* msgs,int num )
{

	/* 把num个msg的I2C数据发送出去/读进来 */
	s3c2440_i2c_xfer_data.msgs = msgs;
	s3c2440_i2c_xfer_data.msn_num = num;
	s3c2440_i2c_xfer_data.cur_msg = 0;
	s3c2440_i2c_xfer_data.cur_ptr = 0;
	s3c2440_i2c_xfer_data.err = -ENODEV

	                            s3c2440_i2c_start();


}


//返回本总线所能支持的功能
static u32 s3c2440_i2c_func ( struct i2c_adapter* adap )
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_PROTOCOL_MANGLING;
}


static const struct i2c_algorithm s3c2440_i2c_algo =
{
	.master_xfer =  s3c2440_i2c_xfer,
	.functionality = s3c2440_i2c_func,
};



static irqreturn_t s3c2440_i2c_xfer_irq ( int irq, void* dev_id )
{

	return IRQ_HANDLED;
}


//i2c初始化
static void s3c2440_i2c_init ( void )
{
	struct clk* clk;
	//使能i2c时钟
	clk =clk_get ( NULL,"i2c" );
	clk_enable ( clk );

	// 设置引脚功能：GPE15:IICSDA, GPE14:IICSCL
	s3c_gpio_cfgpin ( S3C2410_GPE ( 14 ),S3C2410_GPE14_IICSCL );
	s3c_gpio_cfgpin ( S3C2410_GPE ( 15 ),S3C2410_GPE15_IICSDA );


	/* bit[7] = 1, 使能ACK
	 * bit[6] = 0, IICCLK = PCLK/16
	 * bit[5] = 1, 使能中断
	 * bit[3:0] = 0xf, Tx clock = IICCLK/16
	 * PCLK = 50MHz, IICCLK = 3.125MHz, Tx Clock = 0.195MHz
	 */
	s3c2440_i2c_regs->iiccon = ( 1<<7 ) | ( 0<<6 ) | ( 1<<5 ) | ( 0xf );
	s3c2440_i2c_regs->iicadd  = 0x10;	  // S3C24xx slave address = [7:1]
	s3c2440_i2c_regs->iicstat = 0x10;// I2C串行输出使能(Rx/Tx)

}


/* 1. 分配/设置i2c_adapter
 */
static struct i2c_adapter s3c2440_i2c_adapter =
{
	.name = "s3c2440_100ask",
	.algo = &s3c2440_i2c_algo,//设置访问总线的算法
	.owner  = THIS_MODULE,

};

static int i2c_bus_s3c2440_init ( void )
{

	//分配虚拟地址
	s3c2440_i2c_regs = ioremap ( 0x54000000, sizeof ( struct s3c2440_i2c_regs ) );

	s3c2440_i2c_init();
	//不指定中断的触发方式 只要能产生中断就会触发
	request_irq ( IRQ_IIC, s3c2440_i2c_xfer_irq, 0, "s3c2440-i2c", NULL );


	/* 2. 注册i2c_adapter */
	i2c_add_adapter ( &s3c2440_i2c_adapter );

	return 0;

}

static void i2c_bus_s3c2440_exit ( void )
{

	//释放中断
	free_irq ( IRQ_IIC,NULL );
	//删除i2c_adapter
	i2c_del_adapter ( &s3c2440_i2c_adapter );
	iounmap ( s3c2440_i2c_regs );

}

module_init ( i2c_bus_s3c2440_init );
module_exit ( i2c_bus_s3c2440_init );
MODULE_LICENSE ( "GPL" );
