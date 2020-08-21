#
# Copyright 2018, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

cmake_minimum_required(VERSION 3.8.2)

#
# TimeServer
#
# Copyright (C) 2020, Hensoldt Cyber GmbH
#

# we cannot use CMAKE_CURRENT_LIST_DIR in a function or macro, because that
# would give us the dir of the file that is invoking the macro. The work around
# is that we create a global (ie cached and flagged as internal) variable here
# and then use it in the macro below. As a convenient side effect we don't
# require anybody else to provide any helper variables, this CMake file becomes
# fully self-contained and "copy&paste safe".
set(
    OS_COMPONENT_TimeServer_DIR
    "${CMAKE_CURRENT_LIST_DIR}"
    CACHE INTERNAL ""
)


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
function(DeclareCAmkESComponent_TimeServer
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
        "${OS_COMPONENT_TimeServer_DIR}/server/include/plat/${OS_COMPONENT_TimeServer_PLAT_INC}"
    )

    set(OS_COMPONENT_TimeServer_PLAT_SOURCES "")
    if("${KernelPlatform}" STREQUAL "pc99")
        list(
            APPEND
            OS_COMPONENT_TimeServer_PLAT_SROURCES
            ${OS_COMPONENT_TimeServer_DIR}/server/src/plat/pc99/plat.c)
    endif()

    DeclareCAmkESComponent(
        ${name}
        SOURCES
            ${OS_COMPONENT_TimeServer_DIR}/server/src/timeServer_rpc.c
            ${OS_COMPONENT_TimeServer_PLAT_SROURCES}
        C_FLAGS
            -Wall
            -Werror
            # Fix some compiler error
            -Wno-unused-const-variable
        LIBS
            os_core_api
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
            ${OS_COMPONENT_TimeServer_DIR}/client/src/TimeServer.c
    )

    target_include_directories(${PROJECT_NAME}
        INTERFACE
            ${OS_COMPONENT_TimeServer_DIR}/client/include
    )

endfunction()
