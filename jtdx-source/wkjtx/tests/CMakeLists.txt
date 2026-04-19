# WKjTX unit tests — only built when WKJTX_BUILD_TESTS=ON.
# Run with: ctest --output-on-failure
#
# These tests cover the NEW modules added by WKjTX (ProfileManager,
# AutoCall, detectors, LogRouter, QrzUploader). They do not test
# JTDX upstream code.

find_package (Qt5 REQUIRED COMPONENTS Test Core Network)

include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../detectors
)

# test_autocall_safeguards
add_executable (test_autocall_safeguards
  test_autocall_safeguards.cpp
  ../AutoCall.cpp
  ../AutoCall.hpp
)
target_link_libraries (test_autocall_safeguards Qt5::Test Qt5::Core)
add_test (NAME test_autocall_safeguards COMMAND test_autocall_safeguards)

# test_grid_detector
add_executable (test_grid_detector
  test_grid_detector.cpp
  ../detectors/GridDetector.cpp
  ../detectors/GridDetector.hpp
)
target_link_libraries (test_grid_detector Qt5::Test Qt5::Core)
add_test (NAME test_grid_detector COMMAND test_grid_detector)
