CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
PROJECT(fsm)

SET(CMAKE_CXX_STANDARD 17)

IF (EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	INCLUDE(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	CONAN_BASIC_SETUP()
ELSE ()
	MESSAGE(WARNING "The file conanbuildinfo.cmake doesn't exist, you have to run conan install first")
ENDIF ()

INCLUDE_DIRECTORIES(include)

ADD_LIBRARY(fsm include/fsm/fsm.h include/fsm/event.h include/fsm/error.h
			examples/simple.cc src/fsm.cc src/event.cc)

IF (BUILD_EXAMPLES)
	ADD_EXECUTABLE(simple examples/simple.cc)
	ADD_EXECUTABLE(alternative examples/alternative.cc)
	ADD_EXECUTABLE(struct examples/struct.cc)

	TARGET_LINK_LIBRARIES(simple fsm)
	TARGET_LINK_LIBRARIES(alternative fsm)
	TARGET_LINK_LIBRARIES(struct fsm)
ENDIF ()