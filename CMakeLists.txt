cmake_minimum_required(VERSION 3.14)

set(HUNTER_PACKAGES Catch FakeIt Boost OpenSSL)

include(FetchContent)
FetchContent_Declare(SetupHunter GIT_REPOSITORY https://github.com/cpp-pm/gate)
FetchContent_MakeAvailable(SetupHunter)

project ("TDDSeed")

find_package(Catch2 CONFIG REQUIRED)
find_package(FakeIt CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(lib "src/lib.cpp" "include/lib.h")

target_link_libraries(lib PUBLIC Boost::boost OpenSSL::Crypto)

target_include_directories(lib PUBLIC "include")

add_executable(app "src/main.cpp")

target_include_directories(app PUBLIC "include")

target_link_libraries(app PUBLIC lib Boost::boost)

add_executable (lib_test "test/sample_test.cpp" "test/main.cpp")

target_include_directories(lib_test PUBLIC "include")

target_link_libraries(lib_test PUBLIC Catch2::Catch2 FakeIt::FakeIt lib Boost::boost)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/english.txt ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

target_compile_features(lib PUBLIC cxx_std_20)
target_compile_features(app PUBLIC cxx_std_20)
target_compile_features(lib_test PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(lib PRIVATE /W4 /WX)
  target_compile_options(app PRIVATE /W4 /WX)
  target_compile_options(lib_test PRIVATE /W4 /WX)
else()
  target_compile_options(lib PRIVATE -Wall -Wextra -pedantic -Werror)
  target_compile_options(app PRIVATE -Wall -Wextra -pedantic -Werror)
  target_compile_options(lib_test PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

include(CTest)
include(Catch)
catch_discover_tests(lib_test)
