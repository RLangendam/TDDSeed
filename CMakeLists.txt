cmake_minimum_required(VERSION 3.14)

set(HUNTER_PACKAGES GTest)

include(FetchContent)
FetchContent_Declare(SetupHunter GIT_REPOSITORY https://github.com/cpp-pm/gate)
FetchContent_MakeAvailable(SetupHunter)

project ("TDDSeed")

find_package(GTest CONFIG REQUIRED)

add_executable (TDDSeed "sample_test.cpp")

target_link_libraries(TDDSeed PUBLIC GTest::gmock_main)

target_compile_features(TDDSeed PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(TDDSeed PRIVATE /W4 /WX)
else()
  target_compile_options(TDDSeed PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

include(CTest)
include(GoogleTest)
gtest_add_tests(TARGET TDDSeed)