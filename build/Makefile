
MODULE_NAME += kd6_lkm

#$(MODULE_NAME)-y += ../src/kd6_sm/kd6_sm_funcs.o
#$(MODULE_NAME)-y += ../src/kd6_sm/kd6_sm.o
#$(MODULE_NAME)-y += ../src/kd6_main/kd6_main_funcs.o
$(MODULE_NAME)-y += ../src/kd6_main/kd6_main.o

$(MODULE_NAME)-objs += ../src/kd6_sm/kd6_sm_funcs.o
$(MODULE_NAME)-objs += ../src/kd6_sm/kd6_sm.o
$(MODULE_NAME)-objs += ../src/kd6_main/kd6_main_funcs.o

EXTRA_CFLAGS += -I$(src)/../src/kd6_main
EXTRA_CFLAGS += -I$(src)/../src/kd6_sm
EXTRA_CFLAGS += -I$(src)/../include

obj-m := $(MODULE_NAME).o
