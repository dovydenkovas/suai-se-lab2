#include <boost/test/unit_test.hpp>
#include "../src/apihandler.h"

#define BOOST_TEST_MODULE ApiHandlerTest
BOOST_AUTO_TEST_SUITE(TryParseRoute)
BOOST_AUTO_TEST_CASE(Simple) {
  auto res = internal::try_parse_route("/simple/32");
  BOOST_REQUIRE(res);
  BOOST_REQUIRE_EQUAL("/simple", res->first);
  BOOST_REQUIRE_EQUAL(32, res->second);
}
BOOST_AUTO_TEST_CASE(Long) {
  auto res = internal::try_parse_route("/long/path/to/index/page/42");
  BOOST_REQUIRE(res);
  BOOST_REQUIRE_EQUAL("/long/path/to/index/page", res->first);
  BOOST_REQUIRE_EQUAL(42, res->second);
}
BOOST_AUTO_TEST_CASE(Empty) {
  auto res = internal::try_parse_route("/simple/path");
  BOOST_REQUIRE(not res.has_value());
}
BOOST_AUTO_TEST_CASE(EmptyWithDelimeter) {
  auto res = internal::try_parse_route("/simple/path/");
  BOOST_REQUIRE(not res.has_value());
}
BOOST_AUTO_TEST_CASE(SimpleDelimeter) {
  auto res = internal::try_parse_route("/simple/32/");
  BOOST_REQUIRE(not res.has_value());
}
BOOST_AUTO_TEST_SUITE_END()
