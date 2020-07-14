
//s3c2440 iis启动 关闭 参数设置
static const struct snd_soc_dai_ops s3c2440_i2s_dai_ops = {
};


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
	return snd_soc_unregister_dai(&pdev->dev); //4.9.88内核中此函数只能在soc-core.c中使用
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
   platform_device_register(&s3c2440_iis_dev);
   platform_driver_register(&s3c2440_iis_drv);

   return 0;
}

static void s3c2440_iis_exit(void)
{
   
	platform_device_unregister(&s3c2440_iis_dev);
	platform_driver_unregister(&s3c2440_iis_drv);
}


module_init(s3c2440_iis_init);
module_exit(s3c2440_iis_exit);


