#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <asm/io.h>
/* 参考 sound\soc\codecs\uda134x.c
 */

/* 1. 构造一个snd_soc_dai_driver
 * 2. 构造一个snd_soc_codec_driver
 * 3. 注册它们
 */
#define WM8976_RATES SNDRV_PCM_RATE_8000_48000
#define WM8976_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | \
				SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)
	
	
#define WM8976_REG_NUM 58

/* 所有寄存器的默认值 */
static const unsigned short wm8976_reg[WM8976_REG_NUM] = {
};



static volatile unsigned int *gpbdat;
static volatile unsigned int *gpbcon;

static void wm8976_init_regs ( struct snd_soc_codec* codec );

/*
 * 获得音量信息,比如最小值最大值
 */
static int wm8976_info_vol(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER; //音量为整数数据
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 63;
	return 0;
}

/*
 * 获得当前音量值
 */
static int wm8976_get_vol(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	
	ucontrol->value.integer.value[0] = snd_soc_read(codec, 52) & 0x3f;
	ucontrol->value.integer.value[1] = snd_soc_read(codec, 53) & 0x3f;
	return 0;
}

/*
 * 设置当前音量值
 */
static int wm8976_put_vol(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int val;

	val = ucontrol->value.integer.value[0];
    snd_soc_write(codec, 52, (1<<8)|val);

	val = ucontrol->value.integer.value[1];
    snd_soc_write(codec, 53, (1<<8)|val);
	
	return 0;
}

//音量信息获取与调整
static const struct snd_kcontrol_new wm8976_vol_control = 
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, 
    .name = "Master Playback Volume",  //名字代表功能 不能随意取
	.info = wm8976_info_vol, //获取音量数据信息
	.get  = wm8976_get_vol, //获取当前音量
	.put  = wm8976_put_vol, //设置音量
};

static int wm8976_soc_probe ( struct snd_soc_codec* codec )
{
    int ret;

	wm8976_init_regs(codec);
    
	ret = snd_soc_add_codec_controls(codec, &wm8976_vol_control, 1); //注册kcontrol数据结构
	
	return ret;
}

/*
 * The codec has no support for reading its registers except for peak level...
 */
static inline unsigned int wm8976_read_reg_cache ( struct snd_soc_codec* codec,
                                                    unsigned int reg )
{
	u8* cache = codec->reg_cache;

	if (reg >= WM8976_REG_NUM)
		return -1;
	return cache[reg];
}

static void set_csb(int val)
{
	if (val)
	{
		*gpbdat |= (1<<2);
	}
	else
	{
		*gpbdat &= ~(1<<2);
	}
}

static void set_clk ( int val )
{
	if ( val )
	{
		*gpbdat |= ( 1<<4 );
	}
	else
	{
		*gpbdat &= ~ ( 1<<4 );
	}
}

static void set_dat ( int val )
{
	if ( val )
	{
		*gpbdat |= ( 1<<3 );
	}
	else
	{
		*gpbdat &= ~ ( 1<<3 );
	}
}



//写寄存器函数 reg代表写哪个寄存器
static int wm8976_write_reg ( struct snd_soc_codec* codec, unsigned int reg,
                               unsigned int value )
{

	u8* cache = codec->reg_cache;
	int i;
	unsigned short val = (reg << 9) | (value & 0x1ff);

	/* 先保存 */
	if ( reg >= WM8976_REG_NUM )
		return -1;
	cache[reg] = value;

	/* 再写入硬件 */

    /* 再写入硬件 */
    set_csb(1);
    set_dat(1);
    set_clk(1);

	for (i = 0; i < 16; i++){
		if (val & (1<<15))
		{
            set_clk(0);
            set_dat(1);
			udelay(1);
            set_clk(1);
		}
		else
		{
            set_clk(0);
            set_dat(0);
			udelay(1);
            set_clk(1);
		}

		val = val << 1;
	}

    set_csb(0);
	udelay(1);
    set_csb(1);
    set_dat(1);
    set_clk(1);
    


	return 0;
}


//芯片操作相关函数
static struct snd_soc_codec_driver soc_codec_dev_wm8976 = {
	.probe = wm8976_soc_probe,
	/* UDA1341的寄存器不支持读操作
	* 要知道某个寄存器的当前值,
	* 只能在写入时保存起来
	*/
	.reg_cache_size = sizeof ( wm8976_reg ), //所有寄存器占据的空间大小
	.reg_word_size = sizeof ( u16 ),           //每个寄存器的大小
	.reg_cache_default = wm8976_reg,         //寄存器的默认值
	.reg_cache_step = 2,                      //用几个字节表示一个寄存器
	.read  = wm8976_read_reg_cache,          //读寄存器函数
	.write = wm8976_write_reg,               // 写寄存器 
};

static void wm8976_init_regs ( struct snd_soc_codec* codec )
{

	/* GPB 4: L3CLOCK */
	/* GPB 3: L3DATA */
	/* GPB 2: L3MODE */
    *gpbcon &= ~((3<<4) | (3<<6) | (3<<8)); //设置mode线为输出模式
    *gpbcon |= ((1<<4) | (1<<6) | (1<<8));
	/* software reset */
	wm8976_write_reg(codec, 0, 0);

	/* OUT2的左/右声道打开
	 * 左/右通道输出混音打开
	 * 左/右DAC打开
	 */
	wm8976_write_reg(codec, 0x3, 0x6f);
	
	wm8976_write_reg(codec, 0x1, 0x1f);//biasen,BUFIOEN.VMIDSEL=11b  
	wm8976_write_reg(codec, 0x2, 0x185);//ROUT1EN LOUT1EN, inpu PGA enable ,ADC enable

	wm8976_write_reg(codec, 0x6, 0x0);//SYSCLK=MCLK  
	wm8976_write_reg(codec, 0x4, 0x10);//16bit 		
	wm8976_write_reg(codec, 0x2B,0x10);//BTL OUTPUT  
	wm8976_write_reg(codec, 0x9, 0x50);//Jack detect enable  
	wm8976_write_reg(codec, 0xD, 0x21);//Jack detect  
	wm8976_write_reg(codec, 0x7, 0x01);//Jack detect 

}


static int wm8976_hw_params ( struct snd_pcm_substream* substream,
                               struct snd_pcm_hw_params* params,
                               struct snd_soc_dai* dai )
{
	/* 根据params的值,设置UDA1341的寄存器
	 * 比如时钟设置,格式
	 */
	/* 为了简单, 在wm8976_init_regs里就设置好时钟、格式等参数 */
	return 0;
}



//iis 开启、关闭、参数设置
static const struct snd_soc_dai_ops wm8976_dai_ops = {
	.hw_params	= wm8976_hw_params, //参数设置
};

//芯片DAI相关参数
//wm8976 iis的通道数 采样率 格式
static struct snd_soc_dai_driver wm8976_dai = {
	.name = "wm8976-iis",
	/* playback capabilities */
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = WM8976_RATES,
		.formats = WM8976_FORMATS,
	},
	/* capture capabilities */
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = WM8976_RATES,
		.formats = WM8976_FORMATS,
	},
	/* pcm operations */
	.ops = &wm8976_dai_ops, //芯片dai相关操作函数
};


/* 通过注册平台设备、平台驱动来实现对snd_soc_register_codec的调用
 *
 */

static void wm8976_dev_release ( struct device* dev )
{
}

static int wm8976_probe ( struct platform_device* pdev )
{
	return snd_soc_register_codec ( &pdev->dev,
	                                &soc_codec_dev_wm8976, &wm8976_dai, 1 );
}

static int wm8976_remove ( struct platform_device* pdev )
{
    snd_soc_unregister_codec(&pdev->dev);
    return 0;
}


static struct platform_device wm8976_dev = {
	.name         = "wm8976-codec",
	.id       = -1,
	.dev = {
		.release = wm8976_dev_release,
	},
};
struct platform_driver wm8976_drv = {
	.probe		= wm8976_probe,
	.remove		= wm8976_remove,
	.driver		= {
		.name	= "wm8976-codec",
	}
};

static int wm8976_init ( void )
{
    
    gpbcon = ioremap(0x56000010, 4);
    gpbdat = ioremap(0x56000014, 4);

	platform_device_register ( &wm8976_dev );
	platform_driver_register ( &wm8976_drv );
	return 0;
}

static void wm8976_exit ( void )
{
	platform_device_unregister ( &wm8976_dev );
	platform_driver_unregister ( &wm8976_drv );

    
    iounmap(gpbcon);
    iounmap(gpbdat);
	
}

module_init ( wm8976_init );
module_exit ( wm8976_exit );

MODULE_LICENSE("GPL");
