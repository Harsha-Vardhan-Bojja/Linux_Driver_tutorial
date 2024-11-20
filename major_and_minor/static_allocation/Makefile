obj-m += static_alloc.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

install :	
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules_install

