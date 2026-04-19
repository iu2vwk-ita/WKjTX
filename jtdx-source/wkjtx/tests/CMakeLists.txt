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

# Helper macro — every test has the same link requirements.
macro (wkjtx_add_test name)
  add_executable (${name} ${name}.cpp ${ARGN})
  target_link_libraries (${name} Qt5::Test Qt5::Core Qt5::Network)
  add_test (NAME ${name} COMMAND ${name})
endmacro ()

# test_autocall_safeguards — constants lock, no external deps
wkjtx_add_test (test_autocall_safeguards
  ../AutoCall.cpp
)

# test_grid_detector — 4-char Maidenhead regex + mark/query
wkjtx_add_test (test_grid_detector
  ../detectors/GridDetector.cpp
)

# test_maidenhead — grid/latlon conversion + haversine
wkjtx_add_test (test_maidenhead
  ../detectors/Maidenhead.cpp
)

# test_zone_detector — nearest-center CQ/ITU lookup against real tables
wkjtx_add_test (test_zone_detector
  ../detectors/ZoneDetector.cpp
  ../detectors/Maidenhead.cpp
)

# test_prefix_detector — prefix regex with /P /M /DX-prefix handling
wkjtx_add_test (test_prefix_detector
  ../detectors/PrefixDetector.cpp
)

# test_log_router — shared vs per-profile ADIF routing
wkjtx_add_test (test_log_router
  ../LogRouter.cpp
)

# test_autocall_pipeline — end-to-end pipeline with real detectors
wkjtx_add_test (test_autocall_pipeline
  ../AutoCall.cpp
  ../detectors/GridDetector.cpp
  ../detectors/PrefixDetector.cpp
  ../detectors/ZoneDetector.cpp
  ../detectors/NewDxccDetector.cpp
  ../detectors/CallsignDetector.cpp
  ../detectors/Maidenhead.cpp
)
