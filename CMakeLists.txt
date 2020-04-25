cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# ---- Project ----

# Note: update this to your new project's name and version
project(Gentools 
  VERSION 0.1
  LANGUAGES CXX
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()

# --- Import tools ----

include(cmake/tools.cmake)

# ---- Add dependencies via CPM ----
# see https://github.com/TheLartians/CPM.cmake for more info

include(cmake/CPM.cmake)

# PackageProject.cmake will be used to make our target installable
CPMAddPackage(
  NAME PackageProject.cmake
  GITHUB_REPOSITORY TheLartians/PackageProject.cmake
  VERSION 1.0
)

CPMAddPackage(
  NAME range-v3
  URL https://github.com/ericniebler/range-v3/archive/0.10.0.zip
  VERSION 0.10.0
  # the range-v3 CMakeLists screws with configuration options
  DOWNLOAD_ONLY True
)

if(range-v3_ADDED) 
  add_library(range-v3 INTERFACE IMPORTED)
  target_include_directories(range-v3 INTERFACE "${range-v3_SOURCE_DIR}/include")
endif()

# ---- Add source files ----

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files automatically.
# Keep that in mind when changing files, or explicitly mention them here.
FILE(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
FILE(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# ---- Create library ----

# Note: for header-only libraries change all PUBLIC flags to INTERFACE and create an interface target:
add_library(Gentools INTERFACE)  
set_target_properties(Gentools PROPERTIES INTERFACE_COMPILE_FEATURES cxx_std_20)

#add_library(Gentools ${headers} ${sources})

# Note: Not allowed on INTERFACE_LIBRARY targets (eg. header-only lib)
#set_target_properties(Gentools PROPERTIES CXX_STANDARD 20)

# being a cross-platform target, we enforce enforce standards conformance on MSVC
target_compile_options(Gentools INTERFACE 
	"$<$<BOOL:${MSVC}>:/permissive->"
	"$<$<BOOL:${MSVC}>:/await>"
	"$<$<BOOL:${MSVC}>:/experimental:preprocessor>"
)

# Link dependencies (if required)
# target_link_libraries(Gentools PUBLIC cxxopts)

target_include_directories(Gentools
  INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
	$<BUILD_INTERFACE:${range-v3_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# ---- Create an installable target ----
# this allows users to install and find the library via `find_package()`.

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
)
