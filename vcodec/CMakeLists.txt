# vim: syntax=cmake

INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/base/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/codec)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/codec/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/codec/rc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/codec/enc/h264e)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/codec/enc/h265e)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/codec/enc/jpeg)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/common)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/hal/common)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/hal/common/h264)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/hal/common/h265)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/hal/common/jpeg)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/hal/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/hal/rkenc/common)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/hal/rkenc/h264e)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/hal/rkenc/h265e)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/hal/rkenc/jpege)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/mpp/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vcodec/osal/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vproc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/vproc/pp)
INCLUDE_DIRECTORIES(${KMPP_TOP}/mpp)

ADD_DEFINITIONS(-DBUILD_ONE_KO)

SET(SOC_TYPE "RV1103B")

IF(${SOC_TYPE} STREQUAL "RV1103B")
    SET(VEPU_PP_FILE ../vproc/pp/vepu500_pp.c)
ELSE()
    SET(VEPU_PP_FILE ../vproc/pp/vepu_pp.c)
ENDIF()

SET(VCODEC_SRC
    ${VEPU_PP_FILE}
    ../vproc/pp/vepu_pp_service.c
    ../mpp/mpp_iommu.c
    ../mpp/mpp_service.c
    ../mpp/mpp_common.c
    mpp/hal/mpp_enc_hal.c
    mpp/hal/rkenc/h264e/hal_h264e_scaling_list.c
    #mpp/hal/rkenc/h264e/hal_h264e_vepu580.c
    mpp/hal/rkenc/h264e/hal_h264e_vepu500.c
    mpp/hal/rkenc/h264e/hal_h264e_vepu540c.c
    mpp/hal/rkenc/common/vepu541_common.c
    mpp/hal/rkenc/common/vepu5xx_common.c
    mpp/hal/rkenc/common/vepu540c_common.c
    mpp/hal/rkenc/common/vepu500_common.c
    #mpp/hal/rkenc/h265e/hal_h265e_vepu580.c
    #mpp/hal/rkenc/h265e/hal_h265e_vepu500.c
    #mpp/hal/rkenc/h265e/hal_h265e_vepu541.c
    mpp/hal/rkenc/h265e/hal_h265e_scaling_list.c
    mpp/hal/rkenc/h265e/hal_h265e_vepu540c.c
    mpp/hal/rkenc/jpege/hal_jpege_vepu540c.c
    mpp/hal/rkenc/jpege/hal_jpege_vepu500.c
    mpp/hal/common/h264/hal_h264e_api_v2.c
    mpp/hal/common/h265/hal_h265e_api_v2.c
    mpp/hal/common/hal_bufs.c
    mpp/hal/common/jpeg/hal_jpege_api_v2.c
    mpp/hal/common/jpeg/hal_jpege_hdr.c
    mpp/base/mpp_enc_refs.c
    mpp/base/mpp_frame.c
    mpp/base/mpp_trie.c
    mpp/base/mpp_enc_cfg.c
    mpp/base/mpp_packet.c
    mpp/base/mpp_bitwrite.c
    mpp/base/mpp_enc_ref.c
    mpp/base/mpp_enc_refs_setup.c
    mpp/base/mpp_buffer.c
    mpp/base/mpp_stream_ring_buf.c
    mpp/base/mpp_bitread.c
    mpp/base/mpp_mem_pool.c
    mpp/base/mpp_cfg.c
    mpp/base/mpp_2str.c
    mpp/codec/enc_impl.c
    mpp/codec/mpp_enc.c
    mpp/codec/rc/mpp_rc.c
    mpp/codec/rc/h265e_rc.c
    mpp/codec/rc/rc.c
    mpp/codec/rc/h264e_rc.c
    mpp/codec/rc/jpege_rc.c
    mpp/codec/rc/rc_model_v2.c
    mpp/codec/rc/rc_model_v2_smt.c
    mpp/codec/rc/rc_base.c
    mpp/codec/enc/h264e/h264e_pps.c
    mpp/codec/enc/h264e/h264e_api_v2.c
    mpp/codec/enc/h264e/h264e_dpb.c
    mpp/codec/enc/h264e/h264e_sps.c
    mpp/codec/enc/h264e/h264e_sei.c
    mpp/codec/enc/h264e/h264e_slice.c
    mpp/codec/enc/jpeg/jpege_api_v2.c
    mpp/codec/enc/h265e/h265e_ps.c
    mpp/codec/enc/h265e/h265e_api.c
    mpp/codec/enc/h265e/h265e_dpb.c
    mpp/codec/enc/h265e/h265e_header_gen.c
    mpp/codec/enc/h265e/h265e_enctropy.c
    mpp/codec/enc/h265e/h265e_stream.c
    mpp/codec/enc/h265e/h265e_syntax.c
    mpp/codec/enc/h265e/h265e_slice.c
    mpp/codec/mpp_enc_impl.c
    mpp_vcodec_event.c
    mpp_vcodec_chan.c
    mpp_vcodec_flow.c
    mpp_vcodec_driver.c
    mpp_vcodec_base.c
    osal/mpp_log.c
    osal/mpp_time.c
    osal/osal_mpp_service.c
    osal/mpp_device.c
    osal/mpp_maths.c
    osal/mpp_mem.c
    mpp_vcodec_thread.c
    mpp_vcodec_test.c
)

ADD_KBUILD(vcodec SRC ${VCODEC_SRC})
