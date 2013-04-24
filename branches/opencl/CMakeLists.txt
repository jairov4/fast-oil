cmake_minimum_required(VERSION 2.8)

project(FastOil)

option(USE_AVX256 "Use AVX-256 Instructions" ON)

enable_language(CXX)

configure_file(config.h.in config.h)

find_package(Boost 1.40 REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

add_executable(FastOil main.cpp Nfa.cpp NfaDotExporter.cpp OilTrainer.cpp SamplesReader.cpp Testing.cpp)

install (TARGETS FastOil DESTINATION bin)

# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set (CPACK_PACKAGE_VERSION_MAJOR "${Tutorial_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${Tutorial_VERSION_MINOR}")
include (CPack)