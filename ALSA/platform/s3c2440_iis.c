#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <mach/regs-gpio.h>
#include <mach/dma.h>
/* 参考sound\soc\samsung\s3c24xx-i2s.c
 */

struct s3c2440_iis_regs {
    unsigned int iiscon ; 
    unsigned int iismod ; 
    unsigned int iispsr ; 
    unsigned int iisfcon; 
    unsigned int iisfifo; 
};


static volatile unsigned int *gpecon;
static volatile	struct s3c2440_iis_regs *iis_regs;

#define ABS(a, b) ((a>b)?(a-b):(b-a))

//iis 启动函数
static void s3c2440_iis_start(void)
{
    iis_regs->iiscon |= (1);
}
static void s3c2440_iis_stop(void)
{
    iis_regs->iiscon &= ~(1);
}

static int s3c2440_i2s_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
   /* 根据params设置IIS控制器 */

    int tmp_fs;
    int i;
    int min = 0xffff;
    int pre = 0;
    unsigned int fs;
    struct clk *clk = clk_get(NULL, "pclk");

    /* 配置GPIO用于IIS */
    *gpecon &= ~((3<<0) | (3<<2) | (3<<4) | (3<<6) | (3<<8));
    *gpecon |= ((2<<0) | (2<<2) | (2<<4) | (2<<6) | (2<<8));
    
    
    /* bit[9] : Master clock select, 0-PCLK
     * bit[8] : 0 = Master mode
     * bit[7:6] : 10 = Transmit mode
     * bit[4] : 0-IIS compatible format
     * bit[2] : 384fs, 确定了MASTER CLOCK之后, fs = MASTER CLOCK/384
     * bit[1:0] : Serial bit clock frequency select, 32fs
     */
     
    if (params_format(params) == SNDRV_PCM_FORMAT_S16_LE)
        iis_regs->iismod = (2<<6) | (0<<4) | (1<<3) | (1<<2) | (1);
    else if (params_format(params) == SNDRV_PCM_FORMAT_S8)
        iis_regs->iismod = (2<<6) | (0<<4) | (0<<3) | (1<<2) | (1);
    else
        return -EINVAL;

    /* Master clock = PCLK/(n+1)
     * fs = Master clock / 384
     * fs = PCLK / (n+1) / 384
     */
    fs = params_rate(params);
    for (i = 0; i <= 31; i++)
    {
        tmp_fs = clk_get_rate(clk)/384/(i+1);
        if (ABS(tmp_fs, fs) < min)
        {
            min = ABS(tmp_fs, fs);
            pre = i;
        }
    }
    iis_regs->iispsr = (pre << 5) | (pre);

    /*
     * bit15 : Transmit FIFO access mode select, 1-DMA
     * bit13 : Transmit FIFO, 1-enable
     */
    iis_regs->iisfcon = (1<<15) | (1<<13);
    
    /*
     * bit[5] : Transmit DMA service request, 1-enable
     * bit[1] : IIS prescaler, 1-enable
     */
    iis_regs->iiscon = (1<<5) | (1<<1) ;

    clk_put(clk);
    
    return 0;
}

static int s3c2440_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
				   struct snd_soc_dai *dai)
{
	int ret = 0;

    //在触发传输时，同时开启iis通道
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		s3c2440_iis_start();
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		s3c2440_iis_stop();
	    break;
	default:
		
		ret = -EINVAL;
		break;
	}

exit_err:
	return ret;
}

//s3c2440 iis启动 关闭 参数设置
static const struct snd_soc_dai_ops s3c2440_i2s_dai_ops = {
    .hw_params	= s3c2440_i2s_hw_params, //硬件参数设置
    .trigger = s3c2440_i2s_trigger,
};

#define S3C24XX_I2S_RATES \
	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | \
	SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | \
	SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)

static struct snd_soc_dai_driver s3c2440_i2s_dai = {
    //指明了s3c2440 iis接口的相关属性 采样率 格式等 
	.playback = {
		.channels_min = 2, //通道数
		.channels_max = 2,
		.rates = S3C24XX_I2S_RATES, //采样率
		.formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE,}, //格式
	.capture = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = S3C24XX_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE,},
	.ops = &s3c2440_i2s_dai_ops,
};


static int s3c2440_iis_probe(struct platform_device *pdev)
{
	return snd_soc_register_dai(&pdev->dev, &s3c2440_i2s_dai);
}
static int s3c2440_iis_remove(struct platform_device *pdev)
{
	snd_soc_unregister_dai(&pdev->dev);
    return 0;
}

static void s3c2440_iis_release(struct device * dev)
{
}


//device和driver的名字匹配上之后会调用drv中的probe函数
static struct platform_device s3c2440_iis_dev = {
    .name         = "s3c2440-iis",  
    .id       = -1,
    .dev = { 
    	.release = s3c2440_iis_release, 
	},
};

struct platform_driver s3c2440_iis_drv = {
	.probe		= s3c2440_iis_probe,
	.remove		= s3c2440_iis_remove,
	.driver		= {
		.name	= "s3c2440-iis",
	}
};

static int s3c2440_iis_init(void)
{
    struct clk *clk;

	//使能iis时钟
	clk = clk_get(NULL, "iis");
	clk_enable(clk);
	clk_put(clk);

   gpecon = ioremap(0x56000040, 4);
   iis_regs = ioremap(0x55000000, sizeof(struct s3c2440_iis_regs));

   platform_device_register(&s3c2440_iis_dev);
   platform_driver_register(&s3c2440_iis_drv);

   return 0;
}

static void s3c2440_iis_exit(void)
{   
    struct clk *clk;

	
	platform_device_unregister(&s3c2440_iis_dev);
	platform_driver_unregister(&s3c2440_iis_drv);

    //取消iis时钟使能
	clk = clk_get(NULL, "iis");
	clk_disable(clk);
	clk_put(clk);
	
	iounmap(gpecon);
	iounmap(iis_regs);	
}


module_init(s3c2440_iis_init);
module_exit(s3c2440_iis_exit);

MODULE_LICENSE("GPL");

