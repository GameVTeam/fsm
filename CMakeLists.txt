CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
PROJECT(fsm)

SET(CXX_STANDARD_REQUIRED true)
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

IF (BUILD_TESTS)
  SET(GTEST_LIB gtest_maind gtestd)

  ENABLE_TESTING()

  ADD_EXECUTABLE(fsm_test
                 tests/fsm_test/fsm_test.h
                 tests/fsm_test/test_mem_fn_row.cc
                 tests/fsm_test/test_basic_row.cc
                 tests/fsm_test/test_no_trans.cc
                 tests/fsm_test/test_recursive.cc
                 tests/fsm_test/test_scoped.cc)
  ADD_EXECUTABLE(traits_test tests/traits_test/traits_test.cc)

  SET_TARGET_PROPERTIES(traits_test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

  TARGET_LINK_LIBRARIES(fsm_test ${GTEST_LIB})
  TARGET_LINK_LIBRARIES(traits_test ${GTEST_LIB})

  ADD_TEST(TestFSM fsm_test)
  ADD_TEST(TraitsTest traits_test)
ENDIF ()

IF (BUILD_EXAMPLES)
  ADD_COMPILE_OPTIONS(-DFMT_HEADER_ONLY)
  ADD_EXECUTABLE(cd_player examples/cd_player.cc)
ENDIF ()
