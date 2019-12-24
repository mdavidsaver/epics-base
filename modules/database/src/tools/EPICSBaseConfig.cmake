# Defines targets
#    EPICSBase::Com
#    EPICSBase::ca
#
# Entry point
#   ${EPICS_BASE}/lib/cmake/EPICSBase/EPICSBaseConfig.cmake
#
# Example usage
#   cmake_minimum_required(VERSION 3.10)
#   project(exampleproj VERSION 1.0)
#   find_package(EPICSBase 7.0 REQUIRED)
#   add_executable(example dummy.c)
#   target_link_libraries(example EPICSBase::ca)
#
# cmake -DCMAKE_PREFIX_PATH=${EPICS_BASE} ..

if (CMAKE_VERSION VERSION_LESS 3.1.0)
    message(FATAL_ERROR "EPICSBase requires at least CMake version 3.1.0")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
find_package(EPICSTargetArch)

get_filename_component(_EPICSBase_TOP "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

foreach(arch IN LISTS EPICS_TARGET_ARCHS)
    set(_EPICSBaseConfig_Vars_file "${_EPICSBase_TOP}/lib/${arch}/cmake/EPICSBase/EPICSBaseConfigVars.cmake")
    message(STATUS "Considering ${_EPICSBaseConfig_Vars_file}")
    if(EXISTS "${_EPICSBaseConfig_Vars_file}")
        set(EPICS_TARGET_ARCH "${arch}")
        include(${_EPICSBaseConfig_Vars_file})
        break()
    endif()
endforeach()

if(EPICS_TARGET_ARCH)
    message(STATUS "EPICSBase CPPFLAGS ${_EPICS_CPPFLAGS}")
    message(STATUS "EPICSBase CFLAGS   ${_EPICS_CFLAGS}")
    message(STATUS "EPICSBase CXXFLAGS ${_EPICS_CXXFLAGS}")
    message(STATUS "EPICSBase LDFLAGS  ${_EPICS_LDFLAGS}")
    message(STATUS "EPICSBase LDLIBS   ${_EPICS_LDLIBS}")
endif()

add_library(EPICSBase::Com SHARED IMPORTED)

set_target_properties(EPICSBase::Com PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
        "${_EPICSBase_TOP}/include;${_EPICSBase_TOP}/include/os/${EPICS_TARGET_CLASS};${_EPICSBase_TOP}/include/compiler/${EPICS_TARGET_COMPILER}"
    IMPORTED_LOCATION
        "${_EPICSBase_TOP}/lib/${EPICS_TARGET_ARCH}/${CMAKE_SHARED_MODULE_PREFIX}Com${CMAKE_SHARED_MODULE_SUFFIX}"
    INTERFACE_COMPILE_OPTIONS
        "${_EPICS_CPPFLAGS};$<$<COMPILE_LANGUAGE:C>:${_EPICS_CFLAGS}>;$<$<COMPILE_LANGUAGE:CXX>:${_EPICS_CXXFLAGS}>"
    INTERFACE_LINK_LIBRARIES
        "${_EPICS_LDFLAGS}"
)

add_library(EPICSBase::ca SHARED IMPORTED)

set_target_properties(EPICSBase::ca PROPERTIES
    IMPORTED_LOCATION
        "${_EPICSBase_TOP}/lib/${EPICS_TARGET_ARCH}/${CMAKE_SHARED_MODULE_PREFIX}ca${CMAKE_SHARED_MODULE_SUFFIX}"
)
target_link_libraries(EPICSBase::ca INTERFACE EPICSBase::Com)

if(NOT EPICS_TARGET_ARCH)
    set(EPICSBase_NOT_FOUND_MESSAGE "Unable to determine target arch")
    set(EPICSBase_FOUND False)
endif()
