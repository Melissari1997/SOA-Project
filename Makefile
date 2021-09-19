MODNAME=TAG_Data_Messaging
USER_DIR=user
TEST_DIR=test
CLI_DIR=cli

USER_LIB_INC=-I$(USER_DIR)/interface $(USER_DIR)/interface/interface.c

ifeq ($(KERNELRELEASE),)
# if KERNELRELEASE is not defined, we've been called directly from the command line.
# Invoke the kernel build system.
PWD=$(shell pwd)
.PHONY: all install clean uninstall load unload
all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CC) $(USER_DIR)/commands/get.c $(USER_LIB_INC) -o $(USER_DIR)/commands/get.out -Iglobal_include
	$(CC) $(USER_DIR)/commands/send.c $(USER_LIB_INC) -o $(USER_DIR)/commands/send.out -Iglobal_include
	$(CC) $(USER_DIR)/commands/receive.c $(USER_LIB_INC) -o $(USER_DIR)/commands/receive.out -Iglobal_include
	$(CC) $(USER_DIR)/commands/ctl.c $(USER_LIB_INC) -o $(USER_DIR)/commands/ctl.out -Iglobal_include
	$(CC) $(USER_DIR)/test/demo.c $(USER_LIB_INC) -o $(USER_DIR)/test/demo.out -Iglobal_include -lpthread
clean:
	echo "$(MODNAME) Clean..."
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
install:
	echo "$(MODNAME) Install..."
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules_install
	ln -s /lib/modules/$(shell uname -r)/extra/$(MODNAME).ko /lib/modules/$(shell uname -r)
	depmod -a

uninstall:
	echo "$(MODNAME) Uninstall..."
	rm /lib/modules/$(shell uname -r)/extra/$(MODNAME).ko
	rm /lib/modules/$(shell uname -r)/$(MODNAME).ko
	depmod -a

load:
	echo "$(MODNAME) Loading..."
	sudo insmod $(MODNAME).ko
unload:
	echo "$(MODNAME) Removing..."
	sudo rmmod $(MODNAME).ko

else
# Otherwise KERNELRELEASE is defined; we've been invoked from the
# kernel build system and can use its language.
EXTRA_CFLAGS = -Wall

obj-m += $(MODNAME).o
$(MODNAME)-y += main.o


# TAG-based data exchange Sub-system
$(MODNAME)-y += ./TAG_MESSAGING/TAG_Subsystem/TAG_Messaging.o ./TAG_MESSAGING/TAG_Subsystem/TAG_Messagin_help.o ./TAG_MESSAGING/TAG_Subsystem/rcu-list.o 

# sysCall_Discovery Sub-system
$(MODNAME)-y += ./TAG_MESSAGING/sysCall_Discovery/sysCall_Discovery.o ./TAG_MESSAGING/sysCall_Discovery/vtpmo/vtpmo.o

# charDevice Sub-system
$(MODNAME)-y += ./TAG_MESSAGING/charDev/charDev.o

# this are need to add the include path to the kernel build system
ccflags-y := -I$(PWD)/global_include

endif
