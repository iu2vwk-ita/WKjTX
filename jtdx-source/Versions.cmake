# Version number components
# WKjTX versioning is independent from upstream JTDX (originally 2.2.159).
# v1.1.2 adds the NTP clock-offset badge in the menubar corner and a
# one-click elevated system-clock resync (PowerShell Set-Date).
#
# IMPORTANT: WSJTX_VERSION_32A is a BOOLEAN flag inherited from JTDX —
# when non-zero the build appends "-32A" to the version patch string
# (see CMake/VersionCompute.cmake). It is NOT the patch number.
# WSJTX_VERSION_SUB is the real patch field. Keep 32A at 0 for WKjTX.
set (WSJTX_VERSION_MAJOR 1)
set (WSJTX_VERSION_MINOR 1)
set (WSJTX_VERSION_32A 0)
set (WSJTX_VERSION_SUB 2)
set (WSJTX_RC 0)		 # release candidate number, comment out or zero for development versions
set (WSJTX_VERSION_IS_RELEASE 1) # set to 1 for final release build
