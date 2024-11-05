# vim: syntax=cmake

FUNCTION(ADD_KBUILD MODULE_NAME)
    SET(args ${ARGN})
    SET(add_src 0)
    SET(add_dep 0)

    SET(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

    SET(MODULE_KO "${MODULE_NAME}.ko")
    SET(MODULE_DIR "${CMAKE_CURRENT_BINARY_DIR}/build")
    SET(MODULE_SRCS "")
    SET(MODULE_DEPS "")
    SET(MODULE_OBJS "")
    SET(MODULE_CFLAG "")
    SET(MODULE_EXTRA_SYM "")
    SET(MODULE_BYPRODUCTS "")

    FOREACH(arg IN LISTS args)
        IF(arg STREQUAL "SRC")
            SET(add_src 1)
            SET(add_dep 0)
        ELSEIF(arg STREQUAL "DEPS")
            SET(add_src 0)
            SET(add_dep 1)
        ELSE()
            IF(add_src)
                LIST(APPEND MODULE_SRCS "${arg}")
            ELSEIF(add_dep)
                LIST(APPEND MODULE_DEPS "${arg}")
            ENDIF()
        ENDIF()
    ENDFOREACH()

    STRING(STRIP "${MODULE_SRCS}" MODULE_SRCS)
    STRING(STRIP "${MODULE_DEPS}" MODULE_DEPS)

    FILE(MAKE_DIRECTORY ${MODULE_DIR})

    GET_DIRECTORY_PROPERTY(DIR_DEFS COMPILE_DEFINITIONS)
    FOREACH(d ${DIR_DEFS})
        SET(MODULE_CFLAG "${MODULE_CFLAG} -D${d}")
    ENDFOREACH()

    GET_DIRECTORY_PROPERTY(DIR_DEFS INCLUDE_DIRECTORIES )
    FOREACH(d ${DIR_DEFS})
        SET(MODULE_CFLAG "${MODULE_CFLAG} -I${d}")
    ENDFOREACH()

    SET(MODULE_CFLAG "${MODULE_CFLAG} -mno-outline-atomics ")

    FOREACH(src ${MODULE_SRCS})
        IF(NOT IS_ABSOLUTE ${src})
            SET(src "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
        ENDIF()

        GET_FILENAME_COMPONENT(name ${src} NAME)

        FILE(RELATIVE_PATH dst_rel_path ${SOURCE_DIR} ${src})
        GET_FILENAME_COMPONENT(rel_dir ${dst_rel_path} DIRECTORY)

        IF(NOT EXISTS "${rel_dir}")
            FILE(MAKE_DIRECTORY ${MODULE_DIR}/${rel_dir})
        ENDIF()

        SET(dst "${MODULE_DIR}/${dst_rel_path}")

        EXECUTE_PROCESS(COMMAND bash -c "ln -sf ${src} ${dst}")

        GET_FILENAME_COMPONENT(name_we ${dst} NAME_WE)
        SET(obj "${rel_dir}/${name_we}.o")

        STRING(APPEND MODULE_OBJS "${obj} ")

        LIST(APPEND MODULE_BYPRODUCTS
            "${MODULE_DIR}/${obj}"
            "${MODULE_DIR}/${obj}.cmd"
        )
    ENDFOREACH()

    FOREACH(d ${MODULE_DEPS})
        GET_PROPERTY(dep_dir TARGET ${d} PROPERTY LOCATION)
        STRING(APPEND MODULE_EXTRA_SYM "${dep_dir}/Module.symvers ")
    ENDFOREACH()

    LIST(APPEND MODULE_BYPRODUCTS
        "${MODULE_DIR}/.${MODULE_KO}.cmd"
        "${MODULE_DIR}/.${MODULE_NAME}.mod.o.cmd"
        "${MODULE_DIR}/.${MODULE_NAME}.o.cmd"
        "${MODULE_DIR}/.built-in.o.cmd"
        "${MODULE_DIR}/${MODULE_NAME}.mod.c"
        "${MODULE_DIR}/${MODULE_NAME}.mod.o"
        "${MODULE_DIR}/${MODULE_NAME}.o"
        "${MODULE_DIR}/built-in.o"
        "${MODULE_DIR}/Module.symvers"
        "${MODULE_DIR}/modules.order"
        )

    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/build/cmake/Kbuild.in ${MODULE_DIR}/Kbuild @ONLY)

    SET(KBUILD_CMD
        PATH=$ENV{PATH}
        ${CMAKE_MAKE_PROGRAM}
        modules
        V=1
        -C ${KERNEL_DIR}
        ARCH=arm64
        CROSS_COMPILE=${CROSS_COMPILER_PREFIX}
        ${KERNEL_MAKE_OPT}
        M=${MODULE_DIR}
        src=${MODULE_DIR}
        )

    ADD_CUSTOM_COMMAND(
        COMMAND ${KBUILD_CMD}
        COMMENT "Building ${MODULE_KO}"
        OUTPUT ${MODULE_DIR}/${MODULE_KO}
        DEPENDS ${MODULE_SRCS}
        WORKING_DIRECTORY ${MODULE_DIR}
        BYPRODUCTS ${MODULE_BYPRODUCTS}
        VERBATIM
        )

    ADD_CUSTOM_TARGET(
        ${MODULE_NAME} ALL
        COMMAND ${KBUILD_CMD}
        DEPENDS ${MODULE_DIR}/${MODULE_KO}
        DEPENDS ${MODULE_DEPS}
        )

    SET_TARGET_PROPERTIES(
        ${MODULE_NAME}
        PROPERTIES
        LOCATION            ${MODULE_DIR}
        OUTPUT_NAME         ${MODULE_NAME}
        MODULE_KO           ""
        MODULE_DIR          ""
        MODULE_SRCS         ""
        MODULE_DEPS         ""
        MODULE_OBJS         ""
        MODULE_CFLAG        ""
        MODULE_EXTRA_SYM    ""
        MODULE_BYPRODUCTS   ""
        )
ENDFUNCTION()

