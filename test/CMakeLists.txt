# vim: syntax=cmake
ADD_SUBDIRECTORY(osal_test)
ADD_SUBDIRECTORY(kmpp_test)
ADD_SUBDIRECTORY(kmpp_enc_test)

option(BUILD_SYM_TEST "Build kernel mpp cross module symbol test" ON)
IF(${BUILD_SYM_TEST})
    ADD_SUBDIRECTORY(sym_test)
ENDIF(${BUILD_SYM_TEST})
