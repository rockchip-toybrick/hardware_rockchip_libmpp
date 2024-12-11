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
INCLUDE_DIRECTORIES(include)
INCLUDE_DIRECTORIES(mpp_osal)
INCLUDE_DIRECTORIES(mpp_service)
INCLUDE_DIRECTORIES(sys/inc)
INCLUDE_DIRECTORIES(dvbm)
INCLUDE_DIRECTORIES(legacy/inc)
INCLUDE_DIRECTORIES(${KERNEL_DIR})
INCLUDE_DIRECTORIES(${KERNEL_DIR}/include/linux)
INCLUDE_DIRECTORIES(${KERNEL_DIR}/include/uapi)
INCLUDE_DIRECTORIES(${KERNEL_DIR}/drivers/iommu)

SET(ENV{PATH} "${TOOLCHAIN_DIR}:$ENV{PATH}")
SET(KMPP_TOP ${PROJECT_SOURCE_DIR})

optiON(BUILD_KMPP_OSAL "Build kernel osal modules" ON)
IF(${BUILD_KMPP_OSAL})
    ADD_SUBDIRECTORY("osal")
ENDIF(${BUILD_KMPP_OSAL})

optioN(BUILD_KMPP_SYS "Build kernel system modules" ON)
IF(${BUILD_KMPP_SYS})
    ADD_SUBDIRECTORY("sys")
ENDIF(${BUILD_KMPP_SYS})

OPTION(BUILD_KMPP "Build kernel mpp libraries" ON)
IF(${BUILD_KMPP})
    ADD_SUBDIRECTORY("mpp_service")
ENDIF(${BUILD_KMPP})

OPTION(BUILD_TEST "Build kernel mpp test" ON)
IF(${BUILD_TEST})
    ADD_SUBDIRECTORY("test")
ENDIF(${BUILD_TEST})
