#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE AuthTest
#include "../src/auth.h"
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(Auth)
BOOST_AUTO_TEST_CASE(Password) {
  auto password = "password123";
  auto hash = auth::hash_password(password);
  BOOST_REQUIRE(auth::check_password(password, hash));

}


BOOST_AUTO_TEST_CASE(Tokens) {
  string fake_token = "test_token";
  BOOST_REQUIRE(!auth::check_token(fake_token));
  auto token = auth::new_token("user9000");
  BOOST_REQUIRE(auth::check_token(token));
  BOOST_REQUIRE_EQUAL("user9000", auth::login_by_token(token));
  token += "f";
  BOOST_REQUIRE(!auth::check_token(token));
}
BOOST_AUTO_TEST_SUITE_END()
