/* 参考 sound\soc\codecs\uda134x.c
 */

/* 1. 构造一个snd_soc_dai_driver
 * 2. 构造一个snd_soc_codec_driver
 * 3. 注册它们
 */

//codec相关函数
static struct snd_soc_codec_driver soc_codec_dev_uda1341 =
{

};


//iis 开启、关闭、参数设置 
static const struct snd_soc_dai_ops uda1341_dai_ops = 
{

};

//指明uda1341 iis的通道数 采样率 格式
static struct snd_soc_dai_driver uda1341_dai =
{
	.name = "uda1341-iis",
	/* playback capabilities */
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = UDA134X_RATES,
		.formats = UDA134X_FORMATS,
	},
	/* capture capabilities */
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = UDA134X_RATES,
		.formats = UDA134X_FORMATS,
	},
	/* pcm operations */
	.ops = &uda1341_dai_ops,
};


/* 通过注册平台设备、平台驱动来实现对snd_soc_register_codec的调用
 *
 */

static void uda1341_dev_release ( struct device* dev )
{
}

static int uda1341_probe ( struct platform_device* pdev )
{
	return snd_soc_register_codec ( &pdev->dev,
	                                &soc_codec_dev_uda1341, &uda1341_dai, 1 );
}

static int uda1341_remove ( struct platform_device* pdev )
{
	return snd_soc_unregister_codec ( &pdev->dev );
}


static struct platform_device uda1341_dev =
{
	.name         = "uda1341-codec",
	.id       = -1,
	.dev = {
		.release = uda1341_dev_release,
	},
};
struct platform_driver uda1341_drv =
{
	.probe		= uda1341_probe,
	.remove		= uda1341_remove,
	.driver		= {
		.name	= "uda1341-codec",
	}
};

static int uda1341_init ( void )
{
	platform_device_register ( &uda1341_dev );
	platform_driver_register ( &uda1341_drv );
	return 0;
}

static void uda1341_exit ( void )
{
	platform_device_unregister ( &uda1341_dev );
	platform_driver_unregister ( &uda1341_drv );
}

module_init ( uda1341_init );
module_exit ( uda1341_exit );

