# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-Studio-CLA-applies
#
# MuseScore Studio
# Music Composition & Notation
#
# Copyright (C) 2021 MuseScore Limited
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

muse_create_module(appjs)

target_sources(appjs PRIVATE
    appjsmodule.cpp
    appjsmodule.h
    webapi.cpp
    webapi.h
)

set(MUSE_AUDIO_BUILD_DIR ${CMAKE_SOURCE_DIR}/build.audio.release)
set(MUSE_AUDIO_OUTPUT ${CMAKE_SOURCE_DIR}/build.artifacts/MuseAudio.js)
set(MUSE_AUDIO_STAMP ${CMAKE_CURRENT_BINARY_DIR}/museaudio-build.stamp)
set(MUSE_EMSDK_TOOLCHAIN ${CMAKE_CXX_COMPILER})
get_filename_component(MUSE_EMSDK_TOOLCHAIN_DIR ${MUSE_EMSDK_TOOLCHAIN} DIRECTORY)
set(MUSE_EMSCRIPTEN_TOOLCHAIN_FILE ${MUSE_EMSDK_TOOLCHAIN_DIR}/cmake/Modules/Platform/Emscripten.cmake)

add_custom_command(
    OUTPUT ${MUSE_AUDIO_STAMP}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${MUSE_AUDIO_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND}
            -S ${CMAKE_SOURCE_DIR}/src/web/audioengine
            -B ${MUSE_AUDIO_BUILD_DIR}
            -G Ninja
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DQT_CHAINLOAD_TOOLCHAIN_FILE=${MUSE_EMSCRIPTEN_TOOLCHAIN_FILE}
            -DMUSESCORE_BUILD_CONFIGURATION=app-web
            -DMUSE_APP_BUILD_MODE=dev
            -DMUSE_FIX_MUSEAUDIO_FILENAME=ON
            -DMUSE_MODULE_AUDIO_WORKER=OFF
    COMMAND ${CMAKE_COMMAND} --build ${MUSE_AUDIO_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${MUSE_AUDIO_STAMP}
    DEPENDS
            ${CMAKE_SOURCE_DIR}/src/web/audioengine/CMakeLists.txt
            ${CMAKE_SOURCE_DIR}/src/web/audioengine/engine_export.cpp
            ${CMAKE_SOURCE_DIR}/src/web/audioengine/webaudioengine.cpp
            ${CMAKE_SOURCE_DIR}/src/web/audioengine/webaudioengine.h
    BYPRODUCTS ${MUSE_AUDIO_OUTPUT}
    COMMENT "Building standalone MuseAudio WASM runtime"
    VERBATIM
)

add_custom_target(MuseAudioRuntime ALL DEPENDS ${MUSE_AUDIO_STAMP})
add_dependencies(appjs MuseAudioRuntime)
