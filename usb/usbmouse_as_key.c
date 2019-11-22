#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>


/*
 * �ɲο�drivers\hid\usbhid\usbmouse.c
 */

static struct input_dev* uk_dev;
static int len;
static char *usb_buf; //usb����λ��
static dma_addr_t usb_buf_phys;//usb���������ַ
static struct urb *uk_urb;


static void usbmouse_as_key_irq(struct urb *urb)
{
    int i;
	static int cnt = 0;

	printk("data cnt :%d ",cnt);

    for(i=0;i<len;i++)
    {
       printk("%02x ",usb_buf[i]);

	}
	printk("\r\n");

	/* �����ύurb */
	usb_submit_urb(uk_urb, GFP_KERNEL);
}


//��id_table�ܶ�Ӧ��֮��ִ��probe����
static int usbmouse_as_key_probe ( struct usb_interface* intf, const struct usb_device_id* id )
{
    
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usb_host_interface *interface;
	struct usb_endpoint_descriptor *endpoint;
	int pipe;
	
	interface = intf->cur_altsetting;
	endpoint = &interface->endpoint[0].desc;//�˴��ĵ�0���˿� ��ʵ�ǵ�һ���˿�

	/* a. ����һ��input_dev */
	uk_dev = input_allocate_device();

	/* b. ���� */
	/* b.1 �ܲ����������¼� ���ظ����¼� */
	set_bit ( EV_KEY, uk_dev->evbit );
	set_bit ( EV_REP, uk_dev->evbit );

	/* b.2 �ܲ���L ��S��enter�¼� */
	set_bit ( KEY_L, uk_dev->keybit );
	set_bit ( KEY_S, uk_dev->keybit );
	set_bit ( KEY_ENTER, uk_dev->keybit );
	
	/* c. ע�� */
	input_register_device(uk_dev);
	
	/* d. Ӳ����ز��� */
	/* ���ݴ���3Ҫ��: Դ,Ŀ��,���� */
	/* Դ: USB�豸��ĳ���˵� */
	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);
	
	/* ����: */
	len = endpoint->wMaxPacketSize; //��������
	
	/* Ŀ��: */
	usb_buf = usb_buffer_alloc(dev, len, GFP_ATOMIC, &usb_buf_phys);

	
	/* ʹ��"3Ҫ��" */
	/* ����usb request block */
    uk_urb = usb_alloc_urb(0, GFP_KERNEL);
	
	/* ʹ��"3Ҫ������urb" 
	   usb��������ÿendpoint->bInterval���ʱ��ȥ��ȡһ��usb���ݣ�Ȼ�����ݴ��䵽usb_buf��
       �����ݴ�����ɺ�����usbmouse_as_key_irq����
	*/
	usb_fill_int_urb(uk_urb, dev, pipe, usb_buf, len, usbmouse_as_key_irq, NULL, endpoint->bInterval);
    uk_urb->transfer_dma = usb_buf_phys;
	uk_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	
	/* ʹ��URB */
	usb_submit_urb(uk_urb, GFP_KERNEL);
	
}

static void usbmouse_as_key_disconnect ( struct usb_interface* intf )
{
   
	struct usb_device *dev = interface_to_usbdev(intf);

    //
    usb_kill_urb(uk_urb);
    usb_free_urb(uk_urb);
	
    usb_buffer_free(dev, len,usb_buf, usb_buf_phys);
	input_unregister_device(uk_dev);
	input_free_device(uk_dev);


}


//HID�豸�ӿ�
static struct usb_device_id usbmouse_as_key_id_table [] =
{
	{
		USB_INTERFACE_INFO ( USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
		                     USB_INTERFACE_PROTOCOL_MOUSE )
	},
	//{USB_DEVICE(0x1234,0x5678)}, //Ҳ����ʹ��USB_DEVICEָ�������� ֻ֧��ָ����Ӧ�̺ͳ���
	{ }	/* Terminating entry */
};


/* 1. ����/����usb_driver */
static struct usb_driver usbmouse_as_key_driver =
{
	.name = "usbmouse_as_key",
	.probe		= usbmouse_as_key_probe,
	.disconnect	= usbmouse_as_key_disconnect,
	.id_table	= usbmouse_as_key_id_table,
};

static int usbmouse_as_key_init ( void )
{
	/* 2. ע�� */
	usb_register ( &usbmouse_as_key_driver );
	return 0;
}

static void usbmouse_as_key_exit ( void )
{
	usb_deregister ( &usbmouse_as_key_driver );
}

module_init ( usbmouse_as_key_init );
module_exit ( usbmouse_as_key_exit );

MODULE_LICENSE ( "GPL" );



