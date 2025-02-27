# vim: syntax=cmake
INCLUDE_DIRECTORIES(${KMPP_TOP}/osal/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/sys/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/kmpp/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/kmpp/objs/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/kmpp/vsp/inc)

SET(KMPP_SRC
    objs/kmpp_buffer.c
    objs/kmpp_frame.c
    objs/kmpp_meta.c
    objs/kmpp_venc_objs.c
    objs/kmpp_vsp_objs.c
)

ADD_KBUILD(kmpp SRC ${KMPP_SRC} DEPS sys osal)
