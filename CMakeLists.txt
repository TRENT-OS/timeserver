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

# we cannot use CMAKE_CURRENT_LIST_DIR in a macro, because that would give us
# the dir of the file that is invoking the macro. The work around is that we
# create a global variable here and then use this in the macro below. Note that
# we do this, so we don't require anybody else to provide any helper variables,
# this CMake file becomes fully self-contained and is "copy&paste safe".
set(COMPONENT_TIMESERVER_DIR ${CMAKE_CURRENT_LIST_DIR})


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
macro(DeclareCAmkESComponent_TimeServer
    name
)

    if(KernelArchARM)
        set(CAmkESDTS ON CACHE BOOL "" FORCE)
    endif()

    # running CAMkES files though the C pre processor should be enabled by
    # default in the seL4/CAmkES build system and the SDK also enables this,
    # but just to play safe we ensure once more here that it is enabled.
    set(CAmkESCPP ON CACHE BOOL "" FORCE)

    if("${KernelPlatform}" STREQUAL "pc99")
        list(APPEND
             TimeServerPlatSources
             ${COMPONENT_TIMESERVER_DIR}/src/plat/pc99/plat.c)
    endif()

    # The imx8 boards (imx8mm-evk, imx8mq-evk) share the same timer device
    # configuration
    if("${KernelPlatform}" MATCHES "imx8.*")
        set(PlatPrefix "imx8")
    else()
        set(PlatPrefix "${KernelPlatform}")
    endif()

    DeclareCAmkESComponent(
        ${name}
        SOURCES
            ${COMPONENT_TIMESERVER_DIR}/src/TimeServer.c
            ${TimeServerPlatSources}
        C_FLAGS
            -Wall
            -Werror
            # Fix some compiler error
            -Wno-unused-const-variable
    )

    CAmkESAddCPPInclude("${COMPONENT_TIMESERVER_DIR}/include/plat/${PlatPrefix}/")

    #---------------------------------------------------------------------------
    # create the corresponding client project, which every client of this
    # time server instance must include. We do not define this globally,
    # because the clients may have server specific configurations in the
    # future.
    project(${name}_client C)

    add_library(${PROJECT_NAME} INTERFACE)

    target_include_directories(${PROJECT_NAME}
        INTERFACE
            ${COMPONENT_TIMESERVER_DIR}/include
    )

endmacro()
