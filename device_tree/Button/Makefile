KERN_DIR = /work/system/linux-4.19-rc3   #linux内核目录

all:
	make -C $(KERN_DIR) M=`pwd` modules 

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order

#依赖文件
obj-m	+= buttons.o
