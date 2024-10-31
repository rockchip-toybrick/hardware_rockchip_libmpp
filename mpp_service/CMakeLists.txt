# vim: syntax=cmake
INCLUDE_DIRECTORIES(${KMPP_TOP}/include)
INCLUDE_DIRECTORIES(${KMPP_TOP}/osal/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/mpp_osal)
INCLUDE_DIRECTORIES(${KMPP_TOP}/mpp_service)

SET(KMPP_SRC
    mpp_av1dec.c
    mpp_iep2.c
    mpp_jpgdec.c
    mpp_jpgenc.c
    mpp_rkvdec2.c
    mpp_rkvdec2_link.c
    mpp_rkvdec.c
    mpp_rkvenc2.c
    mpp_rkvenc.c
    mpp_service.c
    mpp_vdpp.c
    mpp_vdpu1.c
    mpp_vdpu2.c
    mpp_vepu1.c
    mpp_vepu2.c
    mpp_iommu.c
    mpp_service.c
    mpp_common.c
    ../mpp_osal/mpp_osal.c
)

ADD_KBUILD(kmpp
           SRC ${KMPP_SRC})
