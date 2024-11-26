# ----------------------------------------------------------------------------
#  Root CMake file for Rockchip Media Process Platform (MPP)
# ----------------------------------------------------------------------------

# vim: syntax=cmake
IF(NOT CMAKE_BUILD_TYPE)
    # default to Release build for GCC builds
    SET(CMAKE_BUILD_TYPE DEBUG CACHE STRING
        "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

CMAKE_MINIMUM_REQUIRED(VERSION 3.12.0)
PROJECT(rk_mpp C)

# ----------------------------------------------------------------------------
# export json compile commands
# ----------------------------------------------------------------------------
SET(CMAKE_EXPORT_COMPILE_COMMANDS true)

# ----------------------------------------------------------------------------
# include common cmake files
# ----------------------------------------------------------------------------
INCLUDE(CMakePrintHelpers)

# ----------------------------------------------------------------------------
# include project cmake files
# ----------------------------------------------------------------------------
INCLUDE(${PROJECT_SOURCE_DIR}/build/cmake/version.cmake)
INCLUDE(${PROJECT_SOURCE_DIR}/build/cmake/kernel.build.cmake)

# ----------------------------------------------------------------------------
# include share common headers
# ----------------------------------------------------------------------------
include_directories(include)
include_directories(mpp_osal)
include_directories(mpp_servicd)
include_directories(dvbm)
include_directories(legacy/inc)
include_directories(${KERNEL_DIR})
include_directories(${KERNEL_DIR}/include/linux)
include_directories(${KERNEL_DIR}/include/uapi)
include_directories(${KERNEL_DIR}/drivers/iommu)

SET(ENV{PATH} "${TOOLCHAIN_DIR}:$ENV{PATH}")
SET(KMPP_TOP ${PROJECT_SOURCE_DIR})

option(BUILD_KMPP_OSAL "Build kernel osal modules" ON)
IF(${BUILD_KMPP_OSAL})
    add_subdirectory("osal")
ENDIF(${BUILD_KMPP_OSAL})

option(BUILD_KMPP_SYS "Build kernel system modules" ON)
IF(${BUILD_KMPP_SYS})
    add_subdirectory("sys")
ENDIF(${BUILD_KMPP_SYS})

option(BUILD_KMPP "Build kernel mpp libraries" ON)
IF(${BUILD_KMPP})
    add_subdirectory("mpp_service")
ENDIF(${BUILD_KMPP})

#option(BUILD_KMPP_VCODEC "Build kernel mpp libraries" OFF)
#IF(${BUILD_KMPP_VCODEC})
#    add_subdirectory("vcodec")
#ENDIF(${BUILD_KMPP_VCODEC})
