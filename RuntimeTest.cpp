#include <catch2/catch_test_macros.hpp>
#include "HttpStatus.h"
#include "ScraperRuntime.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace {
  struct ScopedRuntimeTempDir {
    ScopedRuntimeTempDir()
      : path{boost::filesystem::temp_directory_path()
             / boost::filesystem::unique_path("abrade-runtime-test-%%%%-%%%%-%%%%")} {
      boost::filesystem::create_directories(path);
    }

    ~ScopedRuntimeTempDir() {
      boost::system::error_code ignored;
      boost::filesystem::remove_all(path, ignored);
    }

    boost::filesystem::path path;
  };

  std::string read_runtime_file(const boost::filesystem::path& path) {
    std::ifstream file{path.string()};
    return {
      std::istreambuf_iterator<char>{file},
      std::istreambuf_iterator<char>{}
    };
  }
}

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

  SECTION("records scraper errors with candidate context") {
    const ScopedRuntimeTempDir temp;
    const auto path = temp.path / "errors.log";
    const FileErrorLog error_log{path.string(), false};

    error_log.record("/items/1", std::runtime_error{"connect failed"});

    REQUIRE(read_runtime_file(path) == "/items/1: connect failed\n");
  }
}
