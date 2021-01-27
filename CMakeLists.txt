cmake_minimum_required(VERSION 3.14)

set(HUNTER_PACKAGES Catch FakeIt)

include(FetchContent)
FetchContent_Declare(SetupHunter GIT_REPOSITORY https://github.com/cpp-pm/gate)
FetchContent_MakeAvailable(SetupHunter)

project ("TDDSeed")

find_package(Catch2 CONFIG REQUIRED)
find_package(FakeIt CONFIG REQUIRED)

add_executable (TDDSeed "sample_test.cpp" "main.cpp")

target_link_libraries(TDDSeed PUBLIC Catch2::Catch2 FakeIt::FakeIt)

target_compile_features(TDDSeed PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(TDDSeed PRIVATE /W4 /WX)
else()
  target_compile_options(TDDSeed PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

include(CTest)
include(Catch)
catch_discover_tests(TDDSeed)