#
# TimeServer
#
# Copyright (C) 2020-2024, HENSOLDT Cyber GmbH
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.17)

#-------------------------------------------------------------------------------
#
# Declare TimeServer CAmkES Component
#
# Parameters:
#
#   <name>
#     component instance name. The server will be called "<name>" and the
#     corresponding client library will be called "<name>_client"
#
function(TimeServer_DeclareCAmkESComponent
    name
)

    if(KernelArchARM)
        set(CAmkESDTS ON CACHE BOOL "" FORCE)
    endif()

    # running CAMkES files though the C pre processor should be enabled by
    # default in the seL4/CAmkES build system and the SDK also enables this,
    # but just to play safe we ensure once more here that it is enabled.
    set(CAmkESCPP ON CACHE BOOL "" FORCE)

    # The imx8 boards (imx8mm-evk, imx8mq-evk) share the same timer device
    # configuration
    if("${KernelPlatform}" MATCHES "imx8.*")
        set(OS_COMPONENT_TimeServer_PLAT_INC "imx8")
    else()
        set(OS_COMPONENT_TimeServer_PLAT_INC "${KernelPlatform}")
    endif()
    CAmkESAddCPPInclude(
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/server/include/plat/${OS_COMPONENT_TimeServer_PLAT_INC}"
    )

    set(OS_COMPONENT_TimeServer_PLAT_SOURCES "")
    if("${KernelPlatform}" STREQUAL "pc99")
        list(
            APPEND
            OS_COMPONENT_TimeServer_PLAT_SROURCES
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/server/src/plat/pc99/plat.c)
    endif()

    DeclareCAmkESComponent(
        ${name}
        SOURCES
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/server/src/timeServer_rpc.c
            ${OS_COMPONENT_TimeServer_PLAT_SROURCES}
        C_FLAGS
            -Wall
            -Werror
            # Fix some compiler error
            -Wno-unused-const-variable
        LIBS
            os_core_api
            lib_debug
            lib_macros
    )

    #---------------------------------------------------------------------------
    # create the corresponding client project, which every client of this
    # time server instance must include. We do not define this globally,
    # because the clients may have server specific configurations in the
    # future.
    project(${name}_client C)

    add_library(${PROJECT_NAME} INTERFACE)

    target_sources(${PROJECT_NAME}
        INTERFACE
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/client/src/TimeServer.c
    )

    target_include_directories(${PROJECT_NAME}
        INTERFACE
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/client/include
    )

endfunction()
