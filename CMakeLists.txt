CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(Devourer)

if(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Release")
endif()

SET(CMAKE_CXX_FLAGS_RELEASE "-Wall -O2 -std=c++0x")
SET(CMAKE_CXX_FLAGS_DEBUG   "-Wall -O0 -std=c++0x -g")
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH    ${PROJECT_BINARY_DIR}/lib)

INCLUDE_DIRECTORIES(${INC_DIR} ./src)
LINK_DIRECTORIES(${LIB_DIR})
FILE(GLOB BASESRCS "src/*.cc" "src/modules/*.cc")
FILE(GLOB TESTSRCS "test/*.cc")

# Module code
ADD_LIBRARY(mod_devourer STATIC ${BASESRCS})
TARGET_LINK_LIBRARIES(mod_devourer swarm msgpack)

# Test code
ADD_EXECUTABLE(devourer_test ${TESTSRCS})
TARGET_LINK_LIBRARIES(devourer_test mod_devourer pthread)

# Application (CLI) code
ADD_EXECUTABLE(devourer apps/cli.cc apps/optparse.cc)
TARGET_LINK_LIBRARIES(devourer mod_devourer)

INSTALL(TARGETS devourer RUNTIME DESTINATION bin)


