# WKjTX-specific modules — added on top of JTDX upstream.
#
# This directory is NOT wired into the main wkjtx executable yet (v0.1
# is a pure rebrand of JTDX; no new code paths are active). Plans 2, 3,
# 4 will link these into the main target and expose them through
# MainWindow / Configuration.
#
# To build just the WKjTX modules standalone (e.g. for running tests):
#     cmake -DWKJTX_BUILD_STANDALONE=ON ..

cmake_minimum_required (VERSION 3.7.2)

if (NOT DEFINED WKJTX_BUILD_TESTS)
  option (WKJTX_BUILD_TESTS "Build WKjTX unit tests" OFF)
endif ()

set (WKJTX_SOURCES
  ProfileManager.cpp
  ProfileManager.hpp
  AutoCall.cpp
  AutoCall.hpp
  LogRouter.cpp
  LogRouter.hpp
  QrzUploader.cpp
  QrzUploader.hpp
  detectors/NewDxccDetector.cpp
  detectors/NewDxccDetector.hpp
  detectors/ZoneDetector.cpp
  detectors/ZoneDetector.hpp
  detectors/GridDetector.cpp
  detectors/GridDetector.hpp
  detectors/PrefixDetector.cpp
  detectors/PrefixDetector.hpp
  detectors/CallsignDetector.cpp
  detectors/CallsignDetector.hpp
)

# Export as a static object library that the main wkjtx target will
# link against starting in Plan 2. For v0.1 this is a no-op (not
# referenced by the top-level CMakeLists.txt yet).
add_library (wkjtx_modules OBJECT ${WKJTX_SOURCES})
target_include_directories (wkjtx_modules PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/detectors
)

if (WKJTX_BUILD_TESTS)
  enable_testing ()
  add_subdirectory (tests)
endif ()
