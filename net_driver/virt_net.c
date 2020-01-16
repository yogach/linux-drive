/*
 * �ο� drivers\net\cs89x0.c
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
   
    /* ������ʵ������, ��skb�������ͨ���������ͳ�ȥ */
    netif_stop_queue(dev); /* ֹͣ�������Ķ��� */
    /* ...... */           /* ��ʵ��������һ��Ӧ���ǰ�skb������д������ */
	/* ����һ���ٵ�sk_buff,�ϱ� */
	emulator_rx_packet(skb, dev);



}

static int virt_net_init(void)
{
   /* 1. ����һ��net_device�ṹ�� */
    virt_net = alloc_netdev(0, "vnet%d", ether_setup); //��һ������˽�����ݿռ� �ڶ��������� ��������ʼ������
   /* 2. ���÷������� */
	virt_net->hard_start_xmit = virt_net_send_packet;
   /* ����MAC��ַ */
   virt_net->dev_addr[0] = 0x08;
   virt_net->dev_addr[1] = 0x89;
   virt_net->dev_addr[2] = 0x89;
   virt_net->dev_addr[3] = 0x89;
   virt_net->dev_addr[4] = 0x89;
   virt_net->dev_addr[5] = 0x11;

   /* ���������������pingͨ */
   virt_net->flags |= IFF_NOARP; //��ARPЭ��
   virt_net->features |= NETIF_F_NO_CSUM; //ȡ��У��

   
   /* 3. ע�� */
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


