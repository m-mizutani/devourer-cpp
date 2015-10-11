CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(Devourer)

if(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Release")
endif()

if(POLICY CMP0015)
    cmake_policy(SET CMP0015 NEW)
endif()
if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()

      

SET(CMAKE_CXX_FLAGS_RELEASE "-Wall -O2 -std=c++0x")
SET(CMAKE_CXX_FLAGS_DEBUG   "-Wall -O0 -std=c++0x -g")
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH    ${PROJECT_BINARY_DIR}/lib)

OPTION(WITH_FLUENT "libfluent prefix" ".")
SET(FLUENT_INCLUDES  ${WITH_FLUENT}/src)
SET(FLUENT_LIBRARIES ${WITH_FLUENT}/lib)

INCLUDE_DIRECTORIES(${INC_DIR} ${FLUENT_INCLUDES} ./src ./external/swarm/src)
LINK_DIRECTORIES(${LIB_DIR} ${FLUENT_LIBRARIES} ./external/)
FILE(GLOB BASESRCS 
	  "src/*.cc" "src/*.h" "src/modules/*.cc" "src/modules/*.h"
	  "external/swarm/src/swarm/*.hpp"
	  "external/swarm/src/*.cc" "external/swarm/src/*.hpp"
	  "external/swarm/src/proto/*.cc" "external/swarm/src/proto/*.h"
	  )
FILE(GLOB TESTSRCS "test/*.cc")

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")




# Module code
ADD_LIBRARY(devourer SHARED ${BASESRCS})
TARGET_LINK_LIBRARIES(devourer fluent pcap ev)

# Test code
ADD_EXECUTABLE(devourer-test ${TESTSRCS})
TARGET_LINK_LIBRARIES(devourer-test devourer pthread)

# Application (CLI) code
ADD_EXECUTABLE(devourer-bin apps/cli.cc apps/optparse.cc)
SET_TARGET_PROPERTIES(devourer-bin
  PROPERTIES OUTPUT_NAME devourer)
TARGET_LINK_LIBRARIES(devourer-bin devourer)

INSTALL(TARGETS devourer-bin RUNTIME DESTINATION bin)
INSTALL(TARGETS devourer LIBRARY DESTINATION lib)
INSTALL(FILES src/devourer.h DESTINATION include/)
