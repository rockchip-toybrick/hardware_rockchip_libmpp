# vim: syntax=cmake
INCLUDE_DIRECTORIES(${KMPP_TOP}/osal/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/sys/inc)

SET(SYM_TEST_SRC
    sym_test.c
)

ADD_KBUILD(sym_test SRC ${SYM_TEST_SRC} DEPS osal sys)
