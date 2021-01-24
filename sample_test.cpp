#include <catch2/catch.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <iterator>

TEST_CASE("sample")
{
    std::vector<int> data{1, 2, 3};
    std::vector<int> output;
    boost::copy(data | boost::adaptors::transformed([](auto v){return 2*v;}), std::back_inserter(output));
	REQUIRE(output == std::vector<int>{2, 4, 6});
}
