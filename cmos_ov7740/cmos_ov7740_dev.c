#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/regmap.h>
#include <linux/slab.h>

//设置类型和设备地址 设备地址可以从芯片手册中得到 
/*
手册中的设备地址：
写 -- 0x42(01000010)
读 -- 0x43(01000011)

8bit的地址 = 7bit设备地址（高七位） + 1bit的读/写控制位（低一位）

设备地址 = 0100001 = 0x21


*/
static struct i2c_board_info cmos_ov7740_info = {	
	I2C_BOARD_INFO("cmos_ov7740", 0x21),
};

static struct i2c_client *cmos_ov7740_client;

static int cmos_ov7740_dev_init(void)
{
   
   struct i2c_adapter *i2c_adap;

   //获取adapter总线上的相应的I2C设备,参数是设备号
   i2c_adap = i2c_get_adapter(0);

   //新建一个i2c设备
   cmos_ov7740_client = i2c_new_device(i2c_adap, &cmos_ov7740_info);

   //释放适配器
   i2c_put_adapter(i2c_adap);

}


static void cmos_ov7740_dev_exit(void)
{
    //取消注册i2c设备
	i2c_unregister_device(cmos_ov7740_client);
}

module_init(cmos_ov7740_dev_init);
module_exit(cmos_ov7740_dev_exit);

MODULE_LICENSE("GPL");

