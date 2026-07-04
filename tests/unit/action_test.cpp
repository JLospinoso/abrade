#include <abrade/action.hpp>
#include <abrade/content_filter.hpp>
#include <abrade/run_stats.hpp>
#include <boost/filesystem.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>

using namespace abrade;

namespace {
struct ScopedTempDir {
  ScopedTempDir()
      : path{boost::filesystem::temp_directory_path() /
             boost::filesystem::unique_path("abrade-action-test-%%%%-%%%%-%%%%")} {
    boost::filesystem::create_directories(path);
  }

  ScopedTempDir(const ScopedTempDir&) = delete;
  ScopedTempDir(ScopedTempDir&&) = delete;
  ScopedTempDir& operator=(const ScopedTempDir&) = delete;
  ScopedTempDir& operator=(ScopedTempDir&&) = delete;

  ~ScopedTempDir() {
    boost::system::error_code ignored;
    boost::filesystem::remove_all(path, ignored);
  }

  boost::filesystem::path path;
};

std::string read_file(const boost::filesystem::path& path) {
  std::ifstream file{path.string()};
  return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}
} // namespace

TEST_CASE("HeadAction") {
  SECTION("appends only successful candidates to the output file") {
    const ScopedTempDir temp;
    const auto output_path = temp.path / "found.txt";
    HeadAction action{output_path.string(), false};

    action.process(200, "/ok");
    action.process(204, "/created");
    action.process(404, "/missing");

    REQUIRE(read_file(output_path) == "/ok\n/created\n");
  }
}

TEST_CASE("GetAction") {
  SECTION("writes successful response bodies to sanitized candidate paths") {
    const ScopedTempDir temp;
    RunStats stats;
    GetAction action{temp.path.string(), ContentFilters{}, false, stats};

    action.process(200, std::string{"payload"}, "/items/1?format=json");
    action.process(404, std::string{"missing"}, "/items/2");

    REQUIRE(read_file(temp.path / "_items_1_format_json") == "payload");
    REQUIRE_FALSE(boost::filesystem::exists(temp.path / "_items_2"));
    REQUIRE(stats.bytes_written() == 7);
  }

  SECTION("filters successful responses with literal require and reject rules") {
    const ScopedTempDir temp;
    RunStats stats;
    ContentFilters filters{{"clean"}, {"blocked-marker"}, {}, {}};
    GetAction action{temp.path.string(), std::move(filters), false, stats};

    action.process(200, std::string{"clean payload"}, "/clean");
    action.process(200, std::string{"wrong payload"}, "/missing-required");
    action.process(200, std::string{"blocked-marker payload"}, "/screened");

    REQUIRE(read_file(temp.path / "_clean") == "clean payload");
    REQUIRE_FALSE(boost::filesystem::exists(temp.path / "_missing-required"));
    REQUIRE_FALSE(boost::filesystem::exists(temp.path / "_screened"));
    REQUIRE(stats.filtered() == 2);
    REQUIRE(stats.bytes_written() == 13);
  }

  SECTION("filters successful responses with repeated regex rules") {
    const ScopedTempDir temp;
    RunStats stats;
    ContentFilters filters{{}, {}, {"HCRA\\s+Race", "Division:\\s+Open"}, {"cancelled|postponed"}};
    GetAction action{temp.path.string(), std::move(filters), false, stats};

    action.process(200, std::string{"HCRA Race\nDivision: Open\n"}, "/accepted");
    action.process(200, std::string{"HCRA Race\nDivision: Novice\n"}, "/missing-regex");
    action.process(200, std::string{"HCRA Race\nDivision: Open\ncancelled\n"}, "/rejected");

    REQUIRE(read_file(temp.path / "_accepted") == "HCRA Race\nDivision: Open\n");
    REQUIRE_FALSE(boost::filesystem::exists(temp.path / "_missing-regex"));
    REQUIRE_FALSE(boost::filesystem::exists(temp.path / "_rejected"));
    REQUIRE(stats.filtered() == 2);
  }
}
