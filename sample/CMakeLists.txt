cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(GentoolsSample
  LANGUAGES CXX
)

# ---- Dependencies ----

include(../cmake/CPM.cmake)

#CPMAddPackage(
#  NAME cxxopts
#  GITHUB_REPOSITORY jarro2783/cxxopts
#  VERSION 2.2.0
#  OPTIONS
#    "CXXOPTS_BUILD_EXAMPLES Off"
#    "CXXOPTS_BUILD_TESTS Off"
#)

CPMAddPackage(
  NAME Gentools
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..
)

# ---- Create executable ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_executable(GentoolsSample ${sources})

set_target_properties(GentoolsSample PROPERTIES 
  CXX_STANDARD 20 
  OUTPUT_NAME "GentoolsSample"
)

target_link_libraries(GentoolsSample Gentools) # cxxopts)
