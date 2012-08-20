# -*- mode: cmake; -*-
## This file should be placed in the root directory of your project.
## Then modify the CMakeLists.txt file in the root directory of your
## project to incorporate the testing dashboard.
## # The following are required to uses Dart and the Cdash dashboard
##   ENABLE_TESTING()
##   INCLUDE(CTest)
set(CTEST_PROJECT_NAME "hexabus")
set(CTEST_NIGHTLY_START_TIME "00:00:00 GMT")

set(ENV{http_proxy} "http://squid.itwm.fhg.de:3128/")
set(CTEST_DROP_METHOD "http")
#set(CTEST_DROP_SITE "pubdoc.itwm.fhg.de")
#set(CTEST_DROP_LOCATION "/p/hpc/pspro/cdash/submit.php?project=${CTEST_PROJECT_NAME}")
set(CTEST_DROP_SITE "cdash.hexabus.de")
set(CTEST_DROP_LOCATION "/submit.php?project=${CTEST_PROJECT_NAME}")
set(CTEST_DROP_SITE_CDASH TRUE)
set(CTEST_USE_LAUNCHERS 0)
set(KDE_CTEST_DASHBOARD_DIR "/tmp/msgrid")

set(CTEST_PACKAGE_SITE "packages.mysmartgrid.de")

set(CTEST_PROJECT_SUBPROJECTS
hexabridge
hexaswitch
hexadisplay
hexinfo
)

site_name(CTEST_SITE)

set(_projectNameDir "${CTEST_PROJECT_NAME}")
