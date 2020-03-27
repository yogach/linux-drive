#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/irq.h>

//#include <asm/plat-s3c24xx/ts.h>

//#include <asm/arch/regs-adc.h>
//#include <asm/arch/regs-gpio.h>

static int irq_tc = IRQ_TC;
static int irq_adc = IRQ_ADC;
struct s3c_ts_regs
{
	unsigned long adccon;
	unsigned long adctsc;
	unsigned long adcdly;
	unsigned long adcdat0;
	unsigned long adcdat1;
	unsigned long adcupdn;
};

static struct input_dev* s3c_ts_dev;
static volatile struct s3c_ts_regs* s3c_ts_regs;

static struct timer_list ts_timer;



static void enter_wait_pen_down_mode ( void )
{
	s3c_ts_regs->adctsc = 0xd3;
}

static void enter_wait_pen_up_mode ( void )
{
	s3c_ts_regs->adctsc = 0x1d3;
}

static void enter_measure_xy_mode ( void )
{
	s3c_ts_regs->adctsc = ( 1<<3 ) | ( 1<<2 );
}

static void start_adc ( void )
{
	s3c_ts_regs->adccon |= ( 1<<0 );
}

static int s3c_filter_ts ( int x[], int y[] )
{
#define ERR_LIMIT 10

	int avr_x, avr_y;
	int det_x, det_y;

	avr_x = ( x[0] + x[1] ) /2;
	avr_y = ( y[0] + y[1] ) /2;

	det_x = ( x[2] > avr_x ) ? ( x[2] - avr_x ) : ( avr_x - x[2] );
	det_y = ( y[2] > avr_y ) ? ( y[2] - avr_y ) : ( avr_y - y[2] );

	if ( ( det_x > ERR_LIMIT ) || ( det_y > ERR_LIMIT ) )
	{
		return 0;
	}

	avr_x = ( x[1] + x[2] ) /2;
	avr_y = ( y[1] + y[2] ) /2;

	det_x = ( x[3] > avr_x ) ? ( x[3] - avr_x ) : ( avr_x - x[3] );
	det_y = ( y[3] > avr_y ) ? ( y[3] - avr_y ) : ( avr_y - y[3] );

	if ( ( det_x > ERR_LIMIT ) || ( det_y > ERR_LIMIT ) )
	{
		return 0;
	}

	return 1;
}

static void s3c_ts_timer_function ( struct timer_list* t )
{
	if ( s3c_ts_regs->adcdat0 & ( 1<<15 ) )
	{
		/* �Ѿ��ɿ� */
		input_report_abs ( s3c_ts_dev, ABS_PRESSURE, 0 );
		input_report_key ( s3c_ts_dev, BTN_TOUCH, 0 );
		input_sync ( s3c_ts_dev );
		enter_wait_pen_down_mode();
	}
	else
	{
		/* ����X/Y���� */
		enter_measure_xy_mode();
		start_adc();
	}
}


static irqreturn_t pen_down_up_irq ( int irq, void* dev_id )
{
	if ( s3c_ts_regs->adcdat0 & ( 1<<15 ) )
	{
		//printk("pen up\n");
		input_report_abs ( s3c_ts_dev, ABS_PRESSURE, 0 );
		input_report_key ( s3c_ts_dev, BTN_TOUCH, 0 );
		input_sync ( s3c_ts_dev );
		enter_wait_pen_down_mode();
	}
	else
	{
		//printk("pen down\n");
		//enter_wait_pen_up_mode();
		enter_measure_xy_mode();
		start_adc();
	}
	return IRQ_HANDLED;
}

static irqreturn_t adc_irq ( int irq, void* dev_id )
{
	static int cnt = 0;
	static int x[4], y[4];
	int adcdat0, adcdat1;


	/* �Ż���ʩ2: ���ADC���ʱ, ���ִ������Ѿ��ɿ�, �����˴ν�� */
	adcdat0 = s3c_ts_regs->adcdat0;
	adcdat1 = s3c_ts_regs->adcdat1;

	if ( s3c_ts_regs->adcdat0 & ( 1<<15 ) )
	{
		/* �Ѿ��ɿ� */
		cnt = 0;
		input_report_abs ( s3c_ts_dev, ABS_PRESSURE, 0 );
		input_report_key ( s3c_ts_dev, BTN_TOUCH, 0 );
		input_sync ( s3c_ts_dev );
		enter_wait_pen_down_mode();
	}
	else
	{
		// printk("adc_irq cnt = %d, x = %d, y = %d\n", ++cnt, adcdat0 & 0x3ff, adcdat1 & 0x3ff);
		/* �Ż���ʩ3: ��β�����ƽ��ֵ */
		x[cnt] = adcdat0 & 0x3ff;
		y[cnt] = adcdat1 & 0x3ff;
		++cnt;
		if ( cnt == 4 )
		{
			/* �Ż���ʩ4: ������� */
			if ( s3c_filter_ts ( x, y ) )
			{
				//printk("x = %d, y = %d\n", (x[0]+x[1]+x[2]+x[3])/4, (y[0]+y[1]+y[2]+y[3])/4);
				input_report_abs ( s3c_ts_dev, ABS_X, ( x[0]+x[1]+x[2]+x[3] ) /4 );
				input_report_abs ( s3c_ts_dev, ABS_Y, ( y[0]+y[1]+y[2]+y[3] ) /4 );
				input_report_abs ( s3c_ts_dev, ABS_PRESSURE, 1 );
				input_report_key ( s3c_ts_dev, BTN_TOUCH, 1 );
				input_sync ( s3c_ts_dev );
			}
			cnt = 0;
			enter_wait_pen_up_mode();

			/* ������ʱ��������/��������� */
			mod_timer ( &ts_timer, jiffies + HZ/100 );
		}
		else
		{
			enter_measure_xy_mode();
			start_adc();
		}
	}

	return IRQ_HANDLED;
}

static int s3c_ts_init ( void )
{
	struct clk* clk;

	/* 1. ����һ��input_dev�ṹ�� */
	s3c_ts_dev = input_allocate_device();

	/* 2. ���� */
	/* 2.1 �ܲ��������¼� */
	set_bit ( EV_KEY, s3c_ts_dev->evbit );
	set_bit ( EV_ABS, s3c_ts_dev->evbit );

	/* 2.2 �ܲ��������¼������Щ�¼� */
	set_bit ( BTN_TOUCH, s3c_ts_dev->keybit );

	input_set_abs_params ( s3c_ts_dev, ABS_X, 0, 0x3FF, 0, 0 );
	input_set_abs_params ( s3c_ts_dev, ABS_Y, 0, 0x3FF, 0, 0 );
	input_set_abs_params ( s3c_ts_dev, ABS_PRESSURE, 0, 1, 0, 0 );


	/* 3. ע�� */
	input_register_device ( s3c_ts_dev );

	/* 4. Ӳ����صĲ��� */
	/* 4.1 ʹ��ʱ��(CLKCON[15]) */
	clk = clk_get ( NULL, "adc" );
	clk_prepare_enable ( clk );

	/* 4.2 ����S3C2440��ADC/TS�Ĵ��� */
	s3c_ts_regs = ioremap ( 0x58000000, sizeof ( struct s3c_ts_regs ) );

	/* bit[14]  : 1-A/D converter prescaler enable
	 * bit[13:6]: A/D converter prescaler value,
	 *            49, ADCCLK=PCLK/(49+1)=50MHz/(49+1)=1MHz
	 * bit[0]: A/D conversion starts by enable. ����Ϊ0
	 */
	s3c_ts_regs->adccon = ( 1<<14 ) | ( 49<<6 );

	request_irq ( irq_tc, pen_down_up_irq, 0, "ts_pen", NULL );
	request_irq ( irq_adc, adc_irq, 0, "adc", NULL );

	/* �Ż���ʩ1:
	 * ����ADCDLYΪ���ֵ, ��ʹ�õ�ѹ�ȶ����ٷ���IRQ_TC�ж�
	 */
	s3c_ts_regs->adcdly = 0xffff;

	/* �Ż���ʩ5: ʹ�ö�ʱ��������,���������
	 *
	 */
	//init_timer(&ts_timer);
	//ts_timer.function = s3c_ts_timer_function;
	timer_setup ( &ts_timer, s3c_ts_timer_function, 0 );
	add_timer ( &ts_timer );

	enter_wait_pen_down_mode();

	return 0;
}

static void s3c_ts_exit ( void )
{
	free_irq ( irq_tc, NULL );
	free_irq ( irq_adc, NULL );
	iounmap ( s3c_ts_regs );
	input_unregister_device ( s3c_ts_dev );
	input_free_device ( s3c_ts_dev );
	del_timer ( &ts_timer );
}

static int s3c_ts_probe ( struct platform_device* pdev )
{
	struct device dev = pdev->dev;
	struct device_node* dev_node = dev.of_node;
	struct resource* res;
	int i;

	//�����豸������ ��ȡ�ж���Դ
	res = platform_get_resource ( pdev, IORESOURCE_IRQ, 0 ); //��ȡirq��Դ
	if ( res )
	{
		irq_tc = res->start;
		printk ( "get ts irq:%d\r\n",irq_tc );
	}
	else
	{
		printk ( "get ts irq error\r\n" );
		return -1;
	}

	//�����豸������ ��ȡ�ж���Դ
	res = platform_get_resource ( pdev, IORESOURCE_IRQ, 1 ); //��ȡirq��Դ
	if ( res )
	{
		irq_adc = res->start;
		printk ( "get adc irq:%d\r\n",irq_adc );
	}
	else
	{
		printk ( "get adc irq error\r\n" );
		return -1;
	}



	return s3c_ts_init();

}


static int s3c_ts_remove ( struct platform_device* pdev )
{
	s3c_ts_exit();
	return 0;

}

static const struct of_device_id of_match_s3c_ts[] =
{
	{.compatible = "jz2440,ts", .data= NULL}, //ƥ������Ϊjz2440,ts���ӽڵ�
	{/* sentinel */},

};


static struct platform_driver s3c_ts_drv =
{
	.probe = s3c_ts_probe,
	.remove = s3c_ts_remove,
	.driver ={
		.name = "s3c_ts",
		.of_match_table = of_match_s3c_ts, //��ʾƥ����Щ�豸���ڵ㣬ƥ��ɹ������probe����
	}
};

static int s3c_ts_drv_init ( void )
{
	platform_driver_register ( &s3c_ts_drv );
	return 0;
}

static void s3c_ts_drv_exit ( void )
{
	platform_driver_unregister ( &s3c_ts_drv );
}

module_init ( s3c_ts_drv_init );
module_exit ( s3c_ts_drv_exit );
MODULE_LICENSE ( "GPL" );



