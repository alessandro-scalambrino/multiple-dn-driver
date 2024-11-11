obj-m := multiple-dn-driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

help:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) help
