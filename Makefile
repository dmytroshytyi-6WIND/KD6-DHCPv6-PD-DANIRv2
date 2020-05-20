all: clean comp

comp:	
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/build modules
 
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/build clean
	find . -type f -name '*.o' -delete
	find . -type f -name '*.cmd*' -delete
