#include <catch2/catch.hpp>
#include "lib.h"

TEST_CASE("sample") { REQUIRE("Hello world.\n" == function()); }
