# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
KERNEL_DIR ?= $(realpath ../../../source/kernel/)
ARCH ?= arm
#CROSS_COMPILE ?= /home/csy/3588_linux/prebuilts/gcc/linux-x86/aarch64/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-
#CROSS_COMPILE=/home/csy/3588_linux/prebuilts/gcc/linux-x86/arm/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf/bin/
KERNEL_ROOT=$(KERNEL_DIR)
KERNEL_VERSION=5.10
CPU_TYPE=$(ARCH)
OSTYPE=linux
KMPP_SRC := kmpp
MAKE_JOBS ?= $(shell echo `getconf _NPROCESSORS_ONLN`)
KMPP_MAKE_JOBS := $(shell expr $(MAKE_JOBS) / 2)

EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/kmpp/vcodec/inc
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/kmpp/vcodec/mpp/base/inc
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/kmpp/vcodec/mpp/codec/enc/h264e
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/kmpp/vcodec/mpp/codec/inc
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/kmpp/vcodec/mpp/common
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/kmpp/vcodec/mpp/hal/common
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/kmpp/vcodec/mpp/hal/common/h264
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/kmpp/vcodec/mpp/hal/common/h265
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/kmpp/vcodec/mpp/hal/common/jpeg
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/kmpp/vcodec/mpp/hal/inc
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/kmpp/vcodec/mpp/hal/rkenc/common
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/kmpp/vcodec/mpp/hal/rkenc/h264e
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/kmpp/vcodec/mpp/hal/rkenc/h265e
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/kmpp/vcodec/mpp/hal/rkenc/jpege
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/kmpp/vcodec/mpp/inc
EXTRA_CFLAGS += -I$(KERNEL_DIR)/drivers/kmpp/vcodec/osal/inc

ifeq ($(RK_ENABLE_FASTBOOT), y)
EXTRA_CFLAGS += -fno-verbose-asm
TOP := $(src)
else
TOP := $(PWD)
endif

ifeq ($(CONFIG_CPU_RV1106), y)
VEPU_CORE := RKVEPU540C
lib-m += vproc/pp/vepu_pp.s
endif

ifeq ($(CONFIG_CPU_RV1103B), y)
VEPU_CORE := RKVEPU500
lib-m += vproc/pp/vepu500_pp.s
endif

ifeq ($(CONFIG_CPU_RK3576), y)
VEPU_CORE := RKVEPU510
endif

BUILD_ONE_KO=y
KMPP_TEST_ENABLE=n

ifeq ($(CPU_TYPE), arm64)
	export PREB_KO := ./prebuild/ko_64
	export REL_KO  := ./prebuild/ko_64_rel
else
	export PREB_KO := ./prebuild/ko_32
	export REL_KO  := ./prebuild/ko_32_rel
endif

VCODEC_GIT_REVISION := \
	$(shell cd $(TOP) && git log -1 --no-decorate --date=short \
	--pretty=format:"%h author: %<|(30)%an %cd %s" -- . || \
	echo -n "unknown mpp version for missing VCS info")

VCODEC_REVISION_0 := $(subst \,\\\,$(VCODEC_GIT_REVISION))
VCODEC_REVISION   := $(subst ",\\\",$(VCODEC_REVISION_0))

# add git hooks
$(shell [ -d "$(TOP)/.git/" ] && [ -d "$(TOP)/tools/hooks" ] && cp -rf $(TOP)/tools/hooks $(TOP)/.git/)

ifeq ($(BUILD_ONE_KO), y)
	EXTRA_CFLAGS += -DBUILD_ONE_KO
endif
-include $(TOP)/version.mk
-include $(TOP)/mpp_osal/Makefile
-include $(TOP)/osal/Makefile
-include $(TOP)/sys/Makefile
-include $(TOP)/vproc/Makefile
-include $(TOP)/mpp_service/Makefile
-include $(TOP)/vcodec/Makefile

ifeq ($(KMPP_TEST_ENABLE), y)
-include $(TOP)/test/Makefile
endif

ifeq ($(RK_ENABLE_FASTBOOT), y)
obj-y += ${KMPP_SRC}.o
lib-m += $(${KMPP_SRC}-objs:%.o=%.s)
else
obj-m += ${KMPP_SRC}.o
endif

ifneq ($(SYSDRV_KERNEL_OBJS_OUTPUT_DIR),)
KMPP_BUILD_KERNEL_OBJ_DIR=$(SYSDRV_KERNEL_OBJS_OUTPUT_DIR)
else
KMPP_BUILD_KERNEL_OBJ_DIR=$(KERNEL_DIR)
endif

DIRS := $(shell find . -maxdepth 5 -type d)

FILES = $(foreach dir,$(DIRS),$(wildcard $(dir)/*.c))

OBJS = $(patsubst %.c,%.o, $(FILES))
CMDS = $(foreach dir,$(DIRS),$(wildcard $(dir)/.*.o.cmd))

.PHONY: all clean
all: $(OSTYPE)_build
clean: $(OSTYPE)_clean
install: $(OSTYPE)_install

linux_build: linux_clean
	@echo -e "\e[0;32;1m--Compiling '$(VMPI)'... Configs as follow:\e[0;36;1m"
	@echo ---- CROSS=$(CROSS_COMPILE)
	@echo ---- CPU_TYPE=$(CPU_TYPE)
	@$(call generate_version)
	@$(MAKE) CROSS_COMPILE=${CROSS_COMPILE} ARCH=$(CPU_TYPE) -C $(KERNEL_ROOT) M=$(PWD) modules O=$(KMPP_BUILD_KERNEL_OBJ_DIR) -j$(KMPP_MAKE_JOBS)
	@mkdir -p $(PREB_KO)
	@cp ${KMPP_SRC}.ko $(PREB_KO)
ifneq ($(BUILD_ONE_KO), y)
	@cp rk_vcodec.ko  $(PREB_KO)
	@cp vepu_pp.ko  $(PREB_KO)
endif
	@mkdir -p $(REL_KO)
	@cp ${KMPP_SRC}.ko $(REL_KO)
ifneq ($(BUILD_ONE_KO), y)
	@cp rk_vcodec.ko  $(REL_KO)
	@cp vepu_pp.ko  $(REL_KO)
endif
	@find $(REL_KO) -name "*.ko" | xargs ${CROSS_COMPILE}strip --strip-debug --remove-section=.comment --remove-section=.note --preserve-dates

linux_clean:
	@rm -f *.ko *.mod.c *.o
	@rm -f *.symvers *.order
	@find ./ -name "*.cmd" -o -name "*.s" -o -name "*.o" -o -name "*.mod" | xargs rm -rf
	@rm -rf .tmp_versions
	@rm -f built-in.a lib.a
	@rm -rf $(PREB_KO)
	@rm -rf $(REL_KO)
	@rm -rf $(OBJS)
	@rm -rf $(CMDS)

linux_release:
	find $(REL_KO) -name "*.ko" | xargs ${CROSS_COMPILE}strip --strip-debug --remove-section=.comment --remove-section=.note --preserve-dates
