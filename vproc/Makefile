# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

$(VEPU_CORE)_PP_ENABLE = y
EXTRA_CFLAGS += -D$(VEPU_CORE)_PP_ENABLE
vepu_pp-objs := vproc/pp/vepu_pp_service.o

vepu_pp-$(RKVEPU540C_PP_ENABLE) += vproc/pp/vepu_pp.o
vepu_pp-$(RKVEPU500_PP_ENABLE) += vproc/pp/vepu500_pp.o
ifneq ($(BUILD_ONE_KO), y)
	obj-m += vepu_pp.o
endif
