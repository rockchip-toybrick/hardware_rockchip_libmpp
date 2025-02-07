# vim: syntax=cmake
INCLUDE_DIRECTORIES(${KMPP_TOP}/osal/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/sys/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/kmpp/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/kmpp/objs/inc)

SET(KMPP_SRC
    objs/kmpp_venc_objs.c
    objs/kmpp_meta.c
    objs/kmpp_frame.c
)

ADD_KBUILD(kmpp SRC ${KMPP_SRC} DEPS sys osal)
