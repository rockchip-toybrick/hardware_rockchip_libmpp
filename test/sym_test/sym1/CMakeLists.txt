# vim: syntax=cmake
INCLUDE_DIRECTORIES(${KMPP_TOP}/osal/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/sys/inc)

SET(SYM1_SRC
    sym1.c
)

ADD_KBUILD(sym1 SRC ${SYM1_SRC} DEPS osal sys)
