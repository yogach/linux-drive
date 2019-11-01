#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <asm/atomic.h>
#include <asm/unaligned.h>

#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf-core.h>

// CAMIF GPIO
static unsigned long* GPJCON;
static unsigned long* GPJDAT;
static unsigned long* GPJUP;

// CAMIF
/*
CISRCFMT:
	bit[31] -- 选择传输方式为BT601或者BT656
	bit[30] -- 设置偏移值(0 = +0 (正常情况下) - for YCbCr)
	bit[29] -- 保留位,必须设置为0
	bit[28:16]	-- 设置源图片的水平像素值(640)
	bit[15:14]	-- 设置源图片的颜色顺序(0x02)
	bit[12:0]		-- 设置源图片的垂直像素值(480)
*/
static unsigned long* CISRCFMT;
/*
CIWDOFST:
	bit[31] 	-- 1 = 使能窗口功能、0 = 不使用窗口功能
	bit[30、15:12]-- 清除溢出标志位
	bit[26:16]	-- 水平方向的裁剪的大小
	bit[10:0]		-- 垂直方向的裁剪的大小
*/
static unsigned long* CIWDOFST;

/*
CIGCTRL:
	bit[31] 	-- 软件复位CAMIF控制器
	bit[30] 	-- 用于复位外部摄像头模块
	bit[29] 	-- 保留位，必须设置为1
	bit[28:27]	-- 用于选择信号源(00 = 输入源来自摄像头模块)
	bit[26] 	-- 设置像素时钟的极性(猜0)
	bit[25] 	-- 设置VSYNC的极性(0)
	bit[24] 	-- 设置HREF的极性(0)
*/
static unsigned long* CIGCTRL;

//设置显存物理地址
static unsigned long* CIPRCLRSA1;
static unsigned long* CIPRCLRSA2;
static unsigned long* CIPRCLRSA3;
static unsigned long* CIPRCLRSA4;
/*
CIPRTRGFMT:
	bit[28:16] -- 表示目标图片的水平像素大小(TargetHsize_Pr)
	bit[15:14] -- 是否旋转，我们这个驱动就不选择了
	bit[12:0]	 -- 表示目标图片的垂直像素大小(TargetVsize_Pr)
*/
static unsigned long* CIPRTRGFMT;
/*
CIPRCTRL:
	bit[23:19] -- 主突发长度(Main_burst)
	bit[18:14] -- 剩余突发长度(Remained_burst)
	bit[2]	  -- 是否使能LastIRQ功能(不使能)
*/
static unsigned long* CIPRCTRL;
/*
CIPRSCPRERATIO:
	bit[31:28]: 预览缩放的变化系数(SHfactor_Pr)
	bit[22:16]: 预览缩放的水平比(PreHorRatio_Pr)
	bit[6:0]: 预览缩放的垂直比(PreVerRatio_Pr)

CIPRSCPREDST:
	bit[27:16]: 预览缩放的目标宽度(PreDstWidth_Pr)
	bit[11:0]: 预览缩放的目标高度(PreDstHeight_Pr)

CIPRSCCTRL:
	bit[29:28]: 告诉摄像头控制器(图片是缩小、放大)(ScaleUpDown_Pr)
	bit[24:16]: 预览主缩放的水平比(MainHorRatio_Pr)
	bit[8:0]: 预览主缩放的垂直比(MainVerRatio_Pr)

	bit[31]: 必须固定设置为1
	bit[30]: 设置图像输出格式是RGB16、RGB24
	bit[15]: 预览缩放开始
*/
static unsigned long* CIPRSCPRERATIO;
static unsigned long* CIPRSCPREDST;
static unsigned long* CIPRSCCTRL;
/*
CIPRTAREA:
	表示预览通道的目标区域
*/
static unsigned long* CIPRTAREA;
/*
CIIMGCPT:
	bit[31]: 用来使能摄像头控制器
	bit[30]: 使能编码通道
	bit[29]: 使能预览通道
*/
static unsigned long* CIIMGCPT;

// IRQ
static unsigned long* SRCPND;
static unsigned long* INTPND;
static unsigned long* SUBSRCPND;

//iic总线操作
static struct i2c_client* cmos_ov7740_client;

//源数据分辨率
static unsigned int SRC_Width, SRC_Height;
static unsigned long buf_size;

typedef struct cmos_ov7740_i2c_value
{
	unsigned char regaddr;
	unsigned char value;
} ov7740_t ;

/* init: 640x480,30fps的,YUV422输出格式 */
ov7740_t ov7740_setting_30fps_VGA_640_480[] =
{
	{0x12, 0x80},
	{0x47, 0x02},
	{0x17, 0x27},
	{0x04, 0x40},
	{0x1B, 0x81},
	{0x29, 0x17},
	{0x5F, 0x03},
	{0x3A, 0x09},
	{0x33, 0x44},
	{0x68, 0x1A},

	{0x14, 0x38},
	{0x5F, 0x04},
	{0x64, 0x00},
	{0x67, 0x90},
	{0x27, 0x80},
	{0x45, 0x41},
	{0x4B, 0x40},
	{0x36, 0x2f},
	{0x11, 0x01},
	{0x36, 0x3f},
	{0x0c, 0x12},

	{0x12, 0x00},
	{0x17, 0x25},
	{0x18, 0xa0},
	{0x1a, 0xf0},
	{0x31, 0xa0},
	{0x32, 0xf0},

	{0x85, 0x08},
	{0x86, 0x02},
	{0x87, 0x01},
	{0xd5, 0x10},
	{0x0d, 0x34},
	{0x19, 0x03},
	{0x2b, 0xf8},
	{0x2c, 0x01},

	{0x53, 0x00},
	{0x89, 0x30},
	{0x8d, 0x30},
	{0x8f, 0x85},
	{0x93, 0x30},
	{0x95, 0x85},
	{0x99, 0x30},
	{0x9b, 0x85},

	{0xac, 0x6E},
	{0xbe, 0xff},
	{0xbf, 0x00},
	{0x38, 0x14},
	{0xe9, 0x00},
	{0x3D, 0x08},
	{0x3E, 0x80},
	{0x3F, 0x40},
	{0x40, 0x7F},
	{0x41, 0x6A},
	{0x42, 0x29},
	{0x49, 0x64},
	{0x4A, 0xA1},
	{0x4E, 0x13},
	{0x4D, 0x50},
	{0x44, 0x58},
	{0x4C, 0x1A},
	{0x4E, 0x14},
	{0x38, 0x11},
	{0x84, 0x70}
};

struct camif_buffer
{

	unsigned int order;
	unsigned long virt_address;
	unsigned long phy_address;
};

struct camif_buffer img_buff[] =
{
	{
		.order = 0,
		.virt_address = ( unsigned long ) NULL,
		.phy_address = ( unsigned long ) NULL

	},

	{
		.order = 0,
		.virt_address = ( unsigned long ) NULL,
		.phy_address = ( unsigned long ) NULL

	},

	{
		.order = 0,
		.virt_address = ( unsigned long ) NULL,
		.phy_address = ( unsigned long ) NULL

	},

	{
		.order = 0,
		.virt_address = ( unsigned long ) NULL,
		.phy_address = ( unsigned long ) NULL

	}

};


#define OV7740_INIT_REGS_SIZE  sizeof(ov7740_setting_30fps_VGA_640_480)/sizeof(ov7740_setting_30fps_VGA_640_480[0])
#define CAM_SRC_XRES (640)
#define CAM_SRC_YRES (480)
#define CAM_XRES_OFFSET (0)
#define CAM_YRES_OFFSET (0)

#define CAM_ORDER_YCbYCr (0)
#define CAM_ORDER_YCrYCb (1)
#define CAM_ORDER_CbYCrY (2)
#define CAM_ORDER_CrYCbY (3)




/* 参考 uvc_v4l2_do_ioctl */
static int cmos_ov7740_vidioc_querycap ( struct file* file, void*  priv,
                                         struct v4l2_capability* cap )
{
	return 0;
}

/* 列举支持哪种格式
 * 参考: uvc_fmts 数组
 */
static int cmos_ov7740_vidioc_enum_fmt_vid_cap ( struct file* file, void*  priv,
                                                 struct v4l2_fmtdesc* f )
{
	return 0;
}

/* 返回当前所使用的格式 */
static int cmos_ov7740_vidioc_g_fmt_vid_cap ( struct file* file, void* priv,
                                              struct v4l2_format* f )
{
	return 0;
}

/* 测试驱动程序是否支持某种格式, 强制设置该格式
 * 参考: uvc_v4l2_try_format
 *       myvivi_vidioc_try_fmt_vid_cap
 */
static int cmos_ov7740_vidioc_try_fmt_vid_cap ( struct file* file, void* priv,
                                                struct v4l2_format* f )
{
	return 0;
}

/* 参考 myvivi_vidioc_s_fmt_vid_cap */
static int cmos_ov7740_vidioc_s_fmt_vid_cap ( struct file* file, void* priv,
                                              struct v4l2_format* f )
{
	return 0;
}

//申请缓存区
static int cmos_ov7740_vidioc_reqbufs ( struct file* file, void* priv, struct v4l2_requestbuffers* p )
{
	unsigned int order;

	order = get_order ( buf_size ); //计算buf_size需要的order值的大小(按字节计算)
   
	/************************************************/
	
	img_buff[0].order = order;
	//申请空间 用于DMA的内存，可以睡眠 GFP_DMA | GFP_KERNEL
	img_buff[0].virt_address =  __get_free_pages ( GFP_KERNEL|__GFP_DMA,order );
	if ( !img_buff[0].virt_address )
	{	    
	    printk ( "[%s]:%s __get_free_pages error ",__func__,__LINE__ );
		goto error;
	}
	img_buff[0].phy_address = __virt_to_phys ( img_buff[0].virt_address );
	
	/************************************************/
	img_buff[1].order = order;
	//申请空间 用于DMA的内存，可以睡眠 GFP_DMA | GFP_KERNEL
	img_buff[1].virt_address =  __get_free_pages ( GFP_KERNEL|__GFP_DMA,order );
	if ( !img_buff[1].virt_address )
	{
	    printk ( "[%s]:%s __get_free_pages error ",__func__,__LINE__ );
		goto error1;
	}
	img_buff[1].phy_address = __virt_to_phys ( img_buff[1].virt_address );
	
	/************************************************/
	img_buff[2].order = order;
	//申请空间 用于DMA的内存，可以睡眠 GFP_DMA | GFP_KERNEL
	img_buff[2].virt_address =  __get_free_pages ( GFP_KERNEL|__GFP_DMA,order );
	if ( !img_buff[2].virt_address )
	{
	    printk ( "[%s]:%s __get_free_pages error ",__func__,__LINE__ );
		goto error2;
	}
	img_buff[2].phy_address = __virt_to_phys ( img_buff[2].virt_address );
	
	/************************************************/
	img_buff[3].order = order;
	//申请空间 用于DMA的内存，可以睡眠 GFP_DMA | GFP_KERNEL
	img_buff[3].virt_address =  __get_free_pages ( GFP_KERNEL|__GFP_DMA,order );
	if ( !img_buff[3].virt_address )
	{
	    printk ( "[%s]:%s __get_free_pages error ",__func__,__LINE__ );
		goto error3;
	}
	img_buff[3].phy_address = __virt_to_phys ( img_buff[3].virt_address );
	/************************************************/
	return 0;


error3:
	free_pages(img_buff[2].virt_address,order);
	img_buff[2].phy_address = (unsigned long )NULL;

error2:
	free_pages(img_buff[1].virt_address,order);
	img_buff[1].phy_address = (unsigned long )NULL;
	
error1:
	free_pages(img_buff[0].virt_address,order);
    img_buff[0].phy_address = (unsigned long )NULL;

error:
	return -ENOMEM;

}

/* 启动传输
 * 参考: uvc_video_enable(video, 1):
 *           uvc_commit_video
 *           uvc_init_video
 */
static int cmos_ov7740_vidioc_streamon ( struct file* file, void* priv, enum v4l2_buf_type i )
{
	*CISRCFMT |= ( 0<<30 ) | ( 0<<29 ) | ( CAM_SRC_XRES<<16 ) | ( CAM_ORDER_CbYCrY<<14 ) | ( CAM_SRC_YRES<<0 );

	//清除溢出标志位
	*CIWDOFST |= ( 1<<30 ) | ( 0xf<<12 );
	//使能窗口功能 水平 垂直裁剪大小
	*CIWDOFST |= ( 1<<31 ) | ( <<16 ) | ( <<0 );
	SRC_Width = CAM_SRC_XRES - 2*CAM_XRES_OFFSET;
	SRC_Height= CAM_SRC_YRES - 2*CAM_YRES_OFFSET;

	/*
	CIGCTRL:
		bit[31] 	-- 软件复位CAMIF控制器
		bit[30] 	-- 用于复位外部摄像头模块
		bit[29] 	-- 保留位，必须设置为1
		bit[28:27]	-- 用于选择信号源(00 = 输入源来自摄像头模块)
		bit[26] 	-- 设置像素时钟的极性(猜0)
		bit[25] 	-- 设置VSYNC的极性(0)
		bit[24] 	-- 设置HREF的极性(0)
	*/
	*CIGCTRL  |= ( 1<<29 ) | ( 0<<27 ) | ( 0<<26 ) | ( 0<<25 ) | ( 0<<24 );
	/*
	CIPRCTRL:
		bit[23:19] -- 主突发长度(Main_burst)
		bit[18:14] -- 剩余突发长度(Remained_burst)
		bit[2]	  -- 是否使能LastIRQ功能(不使能)
	*/
    *CIPRCTRL = ;
    
	/*
	CIPRSCPRERATIO:
		bit[31:28]: 预览缩放的变化系数(SHfactor_Pr)
		bit[22:16]: 预览缩放的水平比(PreHorRatio_Pr)
		bit[6:0]: 预览缩放的垂直比(PreVerRatio_Pr)

	CIPRSCPREDST:
		bit[27:16]: 预览缩放的目标宽度(PreDstWidth_Pr)
		bit[11:0]: 预览缩放的目标高度(PreDstHeight_Pr)

	CIPRSCCTRL:
		bit[29:28]: 告诉摄像头控制器(图片是缩小、放大)(ScaleUpDown_Pr)
		bit[24:16]: 预览主缩放的水平比(MainHorRatio_Pr)
		bit[8:0]: 预览主缩放的垂直比(MainVerRatio_Pr)

		bit[31]: 必须固定设置为1
		bit[30]: 设置图像输出格式是RGB16、RGB24
		bit[15]: 预览缩放开始
	*/



	//开始预览缩放
	*CIPRSCCTRL |= ( 1<<15 );
	//使能摄像头控制器 启动预览通道
	*CIIMGCPT |= ( ( 1<<31 ) | ( 1<<29 ) );


	return 0;
}

/* 停止传输
 * 参考 : uvc_video_enable(video, 0)
 */
static int cmos_ov7740_vidioc_streamoff ( struct file* file, void* priv, enum v4l2_buf_type t )
{
	//停止预览缩放
	*CIPRSCCTRL |= ( 0<<15 );
	//不使能摄像头控制器 不启动预览通道
	*CIIMGCPT |= ( ( 0<<31 ) | ( 0<<29 ) );


	return 0;
}


static const struct v4l2_ioctl_ops cmos_ov7740_ioctl_ops =
{
	// 表示它是一个摄像头设备
	.vidioc_querycap      = cmos_ov7740_vidioc_querycap,

	/* 用于列举、获得、测试、设置摄像头的数据的格式 */
	.vidioc_enum_fmt_vid_cap  = cmos_ov7740_vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap     = cmos_ov7740_vidioc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap   = cmos_ov7740_vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap     = cmos_ov7740_vidioc_s_fmt_vid_cap,

	/* 缓冲区操作: 申请/查询/放入队列/取出队列 */
	.vidioc_reqbufs       = cmos_ov7740_vidioc_reqbufs,

	/* 说明: 因为我们是通过读的方式来获得摄像头数据,因此查询/放入队列/取出队列这些操作函数将不在需要 */
#if 0
	.vidioc_querybuf      = myuvc_vidioc_querybuf,
	.vidioc_qbuf          = myuvc_vidioc_qbuf,
	.vidioc_dqbuf         = myuvc_vidioc_dqbuf,
#endif

	// 启动/停止
	.vidioc_streamon      = cmos_ov7740_vidioc_streamon,
	.vidioc_streamoff     = cmos_ov7740_vidioc_streamoff,
};



/* 打开摄像头设备 */
static int cmos_ov7740_open ( struct file* file )
{
	return 0;
}

/* 关闭摄像头设备 */
static int cmos_ov7740_close ( struct file* file )
{

	return 0;
}

/* 应用程序通过读的方式读取摄像头的数据 */
static ssize_t cmos_ov7740_read ( struct file* filep, char __user* buf, size_t count, loff_t* pos )
{
	return 0;
}


/* 3.1. 分配、设置一个v4l2_file_operations结构体
 * 成员为video_device设备的操作函数
 */
static const struct v4l2_file_operations cmos_ov7740_fops =
{
	.owner			= THIS_MODULE,
	.open       		= cmos_ov7740_open,
	.release    		= cmos_ov7740_close,
	.unlocked_ioctl     = video_ioctl2,
	.read			= cmos_ov7740_read,
};


/* 2.1. 分配、设置一个video_device结构体
 *
 */
static struct video_device cmos_ov7740_vdev =
{
	.fops		= &cmos_ov7740_fops,
	.ioctl_ops		= &cmos_ov7740_ioctl_ops,
	.release		= cmos_ov7740_release,
	.name		= "cmos_ov7740",
};

static void cmos_ov7740_gpio_cfg ( void )
{

	/* 设置相应的GPIO用于CAMIF */
	*GPJCON = 0x2aaaaaa;
	*GPJDAT = 0;


	/* 使能上拉电阻 */
	*GPJUP = 0;
}

static void cmos_ov7740_camif_reset ( void )
{
	/* 传输方式为BT601 */
	*CISRCFMT |= ( 1<<31 );


	/* 复位CAMIF控制器 */
	*CIGCTRL  |= ( 1<<31 );
	mdelay ( 10 );
	*CIGCTRL  &=~ ( 1<<31 );
	mdelay ( 10 );

}

static void cmos_ov7740_clk_cfg ( void )
{
	struct clk* camif_clk;
	struct clk* camif_upll_clk;


	/* 使能CAMIF的时钟源 */
	camif_clk = clk_get ( NULL, "camif" );
	if ( !camif_clk || IS_ERR ( camif_clk ) )
	{
		printk ( KERN_INFO"failed to get camif clock\n" );
	}
	clk_enable ( camif_clk );

	/* 使能并设置CAMCLK = 24MHz */
	camif_upll_clk = clk_get ( NULL, "camif-upll" );
	if ( !camif_upll_clk || IS_ERR ( camif_upll_clk ) )
	{
		printk ( KERN_INFO"failed to get camif-upll clock\n" );
	}
	clk_set_rate ( camif_upll_clk, 24000000 ); //设置频率为24Mhz
	mdelay ( 100 );


}

static void cmos_ov7740_reset ( void )
{

	//复位需要的电平顺序为 1-0-1
	*CIGCTRL |= ( 1<<30 );
	mdelay ( 30 );
	*CIGCTRL &=~ ( 1<<30 );
	mdelay ( 30 );
	*CIGCTRL |= ( 1<<30 );
	mdelay ( 30 );

}

static void cmos_ov7740_init ( void )
{
	unsigned int mid;
	int i;

	//读取摄像头的设备id
	mid = i2c_smbus_read_byte_data ( cmos_ov7740_client, 0x0a ) <<8;
	mid = i2c_smbus_read_byte_data ( cmos_ov7740_client, 0x0b );


	/* 使用厂家提供的初始化数组初始化摄像头寄存器

	*/
	for ( i=0; i<OV7740_INIT_REGS_SIZE; i++ )
	{
		i2c_smbus_write_byte_data ( cmos_ov7740_client, ov7740_setting_30fps_VGA_640_480[i].regaddr, ov7740_setting_30fps_VGA_640_480[i].value  );
		mdelay ( 2 );

	}


}


static irqreturn_t cmos_ov7740_camif_irq_c ( int irq, void* dev_id )
{

	return IRQ_HANDLED;

}


static irqreturn_t cmos_ov7740_camif_irq_p ( int irq, void* dev_id )
{

	return IRQ_HANDLED;

}



//当drv name 与dev name相同时会执行probe函数
//需要在probe函数中注册video设备
static int __devinit cmos_ov7740_probe ( struct i2c_client* client, const struct i2c_device_id* id )
{

	printk ( "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__ );
	/* 2.3 硬件相关 */
	/* 2.3.1 映射相应的寄存器 */
	GPJCON = ioremap ( 0x560000d0, 4 );
	GPJDAT = ioremap ( 0x560000d4, 4 );
	GPJUP = ioremap ( 0x560000d8, 4 );

	CISRCFMT = ioremap ( 0x4F000000, 4 );
	CIWDOFST = ioremap ( 0x4F000004, 4 );
	CIGCTRL = ioremap ( 0x4F000008, 4 );
	CIPRCLRSA1 = ioremap ( 0x4F00006C, 4 );
	CIPRCLRSA2 = ioremap ( 0x4F000070, 4 );
	CIPRCLRSA3 = ioremap ( 0x4F000074, 4 );
	CIPRCLRSA4 = ioremap ( 0x4F000078, 4 );
	CIPRTRGFMT = ioremap ( 0x4F00007C, 4 );
	CIPRCTRL = ioremap ( 0x4F000080, 4 );
	CIPRSCPRERATIO = ioremap ( 0x4F000084, 4 );
	CIPRSCPREDST = ioremap ( 0x4F000088, 4 );
	CIPRSCCTRL = ioremap ( 0x4F00008C, 4 );
	CIPRTAREA = ioremap ( 0x4F000090, 4 );
	CIIMGCPT = ioremap ( 0x4F0000A0, 4 );

	SRCPND = ioremap ( 0X4A000000, 4 );
	INTPND = ioremap ( 0X4A000010, 4 );
	SUBSRCPND = ioremap ( 0X4A000018, 4 );

	/* 2.3.2 设置相应的GPIO用于CAMIF */
	cmos_ov7740_gpio_cfg();
	/* 2.3.3 复位一下CAMIF控制器 */
	cmos_ov7740_camif_reset();

	/* 2.3.4 设置、使能时钟(使能HCLK、使能并设置CAMCLK = 24MHz) */
	cmos_ov7740_clk_cfg();

	/* 2.3.5 复位一下摄像头模块 */
	cmos_ov7740_reset();


	/* 2.3.6 通过IIC总线,初始化摄像头模块 */
	cmos_ov7740_client = client;
	cmos_ov7740_init();


	/* 2.3.7 注册中断 */
	if ( request_irq ( IRQ_S3C2440_CAM_C, cmos_ov7740_camif_irq_c, IRQF_DISABLED, "CAM_C", NULL ) )
	{
		printk ( "[%s]:%s request irq fail ",__func__,__LINE__ );
	}

	if ( request_irq ( IRQ_S3C2440_CAM_P, cmos_ov7740_camif_irq_p, IRQF_DISABLED, "CAM_P", NULL ) )
	{
		printk ( "[%s]:%s request irq fail ",__func__,__LINE__ );
	}

	/* 2.2.注册设备 */
	if ( video_register_device ( &cmos_ov7740_vdev, VFL_TYPE_GRABBER, -1 ) ) //VFL_TYPE_GRABBER图像采集设备
	{
		printk ( "[%s]:%s register device fail ",__func__,__LINE__ );
	}

	return 0;

}

//当rmmod此设备驱动时会执行此函数
//remove 函数中移除video设备
static int __devexit cmos_ov7740_remove ( struct i2c_client* client )
{
	printk ( "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__ );

	iounmap ( GPJCON );
	iounmap ( GPJDAT );
	iounmap ( GPJUP );

	iounmap ( CISRCFMT );
	iounmap ( CIWDOFST );
	iounmap ( CIGCTRL );
	iounmap ( CIPRCLRSA1 );
	iounmap ( CIPRCLRSA2 );
	iounmap ( CIPRCLRSA3 );
	iounmap ( CIPRCLRSA4 );
	iounmap ( CIPRTRGFMT );
	iounmap ( CIPRCTRL );
	iounmap ( CIPRSCPRERATIO );
	iounmap ( CIPRSCPREDST );
	iounmap ( CIPRSCCTRL );
	iounmap ( CIPRTAREA );
	iounmap ( CIIMGCPT );

	iounmap ( SRCPND );
	iounmap ( INTPND );
	iounmap ( SUBSRCPND );

	video_unregister_device ( &cmos_ov7740_vdev );
	return 0;

}


static const struct i2c_device_id cmos_ov7740_id_table[] =
{
	{ "cmos_ov7740", 0 },
	{}
};


/* 1.1. 分配、设置一个i2c_driver */
static struct i2c_driver cmos_ov7740_driver =
{
	.driver = {
		.name	= "cmos_ov7740",
		.owner	= THIS_MODULE,
	},
	.probe		= cmos_ov7740_probe,
	.remove 	= __devexit_p ( cmos_ov7740_remove ),
	.id_table	= cmos_ov7740_id_table,
};

static int cmos_ov7740_drv_init ( void )
{

	/* 1.2.注册驱动 */
	i2c_add_driver ( &cmos_ov7740_driver );

}


static void cmos_ov7740_drv_exit ( void )
{
	//退出函数里删除驱动
	i2c_del_driver ( &cmos_ov7740_driver );
}

module_init ( cmos_ov7740_drv_init );
module_exit ( cmos_ov7740_drv_exit );

MODULE_LICENSE ( "GPL" );

