cmake_minimum_required(VERSION 2.8)

project (IRelectra)

include(CTest)
enable_testing()
#set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file (GLOB TESTS_SOURCES "tests/*.h" "tests/*.cpp")
include_directories (. tests)
add_executable (IRelectraTests IRelectra.cpp ${TESTS_SOURCES})
add_test(IRelectraTests ${PROJECT_BINARY_DIR}/IRelectraTests.exe 100)
