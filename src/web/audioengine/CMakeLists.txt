# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-Studio-CLA-applies
#
# MuseScore Studio
# Music Composition & Notation
#
# Copyright (C) 2021-2025 MuseScore Limited
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.22)

project(MuseAudio LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MUSE_MODULE_AUDIO_WORKER "Enable audio worker" OFF)
option(MUSE_FIX_MUSEAUDIO_FILENAME "Fix MuseAudio.js file name" OFF)

# src/web/audioengine
set(MU_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../..)
set(MUSE_FRAMEWORK_PATH ${MU_ROOT})
set(MUSE_FRAMEWORK_SRC_PATH ${MU_ROOT}/src/framework)

if (NOT EXISTS ${MU_ROOT}/buildscripts)
    message(FATAL_ERROR "not root: ${MU_ROOT}")
endif()

set(CMAKE_MODULE_PATH
    ${MU_ROOT}
    ${MU_ROOT}/buildscripts
    ${MU_ROOT}/buildscripts/cmake
    ${MU_ROOT}/src/framework/cmake
    ${CMAKE_MODULE_PATH}
    )

set(PARENT_BUILD_CACHE ${MU_ROOT}/build.release/CMakeCache.txt)
if (EXISTS ${PARENT_BUILD_CACHE})
    file(STRINGS ${PARENT_BUILD_CACHE} PARENT_QT_CACHE_LINES REGEX "^(QT_HOST_PATH(|_CMAKE_DIR)|Qt6[^:]*_DIR):PATH=")
    foreach(PARENT_QT_CACHE_LINE IN LISTS PARENT_QT_CACHE_LINES)
        string(REGEX REPLACE "^([^:]+):PATH=(.*)$" "\\1;\\2" PARENT_QT_CACHE_ENTRY ${PARENT_QT_CACHE_LINE})
        list(LENGTH PARENT_QT_CACHE_ENTRY PARENT_QT_CACHE_ENTRY_LEN)
        if (PARENT_QT_CACHE_ENTRY_LEN EQUAL 2)
            list(GET PARENT_QT_CACHE_ENTRY 0 PARENT_QT_CACHE_KEY)
            list(GET PARENT_QT_CACHE_ENTRY 1 PARENT_QT_CACHE_VALUE)
            if ((NOT DEFINED ${PARENT_QT_CACHE_KEY}) OR ("${${PARENT_QT_CACHE_KEY}}" STREQUAL "") OR ("${${PARENT_QT_CACHE_KEY}}" MATCHES "-NOTFOUND$"))
                set(${PARENT_QT_CACHE_KEY} ${PARENT_QT_CACHE_VALUE} CACHE PATH "Inherited from parent WASM build cache" FORCE)
            endif()
        endif()
    endforeach()
endif()

include(GetPlatformInfo)
include(${MUSE_FRAMEWORK_SRC_PATH}/cmake/MuseDeclareOptions.cmake)

set(MUSESCORE_BUILD_CONFIGURATION "app-web" CACHE STRING "Build configuration")
set(MUSE_APP_BUILD_MODE "dev" CACHE STRING "Build mode")

set(QT_ADD_LINGUISTTOOLS OFF)
set(QT_ADD_CONCURRENT OFF)
set(QT_ADD_WEBSOCKET OFF)
set(QT_QPROCESS_SUPPORTED OFF)
set(QT_CONCURRENT_SUPPORTED OFF)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

set(MUSE_QT_SUPPORT ON)
set(MUSE_CONFIGURATION_IS_WEB ON)
set(MUSE_THREADS_SUPPORT OFF)
set(MUSE_MODULE_AUDIO_EXPORT OFF)
set(MUSE_ENABLE_UNIT_TESTS OFF)
set(MUSE_MODULE_GLOBAL_TESTS OFF)
set(MUSE_COMPILE_USE_PCH OFF)

include(SetupBuildEnvironment)
include(SetupQt6)

include(MuseCreateModule)
include(SetupCompileWarnings)
include(GetBuildType)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${MU_ROOT}/build.artifacts)

set(EMCC_COMPILE_FLAGS "-s USE_ZLIB=1 -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMCC_COMPILE_FLAGS}")

if (BUILD_IS_DEBUG)
    set(EMCC_LINKER_FLAGS -O0)
else()
    set(EMCC_LINKER_FLAGS -Os)
endif()

set(AUDIO_ROOT ${MU_ROOT}/src/framework/audio)
add_subdirectory(${AUDIO_ROOT}/common common)
set(WEBWORKER_FACADE_MODE OFF)
add_subdirectory(${AUDIO_ROOT}/engine engine)

add_executable(MuseAudio
    ${CMAKE_CURRENT_LIST_DIR}/engine_export.cpp
    ${CMAKE_CURRENT_LIST_DIR}/webaudioengine.cpp
    ${CMAKE_CURRENT_LIST_DIR}/webaudioengine.h

    ${AUDIO_ROOT}/driver/platform/web/webaudiochannel.cpp
    ${AUDIO_ROOT}/driver/platform/web/webaudiochannel.h
)

target_include_directories(MuseAudio PRIVATE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}

    ${PROJECT_SOURCE_DIR}/src

    ${MUSE_FRAMEWORK_PATH}
    ${MUSE_FRAMEWORK_PATH}/framework
    ${MUSE_FRAMEWORK_PATH}/framework/global
    ${MUSE_FRAMEWORK_PATH}/framework/audio

    # compat
    ${MUSE_FRAMEWORK_PATH}/src
    ${MUSE_FRAMEWORK_PATH}/src/framework
    ${MUSE_FRAMEWORK_PATH}/src/framework/global
    ${MUSE_FRAMEWORK_PATH}/src/framework/audio
    # end compat
)

add_definitions(-DQ_OS_WASM)
set(GLOBAL_NO_INTERNAL ON)

configure_file(${MU_ROOT}/src/framework/cmake/muse_framework_config.h.in muse_framework_config.h )

add_subdirectory(${MU_ROOT}/src/framework/global global)

target_link_libraries(MuseAudio PRIVATE
    muse_audio_common
    muse_audio_engine
    embind
    muse::global
)

#set_target_properties(MuseAudio PROPERTIES OUTPUT_NAME "MuseAudio.wasm")
#target_link_options(MuseAudio PRIVATE "SHELL:-o MuseAudio.wasm")
target_link_options(MuseAudio PRIVATE ${EMCC_LINKER_FLAGS})
target_link_options(MuseAudio PRIVATE "SHELL:-s EXPORT_NAME=MuseAudio_entry")
target_link_options(MuseAudio PRIVATE "SHELL:-s MODULARIZE=1")
target_link_options(MuseAudio PRIVATE "SHELL:-s FETCH=1")
target_link_options(MuseAudio PRIVATE "SHELL:-s INITIAL_MEMORY=50MB")
target_link_options(MuseAudio PRIVATE "SHELL:-s ALLOW_MEMORY_GROWTH=1")
target_link_options(MuseAudio PRIVATE "SHELL:-s EXPORTED_RUNTIME_METHODS=ccall,cwrap,HEAPU8")
target_link_options(MuseAudio PRIVATE "SHELL:-s EXPORTED_FUNCTIONS=[_main,_malloc,_free,_Init,_process]")

if (MUSE_MODULE_AUDIO_WORKER)
    target_link_options(MuseAudio PRIVATE "SHELL:-s ENVIRONMENT=worker")
else()
    target_link_options(MuseAudio PRIVATE "SHELL:-s ENVIRONMENT=shell")
    target_link_options(MuseAudio PRIVATE "SHELL:-s BINARYEN_ASYNC_COMPILATION=0")
    target_link_options(MuseAudio PRIVATE "SHELL:-s SINGLE_FILE=1")
   # target_link_options(MuseAudio PRIVATE "SHELL:-s EXPORT_ES6=1")
endif()


if (MUSE_FIX_MUSEAUDIO_FILENAME)
    add_custom_command(
        TARGET MuseAudio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:MuseAudio>"
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/MuseAudio.js"
        COMMENT "Fix output js file name"
    )
endif()
