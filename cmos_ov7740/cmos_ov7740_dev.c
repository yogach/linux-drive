#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/regmap.h>
#include <linux/slab.h>

//�������ͺ��豸��ַ �豸��ַ���Դ�оƬ�ֲ��еõ� 
/*
�ֲ��е��豸��ַ��
д -- 0x42(01000010)
�� -- 0x43(01000011)

8bit�ĵ�ַ = 7bit�豸��ַ������λ�� + 1bit�Ķ�/д����λ����һλ��

�豸��ַ = 0100001 = 0x21


*/
static struct i2c_board_info cmos_ov7740_info = {	
	I2C_BOARD_INFO("cmos_ov7740", 0x21),
};

static struct i2c_client *cmos_ov7740_client;

static int cmos_ov7740_dev_init(void)
{
   
   struct i2c_adapter *i2c_adap;

   //��ȡadapter�����ϵ���Ӧ��I2C�豸,�������豸��
   i2c_adap = i2c_get_adapter(0);

   //�½�һ��i2c�豸
   cmos_ov7740_client = i2c_new_device(i2c_adap, &cmos_ov7740_info);

   //�ͷ�������
   i2c_put_adapter(i2c_adap);

}


static void cmos_ov7740_dev_exit(void)
{
    //ȡ��ע��i2c�豸
	i2c_unregister_device(cmos_ov7740_client);
}

module_init(cmos_ov7740_dev_init);
module_exit(cmos_ov7740_dev_exit);

MODULE_LICENSE("GPL");

