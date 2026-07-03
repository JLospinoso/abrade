#include <catch2/catch_test_macros.hpp>
#include "Endpoint.h"

TEST_CASE("HostEndpoint") {
  SECTION("uses default service and port for plain host") {
    const auto endpoint = parse_host_endpoint("example.com", "443", 443);

    REQUIRE(endpoint.authority == "example.com");
    REQUIRE(endpoint.host == "example.com");
    REQUIRE(endpoint.service == "443");
    REQUIRE(endpoint.port == 443);
  }

  SECTION("parses host and explicit port") {
    const auto endpoint = parse_host_endpoint("example.com:8443", "443", 443);

    REQUIRE(endpoint.authority == "example.com:8443");
    REQUIRE(endpoint.host == "example.com");
    REQUIRE(endpoint.service == "8443");
    REQUIRE(endpoint.port == 8443);
  }

  SECTION("keeps unbracketed IPv6 as a host without parsing a port") {
    const auto endpoint = parse_host_endpoint("::1", "80", 80);

    REQUIRE(endpoint.host == "::1");
    REQUIRE(endpoint.service == "80");
    REQUIRE(endpoint.port == 80);
  }

  SECTION("parses bracketed IPv6 with explicit port") {
    const auto endpoint = parse_host_endpoint("[::1]:8080", "80", 80);

    REQUIRE(endpoint.authority == "[::1]:8080");
    REQUIRE(endpoint.host == "::1");
    REQUIRE(endpoint.service == "8080");
    REQUIRE(endpoint.port == 8080);
  }

  SECTION("rejects invalid authorities") {
    REQUIRE_THROWS(parse_host_endpoint("", "80", 80));
    REQUIRE_THROWS(parse_host_endpoint(":80", "80", 80));
    REQUIRE_THROWS(parse_host_endpoint("example.com:", "80", 80));
    REQUIRE_THROWS(parse_host_endpoint("example.com:0", "80", 80));
    REQUIRE_THROWS(parse_host_endpoint("example.com:65536", "80", 80));
    REQUIRE_THROWS(parse_host_endpoint("[::1", "80", 80));
    REQUIRE_THROWS(parse_host_endpoint("[::1]extra", "80", 80));
  }
}
