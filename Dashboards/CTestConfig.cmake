## This file should be placed in the root directory of your project.
## Then modify the CMakeLists.txt file in the root directory of your
## project to incorporate the testing dashboard.
## # The following are required to uses Dart and the Cdash dashboard
##   ENABLE_TESTING()
##   INCLUDE(CTest)
set(CTEST_PROJECT_NAME "mysmartgridflukso")
set(CTEST_NIGHTLY_START_TIME "04:00:00 GMT")

set(CTEST_DROP_METHOD "http")
set(CTEST_DROP_SITE "pubdoc.itwm.fhg.de")
set(CTEST_DROP_LOCATION "/p/hpc/pspro/cdash/submit.php?project=mysmartgridflukso")
set(CTEST_DROP_SITE_CDASH TRUE)

#SET (CTEST_TRIGGER_SITE "http:///cgi-bin/Submit-CMake-TestingResults.pl")
#SET (VALGRIND_COMMAND_OPTIONS "-q --tool=memcheck --leak-check=full --show-reachable=yes --workaround-gcc296-bugs=yes --num-callers=50")
#SET (CTEST_EXPERIMENTAL_COVERAGE_EXCLUDE ".*test_.*")
site_name(CTEST_SITE)
