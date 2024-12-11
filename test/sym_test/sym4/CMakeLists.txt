# vim: syntax=cmake
INCLUDE_DIRECTORIES(${KMPP_TOP}/osal/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/sys/inc)

SET(SYM4_SRC
    sym4.c
)

ADD_KBUILD(sym4 SRC ${SYM4_SRC} DEPS osal sys)
