#include <abrade/http_status.hpp>
#include <abrade/run_stats.hpp>
#include <abrade/scraper_runtime.hpp>
#include <boost/filesystem.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <iterator>
#include <stdexcept>

using namespace abrade;

namespace {
struct ScopedRuntimeTempDir {
  ScopedRuntimeTempDir()
      : path{boost::filesystem::temp_directory_path() /
             boost::filesystem::unique_path("abrade-runtime-test-%%%%-%%%%-%%%%")} {
    boost::filesystem::create_directories(path);
  }

  ScopedRuntimeTempDir(const ScopedRuntimeTempDir&) = delete;
  ScopedRuntimeTempDir(ScopedRuntimeTempDir&&) = delete;
  ScopedRuntimeTempDir& operator=(const ScopedRuntimeTempDir&) = delete;
  ScopedRuntimeTempDir& operator=(ScopedRuntimeTempDir&&) = delete;

  ~ScopedRuntimeTempDir() {
    boost::system::error_code ignored;
    boost::filesystem::remove_all(path, ignored);
  }

  boost::filesystem::path path;
};

std::string read_runtime_file(const boost::filesystem::path& path) {
  std::ifstream file{path.string()};
  return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}
} // namespace

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

TEST_CASE("RunStats") {
  SECTION("distinguishes responses, filtered bodies, errors, and bytes written") {
    RunStats stats;

    stats.record_attempt();
    stats.record_response(200);
    stats.record_bytes_written(1024);
    stats.record_attempt();
    stats.record_response(404);
    stats.record_filtered();
    stats.record_error();

    REQUIRE(stats.attempted() == 2);
    REQUIRE(stats.success_2xx() == 1);
    REQUIRE(stats.non_2xx() == 1);
    REQUIRE(stats.filtered() == 1);
    REQUIRE(stats.errors() == 1);
    REQUIRE(stats.bytes_written() == 1024);
    REQUIRE(stats.has_errors());
    REQUIRE(stats.summary().contains("attempted=2"));
    REQUIRE(stats.summary().contains("2xx=1"));
    REQUIRE(stats.summary().contains("non-2xx=1"));
    REQUIRE(stats.summary().contains("bytes-written=1024"));
  }
}
