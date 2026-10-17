cmake_minimum_required(VERSION 3.21)
project(flatverse_plugin LANGUAGES CXX)

# Plugin UEVR : DLL chargée par UEVRBackend depuis <config UEVR>/<jeu>/plugins/
# Build (Windows, Visual Studio 2022) :
#   cmake -S plugin -B build -A x64      (VS 2022 ou plus récent détecté automatiquement)
#   cmake --build build --config Release
# Résultat : build/Release/flatverse.dll

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
  message(FATAL_ERROR "Les plugins UEVR sont des DLL Windows x64 uniquement.")
endif()

add_library(flatverse SHARED
  src/Plugin.cpp
)

target_include_directories(flatverse PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/external/uevr-api   # API UEVR (MIT)
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(flatverse PRIVATE
  NOMINMAX
  WIN32_LEAN_AND_MEAN
  UNICODE
  _UNICODE
)

if(MSVC)
  target_compile_options(flatverse PRIVATE /W4 /permissive- /Zc:preprocessor)
endif()

set_target_properties(flatverse PROPERTIES
  OUTPUT_NAME "flatverse"
  PREFIX ""
)
