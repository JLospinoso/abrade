#include <catch2/catch_test_macros.hpp>
#include "HttpStatus.h"
#include "ScraperRuntime.h"

TEST_CASE("HttpStatus") {
  SECTION("classifies successful responses") {
    REQUIRE_FALSE(is_success_status(199));
    REQUIRE(is_success_status(200));
    REQUIRE(is_success_status(204));
    REQUIRE(is_success_status(299));
    REQUIRE_FALSE(is_success_status(300));
  }
}

TEST_CASE("ScraperRuntime") {
  SECTION("creates candidates from generated URIs") {
    const auto candidate = make_candidate("/items/1");

    REQUIRE(candidate.uri == "/items/1");
    REQUIRE(candidate.headers.empty());
    REQUIRE(candidate.contents.empty());
  }
}
