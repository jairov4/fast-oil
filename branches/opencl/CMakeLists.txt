cmake_minimum_required(VERSION 2.8)
set(FASTOIL_VERSION_MAJOR 1)
set(FASTOIL_VERSION_MINOR 3)

project(FastOil)

option(USE_AVX256 "Use AVX-256 Instructions" ON)

enable_language(CXX)

configure_file(src/config.h.in config.h)

find_package(Boost 1.40 REQUIRED)
include_directories("${Boost_INCLUDE_DIR}" "${PROJECT_BINARY_DIR}")

add_executable(FastOil 
	"src/main.cpp" 
	"src/Nfa.cpp" 
	"src/NfaDotExporter.cpp" 
	"src/OilTrainer.cpp" 
	"src/SamplesReader.cpp" 
	"src/Testing.cpp")

install (TARGETS FastOil DESTINATION bin)

export(PACKAGE FastOil)

# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "${FASTOIL_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${FASTOIL_VERSION_MINOR}")
include(CPack)