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

# Qt5 is found at top-level CMakeLists.txt; subdirectories inherit the
# find_package result. We reference Qt5::Core (QObject, QString) and
# Qt5::Network (QNetworkAccessManager for QrzUploader).
find_package (Qt5 REQUIRED COMPONENTS Core Network Widgets SerialPort)

# Every wkjtx module with Q_OBJECT needs moc processing: ThemeManager,
# AutoCall, AutoCallBadge, AutoCallSettingsDialog, ProfileManager,
# LogRouter, QrzUploader. Enable AUTOMOC for the whole subdir.
set (CMAKE_AUTOMOC ON)

set (WKJTX_SOURCES
  ProfileManager.cpp
  ProfileManager.hpp
  ProfileButton.cpp
  ProfileButton.hpp
  RadioProfileDialog.cpp
  RadioProfileDialog.hpp
  AutoCall.cpp
  AutoCall.hpp
  LogRouter.cpp
  LogRouter.hpp
  QrzUploader.cpp
  QrzUploader.hpp
  ThemeManager.cpp
  ThemeManager.hpp
  AutoCallSettingsDialog.cpp
  AutoCallSettingsDialog.hpp
  AutoCallBadge.cpp
  AutoCallBadge.hpp
  DataUpdater.cpp
  DataUpdater.hpp
  AdifImporter.cpp
  AdifImporter.hpp
  detectors/Maidenhead.cpp
  detectors/Maidenhead.hpp
  detectors/ZoneTables.hpp
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

# Theme QSS resource — embeds amber-classic.qss etc. into the binary
# so themes work after install and don't require the source tree.
qt5_add_resources (WKJTX_THEME_RESOURCES themes/themes.qrc)

# Export as a static object library that the main wkjtx target will
# link against. Starting v0.2 (theme system) the wkjtx target must
# link against these modules and include the theme resources.
add_library (wkjtx_modules OBJECT ${WKJTX_SOURCES} ${WKJTX_THEME_RESOURCES})
target_include_directories (wkjtx_modules PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/detectors
)
# Propagate Qt5 include paths and usage requirements.
# target_link_libraries on an OBJECT library (CMake >= 3.12) attaches
# INTERFACE_INCLUDE_DIRECTORIES of Qt5 modules so the module sources
# can find QString / QObject / QNetworkAccessManager headers.
target_link_libraries (wkjtx_modules PUBLIC Qt5::Core Qt5::Network Qt5::Widgets Qt5::SerialPort)

if (WKJTX_BUILD_TESTS)
  enable_testing ()
  add_subdirectory (tests)
endif ()
