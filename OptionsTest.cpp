#include "catch.hpp"
#include "Options.h"
#include <vector>
#include <string>
#include <boost/tokenizer.hpp>
#include <algorithm>

namespace {
  Options opt(const std::string& str) {
    std::vector<std::string> tokens;
    boost::tokenizer<boost::char_separator<char>> tokenizer{str, boost::char_separator<char>{" "}};
    tokens.insert(tokens.end(), tokenizer.begin(), tokenizer.end());
    std::vector<const char*> cmdline{"abrade"};
    transform(tokens.begin(), tokens.end(), back_inserter(cmdline),
              [](const auto& str) { return str.c_str(); });
    return Options{static_cast<int>(cmdline.size()), cmdline.data()};
  }
}

TEST_CASE("Options") {
  SECTION("throws when given") {
    SECTION("empty arguments") { REQUIRE_THROWS(opt("")); }

    SECTION("three positionals") { REQUIRE_THROWS(opt("lospi.net /?asdf[1-100] WHATAMIDOING")); }
  }

  SECTION("When given help") {
    auto options = opt("--help");

    SECTION("is_help is true") { REQUIRE(options.is_help()); }

    SECTION("pattern is root") { REQUIRE(options.get_pattern() == "/"); }

    SECTION("host is blank") { REQUIRE(options.get_host() == ""); }

    SECTION("ssl is false") { REQUIRE(options.is_tls() == false); }

    SECTION("verify is false") { REQUIRE(options.is_verify() == false); }

    SECTION("contents is false") { REQUIRE(options.is_contents() == false); }
  }

  SECTION("Help string isn't blank") {
    auto options = opt("--help");
    REQUIRE(options.get_help().size() > 0);
  }

  SECTION("Parses host and pattern correctly with") {
    auto host_name = "lospi.net";
    auto pattern = "?asdf[1-10]";
    auto cmdline = std::string{ host_name };
    cmdline.append(" ");
    cmdline.append(pattern);

    SECTION("no options") {
      auto options = opt(cmdline);
      REQUIRE(options.get_host() == host_name);
      REQUIRE(options.get_pattern() == pattern);
    }

    SECTION("with ssl") {
      auto options = opt(cmdline + " --tls");
      REQUIRE(options.get_host() == host_name);
      REQUIRE(options.get_pattern() == pattern);
    }

    SECTION("with ssl and verify") {
      auto options = opt(cmdline + " --tls --verify");
      REQUIRE(options.get_host() == host_name);
      REQUIRE(options.get_pattern() == pattern);
    }

    SECTION("with ssl and verify and contents") {
      auto options = opt(cmdline + " --tls --verify --contents");
      REQUIRE(options.get_host() == host_name);
      REQUIRE(options.get_pattern() == pattern);
    }
  }

  SECTION("Parses ssl correctly") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("with no options") {
      auto options = opt(cmdline);
      REQUIRE(options.is_tls() == false);
    }

    SECTION("with ssl") {
      auto options = opt(cmdline + " --tls");
      REQUIRE(options.is_tls() == true);
    }

    SECTION("with verify") {
      auto options = opt(cmdline + " --verify");
      REQUIRE(options.is_tls() == true);
    }

    SECTION("with ssl and verify") {
      auto options = opt(cmdline + " --tls --verify");
      REQUIRE(options.is_tls() == true);
    }

    SECTION("with contents") {
      auto options = opt(cmdline + " --contents");
      REQUIRE(options.is_tls() == false);
    }

    SECTION("with contents and ssl") {
      auto options = opt(cmdline + " --contents --tls");
      REQUIRE(options.is_tls() == true);
    }

    SECTION("with contents and verify") {
      auto options = opt(cmdline + " --contents --verify");
      REQUIRE(options.is_tls() == true);
    }
  }


  SECTION("Parses contents correctly") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("with contents") {
      cmdline += " --contents";

      SECTION("and with ssl") {
        auto options = opt(cmdline + " --tls");
        REQUIRE(options.is_contents() == true);
      }

      SECTION("and with verify") {
        auto options = opt(cmdline + " --verify");
        REQUIRE(options.is_contents() == true);
      }

      SECTION("and with ssl and verify") {
        auto options = opt(cmdline + " --tls --verify");
        REQUIRE(options.is_contents() == true);
      }
    }

    SECTION("without contents and") {
      SECTION("with ssl") {
        auto options = opt(cmdline + " --tls");
        REQUIRE(options.is_contents() == false);
      }

      SECTION("with verify") {
        auto options = opt(cmdline + " --verify");
        REQUIRE(options.is_contents() == false);
      }

      SECTION("with ssl and verify") {
        auto options = opt(cmdline + " --tls --verify");
        REQUIRE(options.is_contents() == false);
      }
    }
  }

  SECTION("Parses output correctly with") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("default") {
      auto options = opt(cmdline);
      REQUIRE(options.get_output_path() == "lospi.net");
    }

    SECTION("default and --contents") {
      auto options = opt(cmdline + " --contents");
      REQUIRE(options.get_output_path() == "lospi.net-contents");
    }

    SECTION("custom value") {
      auto options = opt(cmdline + " --out=my-custom-path");
      REQUIRE(options.get_output_path() == "my-custom-path");
    }
  }

  SECTION("Parses proxy correctly with") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("default") {
      auto options = opt(cmdline);
      REQUIRE(options.get_proxy() == "");
      REQUIRE(options.is_proxy() == false);
    }

    SECTION("custom value") {
      auto options = opt(cmdline + " --proxy=127.0.0.1:9050");
      REQUIRE(options.get_proxy() == "127.0.0.1:9050");
      REQUIRE(options.is_proxy() == true);
    }

    SECTION("tor") {
      auto options = opt(cmdline + " --tor");
      REQUIRE(options.get_proxy() == "127.0.0.1:9050");
      REQUIRE(options.is_proxy() == true);
    }

    SECTION("tor and proxy") {
      auto options = opt(cmdline + " --tor --proxy=8.8.8.8:9050");
      REQUIRE(options.get_proxy() == "8.8.8.8:9050");
      REQUIRE(options.is_proxy() == true);
    }
  }

  SECTION("Parses verbose correctly") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("with no options") {
      auto options = opt(cmdline);
      REQUIRE(options.is_verbose() == false);
    }

    SECTION("with verbose option") {
      auto options = opt(cmdline + " --verbose");
      REQUIRE(options.is_verbose() == true);
    }
  }

  SECTION("Parses print found correctly") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("with no options") {
      auto options = opt(cmdline);
      REQUIRE(options.is_print_found() == false);
    }

    SECTION("with found option") {
      auto options = opt(cmdline + " --found");
      REQUIRE(options.is_print_found() == true);
    }

    SECTION("with verbose option") {
      auto options = opt(cmdline + " --verbose");
      REQUIRE(options.is_print_found() == true);
    }

    SECTION("with verbose and found options") {
      auto options = opt(cmdline + " --verbose --found");
      REQUIRE(options.is_print_found() == true);
    }
  }

  SECTION("Parses sensitive TCP teardown correctly") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("with no options") {
      auto options = opt(cmdline);
      REQUIRE(options.is_sensitive_teardown() == false);
    }

    SECTION("with found option") {
      auto options = opt(cmdline + " --sensitive");
      REQUIRE(options.is_sensitive_teardown() == true);
    }
  }

  SECTION("Parses optimizer correctly") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("with no options") {
      auto options = opt(cmdline);
      REQUIRE(options.is_optimizer() == false);
    }

    SECTION("with fixed options") {
      auto options = opt(cmdline + " --optimize");
      REQUIRE(options.is_optimizer() == true);
    }
  }

  SECTION("Parses test correctly") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("with no options") {
      auto options = opt(cmdline);
      REQUIRE(options.is_test() == false);
    }

    SECTION("with test") {
      auto options = opt(cmdline + " --test");
      REQUIRE(options.is_test() == true);
    }
  }

  SECTION("Parses leading zeroes correctly") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("with no options") {
      auto options = opt(cmdline);
      REQUIRE(options.is_leading_zeros() == false);
    }

    SECTION("with leadzero") {
      auto options = opt(cmdline + " --leadzero");
      REQUIRE(options.is_leading_zeros() == true);
    }
  }

  SECTION("Parses telescoping correctly") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("with no options") {
      auto options = opt(cmdline);
      REQUIRE(options.is_telescoping() == false);
    }

    SECTION("with telescoping") {
      auto options = opt(cmdline + " --telescoping");
      REQUIRE(options.is_telescoping() == true);
    }
  }

  SECTION("Parses user agent correctly with") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("default") {
      auto options = opt(cmdline);
      REQUIRE(options.get_user_agent() == "Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:47.0) Gecko/20100101 Firefox/47.0");
    }

    SECTION("custom value") {
      auto options = opt(cmdline + " --agent=my-user-agent-string");
      REQUIRE(options.get_user_agent() == "my-user-agent-string");
    }
  }


  SECTION("Parses initial coroutines correctly") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("with no options") {
      auto options = opt(cmdline);
      REQUIRE(options.get_initial_coroutines() == 1000);
    }

    SECTION("with a non-default value") {
      auto options = opt(cmdline + " --init=25");
      REQUIRE(options.get_initial_coroutines() == 25);
    }

    SECTION("with an illegal value") { REQUIRE_THROWS(opt(cmdline + " --init=0")); }
  }

  SECTION("Parses minimum coroutines correctly") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("with no options") {
      auto options = opt(cmdline);
      REQUIRE(options.get_minimum_coroutines() == 1);
    }

    SECTION("with a non-default value") {
      auto options = opt(cmdline + " --min=25");
      REQUIRE(options.get_minimum_coroutines() == 25);
    }

    SECTION("with an illegal value") { REQUIRE_THROWS(opt(cmdline + " --min=0")); }
  }

  SECTION("Parses maximum coroutines correctly") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("with no options") {
      auto options = opt(cmdline);
      REQUIRE(options.get_maximum_coroutines() == 25000);
    }

    SECTION("with a non-default value") {
      auto options = opt(cmdline + " --max=25");
      REQUIRE(options.get_maximum_coroutines() == 25);
    }

    SECTION("with an illegal value") { REQUIRE_THROWS(opt(cmdline + " --max=0")); }
  }

  SECTION("Parses sample size correctly") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("with no options") {
      auto options = opt(cmdline);
      REQUIRE(options.get_sample_size() == 50);
    }

    SECTION("with a non-default value") {
      auto options = opt(cmdline + " --ssize=25");
      REQUIRE(options.get_sample_size() == 25);
    }

    SECTION("with an illegal value") { REQUIRE_THROWS(opt(cmdline + " --ssize=0")); }
  }

  SECTION("Parses sample interval correctly") {
    auto cmdline = std::string{ "lospi.net ?asdf[1-10]" };

    SECTION("with no options") {
      auto options = opt(cmdline);
      REQUIRE(options.get_sample_interval() == 1000);
    }

    SECTION("with a non-default value") {
      auto options = opt(cmdline + " --sint=25");
      REQUIRE(options.get_sample_interval() == 25);
    }

    SECTION("with an illegal value") { REQUIRE_THROWS(opt(cmdline + " --sint=0")); }
  }

}
