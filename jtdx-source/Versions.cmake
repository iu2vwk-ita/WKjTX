# Version number components
# WKjTX versioning is independent from upstream JTDX (originally 2.2.159).
# v1.2.0 adds qrz.com Logbook upload (API-key-based, auto or queued
# manual), a persistent pending-uploads queue shared with eQSL, a
# PendingUploadsDialog with retry/remove, and a close-time prompt that
# offers Upload now / Leave for later / Discard. It also rolls up the
# v1.1.3 changes (19 JTDX-inherited translations shipped as Qt
# resources, UPDATE DATA TLS fix, incremental-build OMNIRIG_EXE fix).
#
# IMPORTANT: WSJTX_VERSION_32A is a BOOLEAN flag inherited from JTDX —
# when non-zero the build appends "-32A" to the version patch string
# (see CMake/VersionCompute.cmake). It is NOT the patch number.
# WSJTX_VERSION_SUB is the real patch field. Keep 32A at 0 for WKjTX.
set (WSJTX_VERSION_MAJOR 1)
set (WSJTX_VERSION_MINOR 2)
set (WSJTX_VERSION_32A 0)
set (WSJTX_VERSION_SUB 0)
set (WSJTX_RC 0)		 # release candidate number, comment out or zero for development versions
set (WSJTX_VERSION_IS_RELEASE 1) # set to 1 for final release build
