cmake_minimum_required(VERSION 2.8)

project(FastOil)

option(USE_AVX256 "Use AVX-256 Instructions" ON)

enable_language(CXX)

configure_file(FastOil/config.h.in FastOil/config.h)

find_package(Boost 1.40 REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

add_executable(FastOil FastOil/main.cpp FastOil/Nfa.cpp FastOil/NfaDotExporter.cpp FastOil/OilTrainer.cpp FastOil/SamplesReader.cpp FastOil/Testing.cpp)
target_link_libraries(FastOil opencl)

install (TARGETS FastOil DESTINATION bin)

# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set (CPACK_PACKAGE_VERSION_MAJOR "${Tutorial_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${Tutorial_VERSION_MINOR}")
include (CPack)