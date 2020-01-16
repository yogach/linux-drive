/*
 * 参考 drivers\net\cs89x0.c
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/ip.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>


struct net_device *virt_net;


int virt_net_send_packet(struct sk_buff *skb,struct net_device *dev)
{
   
    /* 对于真实的网卡, 把skb里的数据通过网卡发送出去 */
    netif_stop_queue(dev); /* 停止该网卡的队列 */
    /* ...... */           /* 真实网卡的这一步应该是把skb的数据写入网卡 */
	/* 构造一个假的sk_buff,上报 */
	emulator_rx_packet(skb, dev);



}

static int virt_net_init(void)
{
   /* 1. 分配一个net_device结构体 */
    virt_net = alloc_netdev(0, "vnet%d", ether_setup); //第一个参数私有数据空间 第二网卡名字 第三个初始化函数
   /* 2. 设置发包函数 */
	virt_net->hard_start_xmit = virt_net_send_packet;
   /* 设置MAC地址 */
   virt_net->dev_addr[0] = 0x08;
   virt_net->dev_addr[1] = 0x89;
   virt_net->dev_addr[2] = 0x89;
   virt_net->dev_addr[3] = 0x89;
   virt_net->dev_addr[4] = 0x89;
   virt_net->dev_addr[5] = 0x11;

   /* 设置下面两项才能ping通 */
   virt_net->flags |= IFF_NOARP; //非ARP协议
   virt_net->features |= NETIF_F_NO_CSUM; //取消校验

   
   /* 3. 注册 */
   register_netdev(virt_net);

   return 0;

}

sttaic void virt_net_exit(void)
{
  unregister_netdev(virt_net);
  free_netdev(virt_net);
}

module_init(virt_net_init);
module_exit(virt_net_exit);
MODULE_LICENSE("GPL");


