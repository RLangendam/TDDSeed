cmake_minimum_required(VERSION 3.14)

set(HUNTER_PACKAGES Catch FakeIt)

include(FetchContent)
FetchContent_Declare(SetupHunter GIT_REPOSITORY https://github.com/cpp-pm/gate)
FetchContent_MakeAvailable(SetupHunter)

project ("TDDSeed")

find_package(Catch2 CONFIG REQUIRED)
find_package(FakeIt CONFIG REQUIRED)

add_library(seed_lib "src/lib.cpp" "include/lib.h")

target_include_directories(seed_lib PUBLIC "include")

add_executable(seed_app "src/main.cpp")

target_include_directories(seed_app PUBLIC "include")

target_link_libraries(seed_app PUBLIC seed_lib)

add_executable (seed_test "test/sample_test.cpp" "test/main.cpp")

target_include_directories(seed_test PUBLIC "include")

target_link_libraries(seed_test PUBLIC Catch2::Catch2 FakeIt::FakeIt seed_lib)

target_compile_features(seed_test PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(seed_lib PRIVATE /W4 /WX)
  target_compile_options(seed_app PRIVATE /W4 /WX)
  target_compile_options(seed_test PRIVATE /W4 /WX)
else()
  target_compile_options(seed_lib PRIVATE -Wall -Wextra -pedantic -Werror)
  target_compile_options(seed_app PRIVATE -Wall -Wextra -pedantic -Werror)
  target_compile_options(seed_test PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

include(CTest)
include(Catch)
catch_discover_tests(seed_test)