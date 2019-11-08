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

#include <linux/clk.h>
#include <asm/io.h>

// CAMIF GPIO
static unsigned long* GPJCON;
static unsigned long* GPJDAT;
static unsigned long* GPJUP;

// CAMIF
/*
CISRCFMT:
	bit[31]	-- ѡ���䷽ʽΪBT601����BT656(1-BT601)
	bit[30]	-- ����ƫ��ֵ(0 = +0 (���������) - for YCbCr)
	bit[29]	-- ����λ,��������Ϊ0
	bit[28:16]	-- ����ԴͼƬ��ˮƽ����ֵ(640)
	bit[15:14]	-- ����ԴͼƬ����ɫ˳��(�洢��ʽ)(02 = CbYCrY )
	bit[12:0]	-- ����ԴͼƬ�Ĵ�ֱ����ֵ(480)
*/
static unsigned long* CISRCFMT;

/*
CIWDOFST:
	bit[31] 	  -- 1 = ʹ�ܴ��ڹ��ܡ�0 = ��ʹ�ô��ڹ��ܣ��˴�ѡ1��
	bit[30��15:12]-- ��������־λ
	bit[26:16]	  -- ˮƽ����Ĳü��Ĵ�С
	bit[10:0]	  -- ��ֱ����Ĳü��Ĵ�С
*/
static unsigned long* CIWDOFST;

/*
CIGCTRL:
	bit[31] 	-- ������λCAMIF������
	bit[30] 	-- ���ڸ�λ�ⲿ����ͷģ��
	bit[29] 	-- ����λ����������Ϊ1
	bit[28:27]	-- ����ѡ���ź�Դ(00 = ����Դ��������ͷģ��)
	bit[26] 	-- �����Ƿ�ת����ʱ�ӵļ���(��0)
	bit[25] 	-- �����Ƿ�תVSYNC�ļ���(0)
	bit[24] 	-- �����Ƿ�תHREF�ļ���(0)
*/
static unsigned long* CIGCTRL;

//�����Դ�������ʼ��ַ ����DMA
static unsigned long* CIPRCLRSA1;
static unsigned long* CIPRCLRSA2;
static unsigned long* CIPRCLRSA3;
static unsigned long* CIPRCLRSA4;
/*
CIPRTRGFMT:
	bit[28:16] -- ��ʾĿ��ͼƬ��ˮƽ���ش�С(TargetHsize_Pr)
	bit[15:14] -- �Ƿ���ת��00-����ת��
	bit[12:0]	 -- ��ʾĿ��ͼƬ�Ĵ�ֱ���ش�С(TargetVsize_Pr)
*/
static unsigned long* CIPRTRGFMT;
/*
CIPRCTRL:
	bit[23:19] -- ��ͻ������(Main_burst)
	bit[18:14] -- ʣ��ͻ������(Remained_burst)
	bit[2]	  -- �Ƿ�ʹ��LastIRQ����(��ʹ��)
*/
static unsigned long* CIPRCTRL;
/*
CIPRSCPRERATIO:
	bit[31:28]: Ԥ�����ŵı仯ϵ��(SHfactor_Pr)
	bit[22:16]: Ԥ�����ŵ�ˮƽ��(PreHorRatio_Pr)
	bit[6:0]: Ԥ�����ŵĴ�ֱ��(PreVerRatio_Pr)

CIPRSCPREDST:
	bit[27:16]: Ԥ�����ŵ�Ŀ�����(PreDstWidth_Pr)
	bit[11:0]: Ԥ�����ŵ�Ŀ��߶�(PreDstHeight_Pr)

CIPRSCCTRL:
	bit[29:28]: ��������ͷ������(ͼƬ����С���Ŵ�)(ScaleUpDown_Pr)
	bit[24:16]: Ԥ�������ŵ�ˮƽ��(MainHorRatio_Pr)
	bit[8:0]: Ԥ�������ŵĴ�ֱ��(MainVerRatio_Pr)

	bit[31]: ����̶�����Ϊ1
	bit[30]: ����ͼ�������ʽ��0-RGB16��1-RGB24 
	bit[15]: Ԥ�����ſ�ʼ
*/
static unsigned long* CIPRSCPRERATIO;
static unsigned long* CIPRSCPREDST;
static unsigned long* CIPRSCCTRL;
/*
CIPRTAREA:
	��ʾԤ��ͨ����Ŀ�������С ����������DMA���� ֵΪĿ�����*Ŀ��߶�
*/
static unsigned long* CIPRTAREA;
/*
CIIMGCPT:
	bit[31]: ����ʹ������ͷ������
	bit[30]: ʹ�ܱ���ͨ��
	bit[29]: ʹ��Ԥ��ͨ��
*/
static unsigned long* CIIMGCPT;

// IRQ
static unsigned long* SRCPND;
static unsigned long* INTPND;
static unsigned long* SUBSRCPND;

//iic���߲���
static struct i2c_client* cmos_ov7740_client;


static unsigned int SRC_Width, SRC_Height;//Դ���ݷֱ���
static unsigned int TargetXres, TargetYres;//Ŀ��ͼƬ�ֱ���


static unsigned long buf_size;//һ֡�����ܴ�С
static unsigned int  bytesperline; //һ����ռ���ֽ���
static unsigned int Main_burst, Remained_burst; // ��ͻ������  ��ͻ������

//���߶���ͷ
static DECLARE_WAIT_QUEUE_HEAD ( cam_wait_queue );
/* �жϱ�־ */
static volatile int ev_cam = 0;

typedef struct cmos_ov7740_i2c_value
{
	unsigned char regaddr;
	unsigned char value;
} ov7740_t ;

/* init: 640x480,30fps��,YUV422�����ʽ */
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

struct cmos_ov7740_fmt
{
	char*  name;
	u32   fourcc;          /* v4l2 format id */
	int   depth;
};

//�оٴ�����֧�ֵ����ظ�ʽ
static struct cmos_ov7740_fmt formats[] =
{
	{
		.name     = "RGB565",
		.fourcc   = V4L2_PIX_FMT_RGB565,
		.depth    = 16,
	},
	{
		.name     = "PACKED_RGB_888",
		.fourcc   = V4L2_PIX_FMT_RGB24,
		.depth    = 24,
	},
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

struct cmos_ov7740_scaler
{
	unsigned int PreHorRatio;//ˮƽ���
	unsigned int PreVerRatio;//��ֱ���
	unsigned int H_Shift;//ˮƽ��
	unsigned int V_Shift;//��ֱ��
	unsigned int PreDstWidth;//Ŀ�����
	unsigned int PreDstHeight;//Ŀ��߶�
	unsigned int MainHorRatio;//Ԥ�������ŵ�ˮƽ��
	unsigned int MainVerRatio;//Ԥ�������ŵĴ�ֱ��
	unsigned int SHfactor;//���ű��
	unsigned int ScaleUpDown;//�Ŵ���С��־
};

struct cmos_ov7740_scaler sc;


#define OV7740_INIT_REGS_SIZE  sizeof(ov7740_setting_30fps_VGA_640_480)/sizeof(ov7740_setting_30fps_VGA_640_480[0])
#define CAM_SRC_XRES (640)
#define CAM_SRC_YRES (480)
#define CAM_XRES_OFFSET (0)
#define CAM_YRES_OFFSET (0)

#define CAM_ORDER_YCbYCr (0)
#define CAM_ORDER_YCrYCb (1)
#define CAM_ORDER_CbYCrY (2)
#define CAM_ORDER_CrYCbY (3)



static irqreturn_t cmos_ov7740_camif_irq_c ( int irq, void* dev_id )
{

	return IRQ_HANDLED;

}

//Ԥ��ͨ���жϺ���
static irqreturn_t cmos_ov7740_camif_irq_p ( int irq, void* dev_id )
{
	/* ���ж� */
	*SRCPND = 1<<6; //ʹ��INT_CAM�ж�
	*INTPND = 1<<6; //ʹ��INT_CAM�ж�
	*SUBSRCPND = 1<<12; //ʹ��Ԥ��ͨ���ж�

	ev_cam = 1;

	wake_up_interruptible ( &cam_wait_queue );

	return IRQ_HANDLED;

}


/*
  ��ѯ�豸���ͺͶ�ȡ��IO��ʽ
  �ο� uvc_v4l2_do_ioctl
*/
static int cmos_ov7740_vidioc_querycap ( struct file* file, void*  priv, struct v4l2_capability* cap )
{
	memset ( cap,0,sizeof ( struct v4l2_capability ) );
	strcpy ( cap->driver,"cmos_ov7740" );
	strcpy ( cap->card,"cmos_ov7740" );
	cap->version = 2;

	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_READWRITE;

	return 0;
}

/* �о�֧����Щ���ݸ�ʽ
 * �ο�: uvc_fmts ����
 */
static int cmos_ov7740_vidioc_enum_fmt_vid_cap ( struct file* file, void*  priv,struct v4l2_fmtdesc* f )
{
	struct cmos_ov7740_fmt* fmt;

	if ( f->index >= ARRAY_SIZE ( formats ) )
	{
		return -EINVAL;
	}

	fmt = &formats[f->index];

	strlcpy ( f->description,fmt->name,sizeof ( f->description ) );
	f->pixelformat = fmt->fourcc;


	return 0;
}

/* ���ص�ǰ��ʹ�õĸ�ʽ */
static int cmos_ov7740_vidioc_g_fmt_vid_cap ( struct file* file, void* priv,struct v4l2_format* f )
{
	return 0;
}

/* �������������Ƿ�֧��ĳ�����ظ�ʽ
 * �ο�: uvc_v4l2_try_format
 *       myvivi_vidioc_try_fmt_vid_cap
 */
static int cmos_ov7740_vidioc_try_fmt_vid_cap ( struct file* file, void* priv,struct v4l2_format* f )
{
	/* �ж��Ƿ�Ϊ����ͷ�豸 */
	if ( f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE )
	{
		return -EINVAL;
	}

	/* ��coms�е�CICOSCCTRL�Ĵ����п�֪����Ԥ��ͨ��ֻ֧��RGB16/RGB24
	*/
	if ( ( f->fmt.pix.pixelformat != V4L2_PIX_FMT_RGB565 ) && ( f->fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24 ) )
	{
		return -EINVAL;
	}

	return 0;
}

/*
 ����ͼ��ֱ���,�������ظ�ʽ

�ο� myvivi_vidioc_s_fmt_vid_cap
*/
static int cmos_ov7740_vidioc_s_fmt_vid_cap ( struct file* file, void* priv,struct v4l2_format* f )
{
	//���Ȳ������������Ƿ�֧��ĳ�����ظ�ʽ
	int ret = cmos_ov7740_vidioc_try_fmt_vid_cap ( file, NULL, f );
	if ( ret < 0 )
	{
		return ret;
	}

	//ȡ�����õ�x y�ֱ���
	TargetXres = f->fmt.pix.width;
	TargetYres = f->fmt.pix.height;

	//���������������ظ�ʽ��RGB565
	if ( f->fmt.pix.pixelformat == V4L2_PIX_FMT_RGB565 )
	{

		*CIPRSCCTRL |= ( 0<<30 );
		//����һ����ռ���ֽ��� �����ռ���ռ���ֽ��� bpp����Ϊ16
		f->fmt.pix.bytesperline = ( f->fmt.pix.width * 16 ) >> 3;
		f->fmt.pix.sizeimage = f->fmt.pix.height * f->fmt.pix.bytesperline;

		//��¼һ����ռ�ֽ����ܴ�С
		bytesperline = f->fmt.pix.bytesperline;
		buf_size = f->fmt.pix.sizeimage;
	}
	else if ( f->fmt.pix.pixelformat == V4L2_PIX_FMT_RGB24 )
	{

		*CIPRSCCTRL |= ( 1<<30 );
		//����һ����ռ���ֽ��� �����ռ���ռ���ֽ��� bpp����Ϊ32
		f->fmt.pix.bytesperline = ( f->fmt.pix.width * 32 ) >> 3;
		f->fmt.pix.sizeimage = f->fmt.pix.height * f->fmt.pix.bytesperline;

		//��¼һ����ռ�ֽ����ܴ�С
		bytesperline = f->fmt.pix.bytesperline;
		buf_size = f->fmt.pix.sizeimage;

	}

	/*
	CIPRTRGFMT:
		bit[28:16] -- ��ʾĿ��ͼƬ��ˮƽ���ش�С(TargetXres)
		bit[15:14] -- �Ƿ���ת��������������Ͳ�ѡ����(0)
		bit[12:0]	 -- ��ʾĿ��ͼƬ�Ĵ�ֱ���ش�С(TargetYres)
	*/
	*CIPRTRGFMT = ( TargetXres<<16 ) | ( 0<<14 ) | ( TargetYres<<0 );
	return 0;
}

//���뻺����
static int cmos_ov7740_vidioc_reqbufs ( struct file* file, void* priv, struct v4l2_requestbuffers* p )
{
	unsigned int order;

	order = get_order ( buf_size ); //����buf_size��Ҫ��orderֵ�Ĵ�С(���ֽڼ���)

	/************************************************/

	img_buff[0].order = order;

	//����ռ� ����DMA���ڴ棬����˯�� GFP_DMA | GFP_KERNEL
	img_buff[0].virt_address =  __get_free_pages ( GFP_KERNEL|__GFP_DMA,order );
	if ( !img_buff[0].virt_address )
	{
		printk ( "[%s]:%d __get_free_pages error ",__FILE__,__LINE__ );
		goto error0;
	}
	img_buff[0].phy_address = __virt_to_phys ( img_buff[0].virt_address );

	/************************************************/
	img_buff[1].order = order;

	//����ռ� ����DMA���ڴ棬����˯�� GFP_DMA | GFP_KERNEL
	img_buff[1].virt_address =  __get_free_pages ( GFP_KERNEL|__GFP_DMA,order );
	if ( !img_buff[1].virt_address )
	{
		printk ( "[%s]:%d __get_free_pages error ",__FILE__,__LINE__ );
		goto error1;
	}
	img_buff[1].phy_address = __virt_to_phys ( img_buff[1].virt_address );

	/************************************************/
	img_buff[2].order = order;

	//����ռ� ����DMA���ڴ棬����˯�� GFP_DMA | GFP_KERNEL
	img_buff[2].virt_address =  __get_free_pages ( GFP_KERNEL|__GFP_DMA,order );
	if ( !img_buff[2].virt_address )
	{
		printk ( "[%s]:%d __get_free_pages error ",__FILE__,__LINE__ );
		goto error2;
	}
	img_buff[2].phy_address = __virt_to_phys ( img_buff[2].virt_address );

	/************************************************/
	img_buff[3].order = order;

	//����ռ� ����DMA���ڴ棬����˯�� GFP_DMA | GFP_KERNEL
	img_buff[3].virt_address =  __get_free_pages ( GFP_KERNEL|__GFP_DMA,order );
	if ( !img_buff[3].virt_address )
	{
		printk ( "[%s]:%d __get_free_pages error ",__FILE__,__LINE__ );
		goto error3;
	}
	img_buff[3].phy_address = __virt_to_phys ( img_buff[3].virt_address );
	/************************************************/
	*CIPRCLRSA1 = img_buff[0].phy_address;
	*CIPRCLRSA2 = img_buff[1].phy_address;
	*CIPRCLRSA3 = img_buff[2].phy_address;
	*CIPRCLRSA4 = img_buff[3].phy_address;
	return 0;


error3:
	free_pages ( img_buff[2].virt_address,order );
	img_buff[2].phy_address = ( unsigned long ) NULL;

error2:
	free_pages ( img_buff[1].virt_address,order );
	img_buff[1].phy_address = ( unsigned long ) NULL;

error1:
	free_pages ( img_buff[0].virt_address,order );
	img_buff[0].phy_address = ( unsigned long ) NULL;

error0:
	return -ENOMEM;

}


/* ͻ�����ȵļ�����2440�û��ֲ��532ҳ */
static void CalculateBurstSize ( unsigned int hSize, unsigned int* mainBusrtSize, unsigned int* remainedBustSize )
{
	unsigned int tmp;
	/*
	  ��оƬ�ֲ��֪�����е�ͻ������ֻ��Ϊ2,4,8,16���������Ǿʹ�����16��ʼ����
	 ��оƬ�ֲ��֪��ÿһ�е����ݱ�����Ϊ��λ��
	*/

	tmp = ( hSize /4 ) %16;

	switch ( tmp )
	{
		case 0:
			*mainBusrtSize = 16;
			*remainedBustSize = 16;
			break;
		case 4:

			*mainBusrtSize = 16;
			*remainedBustSize = 4;
			break;
		case 8:

			*mainBusrtSize = 16;
			*remainedBustSize = 8;
			break;

		default:
			/* ��ͻ�����Ȳ���Ϊ16�����Կ�ʼ������ͻ������Ϊ8 */

			tmp = ( hSize /4 ) %8;
			switch ( tmp )
			{
				case 0:

					*mainBusrtSize = 8;
					*remainedBustSize = 8;
					break;
				case 4:

					*mainBusrtSize = 8;
					*remainedBustSize = 4;
					break;


				default:
					/* ��ͻ�����Ȳ���Ϊ8����ʼ������ͻ������Ϊ4 */
					*mainBusrtSize = 4;

					tmp = ( hSize /4 ) %4;
					*remainedBustSize = ( tmp ) ?tmp:4; //�����Ȼ������ ������������ͻ������


					break;
			}

			break;
	}




}

/* �ճ�jz2440�û��ֲ��527ҳ���� */

static void camif_get_scaler_factor ( unsigned int src,unsigned int dst,unsigned int* ratio,unsigned int* shift )
{
	if ( src >= 64*dst )
	{
		return ;
	}
	else if ( src >= 32*dst )
	{
		*ratio = 32;
		*shift = 5;
	}
	else if ( src >= 16*dst )
	{
		*ratio = 16;
		*shift = 4;
	}
	else if ( src >= 8*dst )
	{
		*ratio = 8;
		*shift = 3;
	}
	else if ( src >= 4*dst )
	{
		*ratio = 4;
		*shift = 2;
	}
	else if ( src >= 2*dst )
	{
		*ratio = 2;
		*shift = 1;
	}
	else
	{
		*ratio = 1;
		*shift = 0;
	}

}


static void cmos_ov7740_calculate_scaler_info ( void )
{

	/* �����Դ���Ⱥ�Դ�߶��Ǿ������ڼ��е�ͼ������
	 * �����Ŀ����Ⱥ͸߶���Ӧ�ó�����cmos_ov7740_vidioc_s_fmt_vid_cap����
	 * �������ݸ�ʽʱ����ģ���ʾӦ�ó�����Ҫ��õ�ͼ��ֱ���
	 */
	//Դ���ݸ߶� ���� Ŀ�����ݸ߶� ����
	unsigned int src_h,src_w,tar_h,tar_w;

	src_h = SRC_Height;
	src_w = SRC_Width;
	tar_h = TargetYres;
	tar_w = TargetXres;

	printk ( "%s: SRC_in(%d, %d), Target_out(%d, %d)\n", __func__, src_w, src_h, tar_w, tar_h );

	//����Դ������Ŀ�����ݼ��� ˮƽ��ֱ����ı���� ˮƽ��
	camif_get_scaler_factor ( src_h,tar_h,&sc.PreVerRatio,&sc.V_Shift );
	camif_get_scaler_factor ( src_w,tar_w,&sc.PreHorRatio,&sc.H_Shift );

	sc.PreDstWidth = src_w / sc.PreHorRatio;
	sc.PreDstHeight = src_h / sc.PreVerRatio;

	sc.MainHorRatio = ( src_w << 8 ) / ( tar_w << sc.H_Shift );
	sc.MainVerRatio = ( src_h << 8 ) / ( tar_h << sc.V_Shift );

	sc.SHfactor = 10 - ( sc.H_Shift + sc.V_Shift );

	sc.ScaleUpDown = ( tar_w>=src_w ) ?1:0;
}


/* ��������
 * �ο�: uvc_video_enable(video, 1):
 *           uvc_commit_video
 *           uvc_init_video
 */
static int cmos_ov7740_vidioc_streamon ( struct file* file, void* priv, enum v4l2_buf_type i )
{
	/*
	CISRCFMT:
		bit[31]	-- ѡ���䷽ʽΪBT601����BT656(1-BT601)
		bit[30]	-- ����ƫ��ֵ(0 = +0 (���������) - for YCbCr)
		bit[29]	-- ����λ,��������Ϊ0
		bit[28:16]	-- ����ԴͼƬ��ˮƽ����ֵ(640)
		bit[15:14]	-- ����ԴͼƬ����ɫ˳��(�洢��ʽ)(02 = CbYCrY )
		bit[12:0]	-- ����ԴͼƬ�Ĵ�ֱ����ֵ(480)
	*/
	*CISRCFMT |= ( 0<<30 ) | ( 0<<29 ) | ( CAM_SRC_XRES<<16 ) | ( CAM_ORDER_CbYCrY<<14 ) | ( CAM_SRC_YRES<<0 );

	/*
	CIWDOFST:
		bit[31]		   -- 1 = ʹ�ܴ��ڹ��ܡ�0 = ��ʹ�ô��ڹ���
		bit[30��15:12] -- ��������־λ
		bit[26:16]	   -- ˮƽ����Ĳü��Ĵ�С
		bit[10:0]	   -- ��ֱ����Ĳü��Ĵ�С
	*/
	*CIWDOFST |= ( 1<<30 ) | ( 0xf<<12 );
	*CIWDOFST |= ( 1<<31 ) | ( CAM_XRES_OFFSET<<16 ) | ( CAM_YRES_OFFSET<<0 ); 	//ʹ�ܴ��ڹ��� ˮƽ ��ֱ�ü���С
	SRC_Width = CAM_SRC_XRES - 2*CAM_XRES_OFFSET;
	SRC_Height= CAM_SRC_YRES - 2*CAM_YRES_OFFSET;

	/*
	CIGCTRL:
		bit[31] 	-- ������λCAMIF������
		bit[30] 	-- ���ڸ�λ�ⲿ����ͷģ��
		bit[29] 	-- ����λ����������Ϊ1
		bit[28:27]	-- ����ѡ���ź�Դ(00 = ����Դ��������ͷģ��)
		bit[26] 	-- ��������ʱ�ӵļ���(��0)
		bit[25] 	-- ����VSYNC�ļ���(0)
		bit[24] 	-- ����HREF�ļ���(0)
	*/
	*CIGCTRL  |= ( 1<<29 ) | ( 0<<27 ) | ( 0<<26 ) | ( 0<<25 ) | ( 0<<24 );

	/*
	CIPRCTRL:
		bit[23:19] -- ��ͻ������(Main_burst)
		bit[18:14] -- ʣ��ͻ������(Remained_burst)
		bit[2]	  --  �Ƿ�ʹ��������ɴ���֮����ж�LastIRQ(��ʹ��)

		48�ֽ� = 10�ֽ� + 10�ֽ� + 10�ֽ� +��10�ֽڡ��������ֽ�
		���������Ҫ����48�ֽ����ݣ�����DMA�޷�һ���Դ���48�ֽڣ����Ǿ�Ҫ��������
	    ��ÿ��10�ֽڷ�����ʱ���10�ֽھͳ�Ϊ��ͻ������ ��
		�����������ӣ�48�ֽڰ�ÿ10�ֽڷ�����Ϻ����һ��8�ֽھ���ʣ��ͻ�����ȡ�
	*/
	CalculateBurstSize ( bytesperline, &Main_burst, &Remained_burst );
	*CIPRCTRL = ( Main_burst<<19 ) | ( Remained_burst<<14 ) | ( 0<<2 );

	/*
	CIPRSCPRERATIO:
		bit[31:28]: Ԥ�����ŵı仯ϵ��(SHfactor_Pr)
		bit[22:16]: Ԥ�����ŵ�ˮƽ��(PreHorRatio_Pr)
		bit[6:0]: Ԥ�����ŵĴ�ֱ��(PreVerRatio_Pr)

	CIPRSCPREDST:
		bit[27:16]: Ԥ�����ŵ�Ŀ�����(PreDstWidth_Pr)
		bit[11:0]: Ԥ�����ŵ�Ŀ��߶�(PreDstHeight_Pr)

	CIPRSCCTRL:
		bit[29:28]: ��������ͷ������(ͼƬ����С���Ŵ�)(ScaleUpDown_Pr)
		bit[24:16]: Ԥ�������ŵ�ˮƽ��(MainHorRatio_Pr)
		bit[8:0]: Ԥ�������ŵĴ�ֱ��(MainVerRatio_Pr)

		bit[31]: ����̶�����Ϊ1
		bit[30]: ����ͼ�������ʽ��RGB16��RGB24
		bit[15]: Ԥ�����ſ�ʼ
	*/
	cmos_ov7740_calculate_scaler_info();
	*CIPRSCPRERATIO = ( sc.SHfactor << 28 ) | ( sc.PreHorRatio<<16 ) | ( sc.PreVerRatio<<0 );
	*CIPRSCPREDST = ( sc.PreDstWidth<<16 ) | ( sc.PreDstHeight<<0 );
	*CIPRSCCTRL |= ( 1<<31 ) | ( sc.ScaleUpDown<<28 ) | ( sc.MainHorRatio<<16 ) | ( sc.MainVerRatio<<0 );

	/*
	CIPRTAREA:
		��ʾԤ��ͨ����Ŀ�������С ����������DMA���� ֵΪĿ�����*Ŀ��߶�
	*/
	*CIPRTAREA =TargetXres * TargetYres;

	/*
	CIIMGCPT:
		bit[31]: ����ʹ������ͷ������
		bit[30]: ʹ�ܱ���ͨ�� ������δʹ�ñ���ͨ�� ��������
		bit[29]: ʹ��Ԥ��ͨ��
	*/
	*CIIMGCPT =  ( 1<<31 ) | ( 1<<29  );
	*CIPRSCCTRL |= ( 1<<15 );


	return 0;
}

/* ֹͣ����
 * �ο� : uvc_video_enable(video, 0)
 */
static int cmos_ov7740_vidioc_streamoff ( struct file* file, void* priv, enum v4l2_buf_type t )
{
	//ֹͣԤ������
	*CIPRSCCTRL |= ( 0<<15 );
	//��ʹ������ͷ������ ������Ԥ��ͨ��
	*CIIMGCPT |= ( ( 0<<31 ) | ( 0<<29 ) );


	return 0;
}


static const struct v4l2_ioctl_ops cmos_ov7740_ioctl_ops =
{
	// ��ʾ����һ������ͷ�豸
	.vidioc_querycap      = cmos_ov7740_vidioc_querycap,

	/* �����о١���á����ԡ���������ͷ�����ݵĸ�ʽ */
	.vidioc_enum_fmt_vid_cap  = cmos_ov7740_vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap     = cmos_ov7740_vidioc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap   = cmos_ov7740_vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap     = cmos_ov7740_vidioc_s_fmt_vid_cap,

	/* ����������: ����/��ѯ/�������/ȡ������ */
	.vidioc_reqbufs       = cmos_ov7740_vidioc_reqbufs,

	/* ˵��: ��Ϊ������ͨ�����ķ�ʽ���������ͷ����,��˲�ѯ/�������/ȡ��������Щ����������������Ҫ */
#if 0
	.vidioc_querybuf      = myuvc_vidioc_querybuf,
	.vidioc_qbuf          = myuvc_vidioc_qbuf,
	.vidioc_dqbuf         = myuvc_vidioc_dqbuf,
#endif

	// ����/ֹͣ
	.vidioc_streamon      = cmos_ov7740_vidioc_streamon,
	.vidioc_streamoff     = cmos_ov7740_vidioc_streamoff,
};



/* ������ͷ�豸 */
static int cmos_ov7740_open ( struct file* file )
{
	return 0;
}

/* �ر�����ͷ�豸 */
static int cmos_ov7740_close ( struct file* file )
{

	return 0;
}

/* Ӧ�ó���ͨ�����ķ�ʽ��ȡ����ͷ������ */
static ssize_t cmos_ov7740_read ( struct file* filep, char __user* buf, size_t count, loff_t* pos )
{
	size_t end_size;
	int i;

	//�õ�����֮���Сֵ
	end_size = min_t ( size_t,buf_size,count );

	wait_event_interruptible ( cam_wait_queue, ev_cam );

	for ( i=0; i<4; i++ )
	{
		//�������ݸ��û��ռ�
		if ( copy_to_user ( buf, img_buff[i].virt_address, end_size ) )
		{
			return -EFAULT;
		}
	}

	ev_cam = 0;

	return end_size;
}

/*
	ע��:
		�ú����Ǳ����,������insmod��ʱ�򣬻����
		��rmmodʱ�ͷŷ�����ڴ�
*/
static void cmos_ov7740_release ( struct video_device* vdev )
{

	free_pages ( img_buff[0].virt_address,img_buff[0].order );
	img_buff[0].phy_address = ( unsigned long ) NULL;

	free_pages ( img_buff[1].virt_address,img_buff[1].order );
	img_buff[1].phy_address = ( unsigned long ) NULL;

	free_pages ( img_buff[2].virt_address,img_buff[2].order );
	img_buff[2].phy_address = ( unsigned long ) NULL;

	free_pages ( img_buff[3].virt_address,img_buff[3].order );
	img_buff[3].phy_address = ( unsigned long ) NULL;

}

/* 3.1. ���䡢����һ��v4l2_file_operations�ṹ��
 * ��ԱΪvideo_device�豸�Ĳ�������
 */
static const struct v4l2_file_operations cmos_ov7740_fops =
{
	.owner			= THIS_MODULE,
	.open       		= cmos_ov7740_open,
	.release    		= cmos_ov7740_close,
	.unlocked_ioctl     = video_ioctl2,
	.read			= cmos_ov7740_read,
};


/* 2.1. ���䡢����һ��video_device�ṹ��
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

	/* ������Ӧ��GPIO����CAMIF */
	*GPJCON = 0x2aaaaaa;
	*GPJDAT = 0;


	/* ʹ���������� */
	*GPJUP = 0;
}

static void cmos_ov7740_camif_reset ( void )
{
	/* ���ô��䷽ʽΪBT601 */
	*CISRCFMT |= ( 1<<31 );


	/* ��λCAMIF������ */
	*CIGCTRL  |= ( 1<<31 );
	mdelay ( 10 );
	*CIGCTRL  &=~ ( 1<<31 );
	mdelay ( 10 );

}

static void cmos_ov7740_clk_cfg ( void )
{
	struct clk* camif_clk;
	struct clk* camif_upll_clk;


	/* ʹ��CAMIF��ʱ��Դ */
	camif_clk = clk_get ( NULL, "camif" );
	if ( !camif_clk || IS_ERR ( camif_clk ) )
	{
		printk ( KERN_INFO"failed to get camif clock\n" );
	}
	clk_enable ( camif_clk );

	/* ʹ�ܲ�����CAMCLK = 24MHz */
	camif_upll_clk = clk_get ( NULL, "camif-upll" );
	if ( !camif_upll_clk || IS_ERR ( camif_upll_clk ) )
	{
		printk ( KERN_INFO"failed to get camif-upll clock\n" );
	}
	clk_set_rate ( camif_upll_clk, 24000000 ); //����Ƶ��Ϊ24Mhz
	mdelay ( 100 );


}

static void cmos_ov7740_reset ( void )
{

	//��λ��Ҫ�ĵ�ƽ˳��Ϊ 1-0-1
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

	//��ȡ����ͷ���豸id
	mid = i2c_smbus_read_byte_data ( cmos_ov7740_client, 0x0a ) <<8;
	mid |= i2c_smbus_read_byte_data ( cmos_ov7740_client, 0x0b );
	printk ( "manufacture ID = 0x%4x\n", mid );

	/* ʹ�ó����ṩ�ĳ�ʼ�������ʼ������ͷ�Ĵ���

	*/
	for ( i=0; i<OV7740_INIT_REGS_SIZE; i++ )
	{
		i2c_smbus_write_byte_data ( cmos_ov7740_client, ov7740_setting_30fps_VGA_640_480[i].regaddr, ov7740_setting_30fps_VGA_640_480[i].value  );
		mdelay ( 2 );

	}


}



//��drv name ��dev name��ͬʱ��ִ��probe����
//��Ҫ��probe������ע��video�豸
static int __devinit cmos_ov7740_probe ( struct i2c_client* client, const struct i2c_device_id* id )
{

	printk ( "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__ );
	/* 2.3 Ӳ����� */
	/* 2.3.1 ӳ����Ӧ�ļĴ��� */
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

	/* 2.3.2 ������Ӧ��GPIO����CAMIF */
	cmos_ov7740_gpio_cfg();
	/* 2.3.3 ��λһ��CAMIF������ */
	cmos_ov7740_camif_reset();

	/* 2.3.4 ���á�ʹ��ʱ��(ʹ��HCLK��ʹ�ܲ�����CAMCLK = 24MHz) */
	cmos_ov7740_clk_cfg();

	/* 2.3.5 ��λһ������ͷģ�� */
	cmos_ov7740_reset();


	/* 2.3.6 ͨ��IIC����,��ʼ������ͷģ�� */
	cmos_ov7740_client = client;
	cmos_ov7740_init();


	/* 2.3.7 ע���ж� */
	if ( request_irq ( IRQ_S3C2440_CAM_C, cmos_ov7740_camif_irq_c, IRQF_DISABLED, "CAM_C", NULL ) )
	{
		printk ( "[%s]:%d request irq fail ",__func__,__LINE__ );
	}

	if ( request_irq ( IRQ_S3C2440_CAM_P, cmos_ov7740_camif_irq_p, IRQF_DISABLED, "CAM_P", NULL ) )
	{
		printk ( "[%s]:%d request irq fail ",__func__,__LINE__ );
	}

	/* 2.2.ע���豸 */
	if ( video_register_device ( &cmos_ov7740_vdev, VFL_TYPE_GRABBER, -1 ) ) //VFL_TYPE_GRABBERͼ��ɼ��豸
	{
		printk ( "[%s]:%d register device fail ",__func__,__LINE__ );
	}

	return 0;

}

//��rmmod���豸����ʱ��ִ�д˺���
//remove �������Ƴ�video�豸
static int __devexit cmos_ov7740_remove ( struct i2c_client* client )
{
	printk ( "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__ );

	//�ͷ������ڴ�ӳ��
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

	//�ͷ��ж�
	free_irq ( IRQ_S3C2440_CAM_C, NULL );
	free_irq ( IRQ_S3C2440_CAM_P, NULL );
	video_unregister_device ( &cmos_ov7740_vdev );
	return 0;

}


static const struct i2c_device_id cmos_ov7740_id_table[] =
{
	{ "cmos_ov7740", 0 },
	{}
};


/* 1.1. ���䡢����һ��i2c_driver */
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

	/* 1.2.ע������ */
	i2c_add_driver ( &cmos_ov7740_driver );

}


static void cmos_ov7740_drv_exit ( void )
{
	//�˳�������ɾ������
	i2c_del_driver ( &cmos_ov7740_driver );
}

module_init ( cmos_ov7740_drv_init );
module_exit ( cmos_ov7740_drv_exit );

MODULE_LICENSE ( "GPL" );
