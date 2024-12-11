# vim: syntax=cmake
INCLUDE_DIRECTORIES(${KMPP_TOP}/osal/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/sys/inc)

SET(SYM3_SRC
    sym3.c
)

ADD_KBUILD(sym3 SRC ${SYM3_SRC} DEPS osal sys)
