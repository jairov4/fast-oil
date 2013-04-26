cmake_minimum_required(VERSION 2.8)
set(FASTOIL_VERSION_MAJOR 1)
set(FASTOIL_VERSION_MINOR 3)

project(FastOil CXX)

option(USE_AVX256 "Use AVX-256 Instructions" ON)

configure_file(src/config.h.in config.h)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")

find_package(Boost 1.40 REQUIRED)
find_package(OpenCL REQUIRED)

include_directories(
	"${Boost_INCLUDE_DIR}"
	"${OPENCL_INCLUDE_DIRS}"
	"${PROJECT_BINARY_DIR}")

add_executable(FastOil 
	"src/main.cpp" 
	"src/NfaDotExporter.cpp" 
	"src/OilTrainer.cpp" 
	"src/SamplesReader.cpp")
	
add_executable(Nfa_Test "src/TestNfa.cpp")
add_test(Nfa_Test NfaTest)

target_link_libraries(FastOil "${OPENCL_LIBRARIES}")

install(TARGETS FastOil DESTINATION bin)

export(PACKAGE FastOil)

# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "${FASTOIL_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${FASTOIL_VERSION_MINOR}")
include(CPack)