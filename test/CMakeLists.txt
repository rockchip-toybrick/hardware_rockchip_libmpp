# vim: syntax=cmake
ADD_SUBDIRECTORY(osal_test)
ADD_SUBDIRECTORY(kmpp_test)
ADD_SUBDIRECTORY(kmpp_enc_test)
ADD_SUBDIRECTORY(kmpi_enc_test)
ADD_SUBDIRECTORY(kmpi_enc_chan_test)
ADD_SUBDIRECTORY(kmpi_vsp_test)

option(BUILD_SYM_TEST "Build kernel mpp cross module symbol test" ON)
IF(${BUILD_SYM_TEST})
    ADD_SUBDIRECTORY(sym_test)
ENDIF(${BUILD_SYM_TEST})

# test stub ko build for stub header
option(BUILD_STUB "Build kernel mpp stub test" OFF)
IF(${BUILD_STUB})
    ADD_SUBDIRECTORY(stub_test)
ENDIF(${BUILD_STUB})
