# vim: syntax=cmake
INCLUDE_DIRECTORIES(${KMPP_TOP}/osal)
INCLUDE_DIRECTORIES(${KMPP_TOP}/osal/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/sys)
INCLUDE_DIRECTORIES(${KMPP_TOP}/sys/inc)

SET(KMPP_SYS_SRC
    kmpp_sys.c
    kmpp_trie.c
    kmpp_dq.c
    kmpp_obj.c
    kmpp_shm.c
    kmpp_sym.c
    kmpp_frame.c
)

ADD_KBUILD(sys SRC ${KMPP_SYS_SRC} DEPS osal)
