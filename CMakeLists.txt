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

ADD_LIBRARY(fsm include/fsm/fsm.h include/fsm/events.h include/fsm/errors.h
			src/fsm.cc src/events.cc include/fsm/utils.h src/utils.cc)

IF (BUILD_EXAMPLES)
	ADD_EXECUTABLE(simple examples/simple.cc)
	ADD_EXECUTABLE(alternative examples/alternative.cc)
	ADD_EXECUTABLE(struct examples/struct.cc)
	ADD_EXECUTABLE(graph examples/graph.cc)

	TARGET_LINK_LIBRARIES(simple fsm)
	TARGET_LINK_LIBRARIES(alternative fsm)
	TARGET_LINK_LIBRARIES(struct fsm)
	TARGET_LINK_LIBRARIES(graph fsm)
ENDIF ()

IF (BUILD_TESTS)
	ENABLE_TESTING()

	ADD_EXECUTABLE(fsm_test tests/fsm_tests.cc)
	TARGET_LINK_LIBRARIES(fsm_test fsm gtest_maind gtestd)

	ADD_TEST(NAME FSM_TEST COMMAND fsm_test)
ENDIF ()