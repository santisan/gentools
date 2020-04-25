cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(GentoolsTests
  LANGUAGES CXX
)

# ---- Options ----

option(ENABLE_TEST_COVERAGE "Enable test coverage" ON)
option(TEST_INSTALLED_VERSION "Test the version found by find_package" OFF)

# ---- Dependencies ----

include(../cmake/CPM.cmake)

CPMAddPackage(
  NAME doctest
  GITHUB_REPOSITORY onqtam/doctest
  GIT_TAG 2.3.7
)

if (TEST_INSTALLED_VERSION)
  find_package(Gentools REQUIRED)
else()
  CPMAddPackage(
    NAME Gentools
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..
  )
endif()

CPMAddPackage(
  NAME Format.cmake
  GITHUB_REPOSITORY TheLartians/Format.cmake
  VERSION 1.0
)

# ---- Create binary ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(GentoolsTests ${sources})
target_link_libraries(GentoolsTests doctest Gentools)

set_target_properties(GentoolsTests PROPERTIES CXX_STANDARD 20)

# enable compiler warnings
if (NOT TEST_INSTALLED_VERSION)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(Gentools INTERFACE -Wall -pedantic -Wextra -Werror)
  elseif(MSVC)
    #target_compile_options(Gentools INTERFACE /W4 /WX)
  endif()
endif()

# ---- Add GentoolsTests ----

ENABLE_TESTING() 

# Note: doctest and similar testing frameworks can automatically configure CMake tests
# For other testing frameworks add the tests target instead:
# ADD_TEST(GentoolsTests GentoolsTests)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(GentoolsTests)

# ---- code coverage ----

if (ENABLE_TEST_COVERAGE)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(Gentools INTERFACE -O0 -g -fprofile-arcs -ftest-coverage --coverage)
    target_link_options(Gentools INTERFACE "--coverage")
  endif()
endif()
