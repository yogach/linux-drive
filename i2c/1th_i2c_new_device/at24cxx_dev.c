#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/regmap.h>
#include <linux/slab.h>


static struct i2c_board_info at24cxx_info = {	
	I2C_BOARD_INFO("at24c08", 0x50),
};


static int at24cxx_dev_init(void)
{

 

  return 0;
}

static void at24cxx_dev_exit(void)
{



}


module_init(at24cxx_dev_init);
module_exit(at24cxx_dev_exit);
MODULE_LICENSE("GPL");

