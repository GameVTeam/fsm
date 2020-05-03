CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(fsm)

SET(CMAKE_CXX_STANDARD 11)

INCLUDE_DIRECTORIES(include)

ADD_LIBRARY(fsm INTERFACE)
TARGET_INCLUDE_DIRECTORIES(fsm INTERFACE include/fsm)

IF (BUILD_EXAMPLES OR BUILD_TESTS)
  IF (EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    INCLUDE(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    CONAN_BASIC_SETUP()
  ELSE ()
    MESSAGE(FATAL_ERROR "The file conanbuildinfo.cmake doesn't exist, you have to run conan install first")
  ENDIF ()
ENDIF ()

IF (BUILD_EXAMPLES)
  ADD_COMPILE_OPTIONS(-DFMT_HEADER_ONLY)

  ADD_EXECUTABLE(simple_switch examples/simple_switch.cc)
  ADD_EXECUTABLE(resetting_switch examples/resetting_switch.cc)
  ADD_EXECUTABLE(multiple_switch examples/multiple_switch.cc)
  ADD_EXECUTABLE(moore_machine examples/moore_machine.cc)
  ADD_EXECUTABLE(mealy_switch examples/mealy_switch.cc)

  ADD_EXECUTABLE(debugging_switch examples/debugging_switch.cc)
  TARGET_COMPILE_OPTIONS(debugging_switch PUBLIC -DDEBUG)

  ADD_EXECUTABLE(elevator examples/elevator/main.cc examples/elevator/elevator.h examples/elevator/fsm_list.h examples/elevator/motor.h examples/elevator/motor.cc examples/elevator/elevator.cc)
ENDIF ()

IF (BUILD_TESTS)
  ENABLE_TESTING()
  # TODO Add tests
ENDIF ()