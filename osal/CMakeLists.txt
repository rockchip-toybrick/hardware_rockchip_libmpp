# vim: syntax=cmake
INCLUDE_DIRECTORIES(${KMPP_TOP}/osal)
INCLUDE_DIRECTORIES(${KMPP_TOP}/osal/inc)

SET(KMPP_OSAL_SRC
    kmpp_atomic.c
    kmpp_cfg.c
    kmpp_clk.c
    kmpp_completion.c
    kmpp_dev.c  
    kmpp_env.c
    kmpp_io.c
    kmpp_mem.c  
    kmpp_osal.c 
    kmpp_rwsem.c
    kmpp_spinlock.c 
    kmpp_string.c
    kmpp_time.c
    kmpp_bitops.c
    kmpp_cls.c
    kmpp_delay.c    
    kmpp_dmabuf.c
    kmpp_fs.c
    kmpp_log.c
    kmpp_mutex.c
    kmpp_pm.c   
    kmpp_sema.c 
    kmpp_sthd.c     
    kmpp_thread.c
    kmpp_trie.c
    kmpp_wait.c
)

ADD_KBUILD(osal SRC ${KMPP_OSAL_SRC})
