# CMake generated Testfile for 
# Source directory: C:/Users/Holour/Desktop/test/OS/backend
# Build directory: C:/Users/Holour/Desktop/test/OS/backend/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(FullApiIntegrationTest "C:/Users/Holour/Desktop/test/OS/backend/build/Debug/api_tests.exe")
  set_tests_properties(FullApiIntegrationTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Holour/Desktop/test/OS/backend/CMakeLists.txt;56;add_test;C:/Users/Holour/Desktop/test/OS/backend/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(FullApiIntegrationTest "C:/Users/Holour/Desktop/test/OS/backend/build/Release/api_tests.exe")
  set_tests_properties(FullApiIntegrationTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Holour/Desktop/test/OS/backend/CMakeLists.txt;56;add_test;C:/Users/Holour/Desktop/test/OS/backend/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(FullApiIntegrationTest "C:/Users/Holour/Desktop/test/OS/backend/build/MinSizeRel/api_tests.exe")
  set_tests_properties(FullApiIntegrationTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Holour/Desktop/test/OS/backend/CMakeLists.txt;56;add_test;C:/Users/Holour/Desktop/test/OS/backend/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(FullApiIntegrationTest "C:/Users/Holour/Desktop/test/OS/backend/build/RelWithDebInfo/api_tests.exe")
  set_tests_properties(FullApiIntegrationTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Holour/Desktop/test/OS/backend/CMakeLists.txt;56;add_test;C:/Users/Holour/Desktop/test/OS/backend/CMakeLists.txt;0;")
else()
  add_test(FullApiIntegrationTest NOT_AVAILABLE)
endif()
