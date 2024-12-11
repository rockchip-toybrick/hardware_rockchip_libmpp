# vim: syntax=cmake
INCLUDE_DIRECTORIES(${KMPP_TOP}/osal/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/sys/inc)

SET(SYM2_SRC
    sym2.c
)

ADD_KBUILD(sym2 SRC ${SYM2_SRC} DEPS osal sys)
