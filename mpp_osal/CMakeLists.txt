# vim: syntax=cmake
INCLUDE_DIRECTORIES(${KMPP_TOP}/inc)
INCLUDE_DIRECTORIES(${KMPP_TOP}/osal/inc)

SET(KMPP_OSAL_SRC
    ${KMPP_TOP}/osal/mpp_osal.c
)

#ADD_KBUILD(kmpp_osal SRC ${KMPP_OSAL_SRC})
