cmake_minimum_required(VERSION 2.8)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON )
set(CMAKE_VERBOSE_MAKEFILE ON)

set(PROJECT_NAME "mtlPy")
project(${PROJECT_NAME})


#The project version number
set(VERSION_MAJOR   0   CACHE STRING "Project major version number.")
set(VERSION_MINOR   2   CACHE STRING "Project minor version number.")
set(VERSION_PATCH   0   CACHE STRING "Project patch version number.")

# output path
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(DOCUMENT_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/docs)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)


#Check if intall prefix flag is found, if not, set a default path
if (CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX_DIR ${CMAKE_INSTALL_PREFIX})
else()
    set(CMAKE_INSTALL_PREFIX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/install)
endif()
unset(CMAKE_INSTALL_PREFIX)


message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")



# Find the directory for ABC. If cache variable ABC_DIR is defined, use it as the path to ABC, otherwise use the system ABC_DIR
if (MTL_DIR)
    set(MTL_ROOT_DIR ${MTL_DIR})
    message(STATUS "Use MTL: ${MTL_ROOT_DIR}")
else()
    set(MTL_ROOT_DIR $ENV{MTL_DIR})
    message(STATUS "Use system MTL ${MTL_ROOT_DIR}")
endif()
unset (MTL_DIR CACHE)

find_package(Boost 1.6 COMPONENTS system graph iostreams)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ${DOXYGEN_FOUND})

if(BUILD_DOCUMENTATION)
    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif()

    set(doxy_main_page ${CMAKE_CURRENT_SOURCE_DIR}/Readme.md)
    set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
    set(doxyfile ${DOCUMENT_OUTPUT_PATH}/Doxyfile)

    #request to configure the file
    configure_file(${doxyfile_in} ${doxyfile} @ONLY)

    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
        WORKING_DIRECTORY ${DOCUMENT_OUTPUT_PATH}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)

    #install(DIRECTORY ${DOCUMENT_OUTPUT_PATH}/html DESTINATION ${CMAKE_INSTALL_PREFIX_DIR}/doc)
endif()
mark_as_advanced(VERSION_MAJOR VERSION_MINOR VERSION_PATCH)



file(GLOB SOURCES src/global/*.h    src/global/*.cpp
                  src/util/*.h      src/util/*.cpp
                  )

file(GLOB EXE_SOURCES src/main/main.cpp)
file(GLOB PY_API_SOURCES src/api/*.cpp)

#pybind11
if (PYBIND11_DIR)
    set(PYBIND11_ROOT_DIR ${PYBIND11_DIR})
    message(STATUS "Use pybind11: ${PYBIND11_ROOT_DIR}")
else()
    set(PYBIND11_ROOT_DIR $ENV{PYBIND11_DIR})
    message(STATUS "Use system pybind11: ${PYBIND11_ROOT_DIR}")
endif()
unset(PYBIND11_DIR CACHE)

add_subdirectory(${PYBIND11_ROOT_DIR} "./pybind11")

# Python
if (NOT PYTHON_INCLUDE_DIR OR NOT PYTHON_LIBRARIES)
find_package(PythonLibs REQUIRED)
endif()
message(STATUS "python include directory: ${PYTHON_INCLUDE_DIR}")
message(STATUS "python libraries: ${PYTHON_LIBRARIES}")

#Print out the used build type
IF(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Using build type DEBUG")
    #ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)
ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
    message(STATUS "Using build type RELEASE")
    #ENDIF(CMAKE_BUILD_TYPE MATCHES Release)
ELSEIF(CMAKE_BUILD_TYPE MATCHES Profile)
    message(STATUS "Using build type PROFILE")
    #ENDIF(CMAKE_BUILD_TYPE MATCHES Profile)
ELSE()
    message(STATUS "Using build type DEFAULT: using Release flag")
    set(CMAKE_BUILD_TYPE Release)
ENDIF()
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_CXX_FLAGS "-std=gnu++17 -Wall -march=native -fopenmp -fPIC -lreadline -O3 -DNDEBUG -W -Wextra -Wno-unknown-pragmas -pthread -DABC_NAMESPACE=pabc -DABC_NO_USE_READLINE -DDISABLE_NAUTY -DFMT_HEADER_ONLY -DLIN64 -I${MTL_ROOT_DIR}/include -I${MTL_ROOT_DIR}/lib/lorina -I${MTL_ROOT_DIR}/lib/rang -I${MTL_ROOT_DIR}/lib/fmt -I${MTL_ROOT_DIR}/lib/abcsat -I${MTL_ROOT_DIR}/lib/json -I${MTL_ROOT_DIR}/lib/abcesop -I${MTL_ROOT_DIR}/lib/abcresub -isystem ${MTL_ROOT_DIR}/lib/kitty -isystem ${MTL_ROOT_DIR}/lib/parallel_hashmap -isystem ${MTL_ROOT_DIR}/lib/percy -isystem ${MTL_ROOT_DIR}/lib/bill")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-inline ")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -frename-registers -fprofile-use -fprofile-correction") 
set(CMAKE_CXX_FLAGS_PROFILE "-Ofast -pg -Winline")
set(CMAKE_EXE_LINKER_FLAGS "-pthread -static-libgcc -static-libstdc++ -static -O3")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)

link_libraries (
    ${GTEST_MAIN_LIB}
    ${PYTHON_LIBRARIES}
)

# Add modules to pybind
pybind11_add_module("mtlPy" ${PY_API_SOURCES} ${SOURCES})
target_link_libraries("mtlPy" PUBLIC ${Boost_LIBRARIES} )

