#include <catch2/catch_test_macros.hpp>
#include "Action.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <iterator>
#include <string>

namespace {
  struct ScopedTempDir {
    ScopedTempDir()
      : path{boost::filesystem::temp_directory_path()
             / boost::filesystem::unique_path("abrade-action-test-%%%%-%%%%-%%%%")} {
      boost::filesystem::create_directories(path);
    }

    ~ScopedTempDir() {
      boost::system::error_code ignored;
      boost::filesystem::remove_all(path, ignored);
    }

    boost::filesystem::path path;
  };

  std::string read_file(const boost::filesystem::path& path) {
    std::ifstream file{path.string()};
    return {
      std::istreambuf_iterator<char>{file},
      std::istreambuf_iterator<char>{}
    };
  }
}

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
    GetAction action{temp.path.string(), "", false};

    action.process(200, std::string{"payload"}, "/items/1?format=json");
    action.process(404, std::string{"missing"}, "/items/2");

    REQUIRE(read_file(temp.path / "_items_1_format_json") == "payload\n");
    REQUIRE_FALSE(boost::filesystem::exists(temp.path / "_items_2"));
  }

  SECTION("screens successful responses whose body contains the configured text") {
    const ScopedTempDir temp;
    GetAction action{temp.path.string(), "blocked-marker", false};

    action.process(200, std::string{"clean payload"}, "/clean");
    action.process(200, std::string{"blocked-marker payload"}, "/screened");

    REQUIRE(read_file(temp.path / "_clean") == "clean payload\n\n");
    REQUIRE_FALSE(boost::filesystem::exists(temp.path / "_screened"));
  }
}
