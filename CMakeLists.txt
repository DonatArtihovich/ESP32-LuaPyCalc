# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS app drivers)
set(CXX_STANDARD_REQUIRED 17)

if(NOT MICROPY_HELPER_DIR)
    set(MICROPY_HELPER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/managed_components/tomasweij__micropython-helper)
endif()

if(EXISTS ${MICROPY_HELPER_DIR})
    include(${MICROPY_HELPER_DIR}/mpy.cmake)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(luapycalc)
spiffs_create_partition_image(storage1 fonts FLASH_IN_PROJECT)