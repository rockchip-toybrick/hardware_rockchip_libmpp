# vim: syntax=cmake
INCLUDE_DIRECTORIES(${KMPP_TOP}/osal/inc)

SET(OSAL_TEST_SRC
    osal_test.c
)

ADD_KBUILD(osal_test SRC ${OSAL_TEST_SRC} DEPS osal)
