# ArcVideoFoundation - Core library for ArcVideo
# Copyright (C) 2023 Olive Studios LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 4.2 FATAL_ERROR)

project(arcvideo-foundation VERSION 1.0.0 LANGUAGES CXX)

# Global settings only when building standalone
if(PROJECT_IS_TOP_LEVEL)
  option(USE_WERROR "Error on compile warning" OFF)

  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  # Output directories
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

# --- Compiler options ---
# On clang-cl, /W4 is the correct equivalent of GCC -Wall -Wextra.
if(MSVC)
  set(FOUNDATION_COMPILE_OPTIONS
    /W4
    /EHsc
    "$<$<COMPILE_LANGUAGE:CXX>:/MP>"
    "$<$<CONFIG:Release>:/O2>"
  )
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # clang-cl: suppress only FASTBuild and MSVC-compat noise
    list(APPEND FOUNDATION_COMPILE_OPTIONS
      -Wno-unused-command-line-argument
      -Wno-unused-parameter
      -Wno-microsoft-include
      -Wno-gnu-line-marker
    )
  else()
    # Native cl.exe: use /external:W0 to suppress third-party header warnings
    list(APPEND FOUNDATION_COMPILE_OPTIONS
      /external:anglebrackets
      /external:W0
      /wd4100
    )
  endif()
  if(USE_WERROR)
    list(APPEND FOUNDATION_COMPILE_OPTIONS /WX)
  endif()
else()
  set(FOUNDATION_COMPILE_OPTIONS
    "$<$<CONFIG:Release>:-O2>"
    -Wuninitialized
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-gnu-line-marker
    -Wshadow
  )
  if(USE_WERROR)
    list(APPEND FOUNDATION_COMPILE_OPTIONS -Werror)
  endif()
endif()

# --- Dependencies ---
# Only when building standalone (not as subdirectory of ArcVideo)
if(PROJECT_IS_TOP_LEVEL)
  # OpenEXR / Imath (vcpkg CONFIG)
  find_package(Imath CONFIG REQUIRED)

  # FFmpeg (vcpkg)
  if(DEFINED VCPKG_INSTALLED_DIR)
    include("${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/ffmpeg/FindFFMPEG.cmake")
    if(NOT FFMPEG_FOUND)
      message(FATAL_ERROR "FFmpeg not found via vcpkg")
    endif()
    if(NOT TARGET FFMPEG::FFMPEG)
      add_library(FFMPEG::FFMPEG IMPORTED INTERFACE)
      target_include_directories(FFMPEG::FFMPEG INTERFACE ${FFMPEG_INCLUDE_DIRS})
      target_link_libraries(FFMPEG::FFMPEG INTERFACE ${FFMPEG_LIBRARIES})
    endif()
  endif()
endif()

# OpenTime (optional, from parent project or vcpkg)
find_package(opentime CONFIG QUIET)

if(TARGET OTIO::opentime)
  set(FOUNDATION_HAS_OPENTIME TRUE)
elseif(TARGET opentime)
  set(FOUNDATION_HAS_OPENTIME TRUE)
endif()

# --- Gather sources via GLOB ---
file(GLOB_RECURSE FOUNDATION_HEADERS CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

file(GLOB_RECURSE FOUNDATION_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# --- Install layout (needed before INSTALL_INTERFACE) ---
include(GNUInstallDirs)

# --- Library target ---
add_library(arcvideo-foundation STATIC
  ${FOUNDATION_HEADERS}
  ${FOUNDATION_SOURCES}
)

target_include_directories(arcvideo-foundation
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(arcvideo-foundation PROPERTIES EXPORT_NAME Foundation)

target_compile_options(arcvideo-foundation PRIVATE ${FOUNDATION_COMPILE_OPTIONS})
target_compile_features(arcvideo-foundation PUBLIC cxx_std_23)

# Link dependencies
target_link_libraries(arcvideo-foundation
  PUBLIC
    Imath::Imath
    FFMPEG::FFMPEG
)

if(FOUNDATION_HAS_OPENTIME)
  set(ARCVIDEO_FOUNDATION_WITH_OTIO ON)
  if(TARGET OTIO::opentime)
    target_link_libraries(arcvideo-foundation PUBLIC OTIO::opentime)
  else()
    target_link_libraries(arcvideo-foundation PUBLIC opentime)
  endif()
  target_compile_definitions(arcvideo-foundation PUBLIC USE_OTIO)
else()
  set(ARCVIDEO_FOUNDATION_WITH_OTIO OFF)
endif()

# Platform definitions
if(WIN32)
  target_compile_definitions(arcvideo-foundation PUBLIC UNICODE _UNICODE NOMINMAX)
endif()

# --- Alias for parent project consumption ---
add_library(ArcVideo::Foundation ALIAS arcvideo-foundation)

# --- Install rules (for vcpkg / find_package) ---
include(CMakePackageConfigHelpers)

install(TARGETS arcvideo-foundation
  EXPORT arcvideo-foundation-targets
  ARCHIVE  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME  DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(DIRECTORY include/
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(EXPORT arcvideo-foundation-targets
  FILE arcvideo-foundation-targets.cmake
  NAMESPACE ArcVideo::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/arcvideo-foundation"
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/arcvideo-foundation-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/arcvideo-foundation-config.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/arcvideo-foundation"
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/arcvideo-foundation-config-version.cmake"
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/arcvideo-foundation-config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/arcvideo-foundation-config-version.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/arcvideo-foundation"
)
