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
static unsigned long *GPJCON;
static unsigned long *GPJDAT;
static unsigned long *GPJUP;

// CAMIF
static unsigned long *CISRCFMT;
static unsigned long *CIWDOFST;
static unsigned long *CIGCTRL;
static unsigned long *CIPRCLRSA1;
static unsigned long *CIPRCLRSA2;
static unsigned long *CIPRCLRSA3;
static unsigned long *CIPRCLRSA4;
static unsigned long *CIPRTRGFMT;
static unsigned long *CIPRCTRL;
static unsigned long *CIPRSCPRERATIO;
static unsigned long *CIPRSCPREDST;
static unsigned long *CIPRSCCTRL;
static unsigned long *CIPRTAREA;
static unsigned long *CIIMGCPT;

// IRQ
static unsigned long *SRCPND;
static unsigned long *INTPND;
static unsigned long *SUBSRCPND;



/* �ο� uvc_v4l2_do_ioctl */
static int cmos_ov7740_vidioc_querycap(struct file *file, void  *priv,
					struct v4l2_capability *cap)
{
	return 0;
}

/* �о�֧�����ָ�ʽ
 * �ο�: uvc_fmts ����
 */
static int cmos_ov7740_vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
					struct v4l2_fmtdesc *f)
{
	return 0;
}

/* ���ص�ǰ��ʹ�õĸ�ʽ */
static int cmos_ov7740_vidioc_g_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	return 0;
}

/* �������������Ƿ�֧��ĳ�ָ�ʽ, ǿ�����øø�ʽ 
 * �ο�: uvc_v4l2_try_format
 *       myvivi_vidioc_try_fmt_vid_cap
 */
static int cmos_ov7740_vidioc_try_fmt_vid_cap(struct file *file, void *priv,
			struct v4l2_format *f)
{
	return 0;
}

/* �ο� myvivi_vidioc_s_fmt_vid_cap */
static int cmos_ov7740_vidioc_s_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	return 0;
}

//���뻺����
static int cmos_ov7740_vidioc_reqbufs(struct file *file, void *priv,
			  struct v4l2_requestbuffers *p)
{
	return 0;
}

/* �������� 
 * �ο�: uvc_video_enable(video, 1):
 *           uvc_commit_video
 *           uvc_init_video
 */
static int cmos_ov7740_vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	return 0;
}

/* ֹͣ 
 * �ο� : uvc_video_enable(video, 0)
 */
static int cmos_ov7740_vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type t)
{
	return 0;
}


static const struct v4l2_ioctl_ops cmos_ov7740_ioctl_ops = {
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
static int cmos_ov7740_open(struct file *file)
{
	return 0;
}

/* �ر�����ͷ�豸 */
static int cmos_ov7740_close(struct file *file)
{
    
	return 0;
}

/* Ӧ�ó���ͨ�����ķ�ʽ��ȡ����ͷ������ */
static ssize_t cmos_ov7740_read(struct file *filep, char __user *buf, size_t count, loff_t *pos)
{
	return 0;
}


/* 3.1. ���䡢����һ��v4l2_file_operations�ṹ�� 
 * ��ԱΪvideo_device�豸�Ĳ�������
 */
static const struct v4l2_file_operations cmos_ov7740_fops = {
	.owner			= THIS_MODULE,
	.open       		= cmos_ov7740_open,
	.release    		= cmos_ov7740_close,
	.unlocked_ioctl     = video_ioctl2,
	.read			= cmos_ov7740_read,
};


/* 2.1. ���䡢����һ��video_device�ṹ�� 
 * 
 */
static struct video_device cmos_ov7740_vdev = {
	.fops		= &cmos_ov7740_fops,
	.ioctl_ops		= &cmos_ov7740_ioctl_ops,
	.release		= cmos_ov7740_release,
	.name		= "cmos_ov7740",
};

static void cmos_ov7740_gpio_cfg(void)
{

   /* ������Ӧ��GPIO����CAMIF */
   *GPJCON = 0x2aaaaaa;
   *GPJDAT = 0;

   
   /* ʹ���������� */
   *GPJUP = 0;
}

static void cmos_ov7740_camif_reset(void)
{
   /* ���䷽ʽΪBT601 */
   *CISRCFMT |= (1<<31);

   
   /* ��λCAMIF������ */
   *CIGCTRL  |= (1<<31);
   mdelay(10);
   *CIGCTRL  &=~ (1<<31);
   mdelay(10);

}

//��drv name ��dev name��ͬʱ��ִ��probe����
//��Ҫ��probe������ע��video�豸
static int __devinit cmos_ov7740_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    
	printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	/* 2.3 Ӳ����� */
	/* 2.3.1 ӳ����Ӧ�ļĴ��� */    
	GPJCON = ioremap(0x560000d0, 4);
	GPJDAT = ioremap(0x560000d4, 4);
	GPJUP = ioremap(0x560000d8, 4);
    
	CISRCFMT = ioremap(0x4F000000, 4);
	CIWDOFST = ioremap(0x4F000004, 4);
	CIGCTRL = ioremap(0x4F000008, 4);
	CIPRCLRSA1 = ioremap(0x4F00006C, 4);
	CIPRCLRSA2 = ioremap(0x4F000070, 4);
	CIPRCLRSA3 = ioremap(0x4F000074, 4);
	CIPRCLRSA4 = ioremap(0x4F000078, 4);
	CIPRTRGFMT = ioremap(0x4F00007C, 4);
	CIPRCTRL = ioremap(0x4F000080, 4);
	CIPRSCPRERATIO = ioremap(0x4F000084, 4);
	CIPRSCPREDST = ioremap(0x4F000088, 4);
	CIPRSCCTRL = ioremap(0x4F00008C, 4);
	CIPRTAREA = ioremap(0x4F000090, 4);
	CIIMGCPT = ioremap(0x4F0000A0, 4);

	SRCPND = ioremap(0X4A000000, 4);
	INTPND = ioremap(0X4A000010, 4);
	SUBSRCPND = ioremap(0X4A000018, 4);
	
	/* 2.3.2 ������Ӧ��GPIO����CAMIF */
	cmos_ov7740_gpio_cfg();
	/* 2.3.3 ��λһ��CAMIF������ */
    cmos_ov7740_camif_reset();
	
	/* 2.3.4 ���á�ʹ��ʱ��(ʹ��HCLK��ʹ�ܲ�����CAMCLK = 24MHz) */
	/* 2.3.5 ��λһ������ͷģ�� */
	
	/* 2.3.6 ͨ��IIC����,��ʼ������ͷģ�� */
	
	/* 2.3.7 ע���ж� */

	/* 2.2.ע���豸 */
    video_register_device(&cmos_ov7740_vdev, VFL_TYPE_GRABBER, -1);

    return 0;

}

//��rmmod���豸����ʱ��ִ�д˺���
//remove �������Ƴ�video�豸
static int __devexit cmos_ov7740_remove(struct i2c_client *client)
{   
   printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

   video_unregister_device(&cmos_ov7740_vdev);
   return 0; 

}


static const struct i2c_device_id cmos_ov7740_id_table[] = {
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

