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
FILE(GLOB BASESRCS 
	  "src/*.cc" "src/*.h" "src/modules/*.cc" "src/modules/*.h"
	  "src/swarm/*.cc" "src/swarm/*.h" "src/swarm/swarm/*.h" 
	  "src/swarm/utils/*.cc" "src/swarm/utils/*.h"
	  "src/swarm/proto/*.cc" "src/swarm/proto/*.h"
)
FILE(GLOB TESTSRCS "test/*.cc")

# Module code
ADD_LIBRARY(mod_devourer SHARED ${BASESRCS})
TARGET_LINK_LIBRARIES(mod_devourer msgpack pcap ev)

# Test code
ADD_EXECUTABLE(devourer_test ${TESTSRCS})
TARGET_LINK_LIBRARIES(devourer_test mod_devourer)

# Application (CLI) code
ADD_EXECUTABLE(devourer apps/cli.cc apps/optparse.cc)
TARGET_LINK_LIBRARIES(devourer mod_devourer)

INSTALL(TARGETS devourer RUNTIME DESTINATION bin)
INSTALL(TARGETS mod_devourer LIBRARY DESTINATION lib)



