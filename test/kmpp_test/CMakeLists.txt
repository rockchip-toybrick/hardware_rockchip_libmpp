# vim: syntax=cmake
INCLUDE_DIRECTORIES(${KMPP_TOP}/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/osal/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/kmpp/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/kmpp/base/inc)

SET(KMPP_TEST_SRC
    kmpp_test.c
)

ADD_KBUILD(kmpp_test SRC ${KMPP_TEST_SRC} DEPS osal sys)
