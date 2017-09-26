#include "catch.hpp"
#include "Generator.h"

namespace {
  UriGenerator make(const std::string& pattern) {
    return UriGenerator{ pattern, true, false };
  }
}

TEST_CASE("UriGenerator") {
  SECTION("with 2 patterns") {
    SECTION("throws when") {
      SECTION("no closing bracket in explicit range") { REQUIRE_THROWS(make( "/my/{10:20}/desired/{1209:3298/route" )); }
      SECTION("no starting number in explicit range") { REQUIRE_THROWS(make( "/my/{10:20}/desired/{:3298}/route" )); }
      SECTION("no ending number in explicit range") { REQUIRE_THROWS(make( "/my/{10:20}/desired/{1209:}/route" )); }
      SECTION("ending number less than starting number") {
        REQUIRE_THROWS(make( "/my/{10:20}/desired/{1209:1208}/route" ));
      }
    }
  }

  SECTION("with 1 explicit range") {
    boost::optional<std::string> next_uri;

    SECTION("throws when") {
      SECTION("no closing bracket in explicit range") { REQUIRE_THROWS(make( "/my/desired/{1209:3298/route" )); }
      SECTION("no starting number in explicit range") { REQUIRE_THROWS(make( "/my/desired/{:3298}/route" )); }
      SECTION("no ending number in explicit range") { REQUIRE_THROWS(make( "/my/desired/{1209:}/route" )); }
      SECTION("ending number less than starting number") {
        REQUIRE_THROWS(make( "/my/desired/{1209:1208}/route" ));
      }
    }

    SECTION("from 0 to 1") {
      auto generator = make("/my/desired/{0:1}/route");

      SECTION("generates correct number of entries.") {
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("gives correct range size.") {
        REQUIRE(generator.get_range_size() == 2);
        next_uri = generator.next();
        REQUIRE(generator.get_range_size() == 2);
        next_uri = generator.next();
        REQUIRE(generator.get_range_size() == 2);
        next_uri = generator.next();
        REQUIRE(generator.get_range_size() == 2);
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route");
      }

      SECTION("reports correct number of entries.") {
        REQUIRE(generator.get_range_size() == 2);
      }
    }

    SECTION("from 14 to 16") {
      auto generator = make("/my/desired/{14:16}/route");

      SECTION("generates correct number of entries.") {
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/14/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/15/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/16/route");
      }
      SECTION("reports correct number of entries.") {
        REQUIRE(generator.get_range_size() == 3);
      }
    }

    SECTION("from 1000000000 to 1000001000") {
      auto generator = make("/my/desired/{1000000000:1000001000}/route");

      SECTION("generates correct number of entries.") {
        auto number_of_entries_generated{ 0 };
        while (generator.next()) { ++number_of_entries_generated; }
        REQUIRE(number_of_entries_generated == 1001);
      }

      SECTION("reports correct number of entries.") {
        REQUIRE(generator.get_range_size() == 1001);
      }
    }
  }

  SECTION("with 1 implicit range") {
    boost::optional<std::string> next_uri;

    SECTION("throws when") {
      SECTION("no closing bracket") { REQUIRE_THROWS(make( "/my/desired/{b/route" )); }
      SECTION("no starting bracket") { REQUIRE_THROWS(make( "/my/desired/b}/route" )); }
      SECTION("no type not recognized") { REQUIRE_THROWS(make( "/my/desired/{z}/route" )); }
    }

    SECTION("in octal") {
      auto generator = make("/my/desired/{o}/route");
      size_t expected_size{ 8 };

      SECTION("generates correct number of entries.") {
        while (expected_size--) {
          next_uri = generator.next();
          REQUIRE(next_uri);
        }
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("gives correct range size.") {
        REQUIRE(generator.get_range_size() == expected_size);
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route");
      }
    }

    SECTION("in octal/octal") {
      auto generator = make("/my/desired/{oo}/route");
      size_t expected_size{ 64 };

      SECTION("generates correct number of entries.") {
        while (expected_size--) {
          next_uri = generator.next();
          REQUIRE(next_uri);
        }
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("gives correct range size.") {
        REQUIRE(generator.get_range_size() == expected_size);
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/00/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/01/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/02/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/03/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/04/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/05/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/06/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/07/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/10/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/11/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/12/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/13/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/14/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/15/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/16/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/17/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/20/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/21/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/22/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/23/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/24/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/25/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/26/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/27/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/30/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/31/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/32/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/33/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/34/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/35/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/36/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/37/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/40/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/41/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/42/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/43/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/44/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/45/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/46/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/47/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/50/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/51/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/52/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/53/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/54/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/55/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/56/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/57/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/60/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/61/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/62/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/63/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/64/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/65/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/66/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/67/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/70/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/71/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/72/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/73/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/74/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/75/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/76/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/77/route");
      }
    }

    SECTION("in decimal") {
      auto generator = make("/my/desired/{d}/route");
      size_t expected_size{10};

      SECTION("generates correct number of entries.") {
        while(expected_size--) {
          next_uri = generator.next();
          REQUIRE(next_uri);
        }
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("gives correct range size.") {
        REQUIRE(generator.get_range_size() == expected_size);
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/8/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/9/route");
      }
    }

    SECTION("in lower-case hexadecimal") {
      auto generator = make("/my/desired/{h}/route");
      size_t expected_size{16};

      SECTION("generates correct number of entries.") {
        while(expected_size--) {
          next_uri = generator.next();
          REQUIRE(next_uri);
        }
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("gives correct range size.") {
        REQUIRE(generator.get_range_size() == expected_size);
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/8/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/9/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/a/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/b/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/c/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/d/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/e/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/f/route");
      }
    }

    SECTION("in upper-case hexadecimal") {
      auto generator = make("/my/desired/{H}/route");
      size_t expected_size{16};

      SECTION("generates correct number of entries.") {
        while(expected_size--) {
          next_uri = generator.next();
          REQUIRE(next_uri);
        }
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("gives correct range size.") {
        REQUIRE(generator.get_range_size() == expected_size);
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/8/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/9/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/A/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/B/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/C/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/D/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/E/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/F/route");
      }
    }

    SECTION("in lower-case alpha") {
      auto generator = make("/my/desired/{a}/route");
      size_t expected_size{26};

      SECTION("generates correct number of entries.") {
        while(expected_size--) {
          next_uri = generator.next();
          REQUIRE(next_uri);
        }
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("gives correct range size.") {
        REQUIRE(generator.get_range_size() == expected_size);
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/a/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/b/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/c/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/d/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/e/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/f/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/g/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/h/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/i/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/j/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/k/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/l/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/m/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/n/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/o/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/p/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/q/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/r/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/s/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/t/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/u/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/v/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/w/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/x/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/y/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/z/route");
      }
    }

    SECTION("in upper-case alpha") {
      auto generator = make("/my/desired/{A}/route");
      size_t expected_size{26};

      SECTION("generates correct number of entries.") {
        while(expected_size--) {
          next_uri = generator.next();
          REQUIRE(next_uri);
        }
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("gives correct range size.") {
        REQUIRE(generator.get_range_size() == expected_size);
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/A/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/B/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/C/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/D/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/E/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/F/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/G/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/H/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/I/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/J/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/K/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/L/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/M/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/N/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/O/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/P/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/Q/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/R/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/S/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/T/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/U/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/V/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/W/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/X/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/Y/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/Z/route");
      }
    }

    SECTION("in lower-case alphanumeric") {
      auto generator = make("/my/desired/{n}/route");
      size_t expected_size{36};

      SECTION("generates correct number of entries.") {
        while(expected_size--) {
          next_uri = generator.next();
          REQUIRE(next_uri);
        }
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("gives correct range size.") {
        REQUIRE(generator.get_range_size() == expected_size);
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/8/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/9/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/a/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/b/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/c/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/d/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/e/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/f/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/g/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/h/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/i/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/j/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/k/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/l/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/m/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/n/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/o/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/p/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/q/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/r/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/s/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/t/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/u/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/v/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/w/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/x/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/y/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/z/route");
      }
    }

    SECTION("in upper-case alphanumeric") {
      auto generator = make("/my/desired/{N}/route");
      size_t expected_size{36};

      SECTION("generates correct number of entries.") {
        while(expected_size--) {
          next_uri = generator.next();
          REQUIRE(next_uri);
        }
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("gives correct range size.") {
        REQUIRE(generator.get_range_size() == expected_size);
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/8/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/9/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/A/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/B/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/C/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/D/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/E/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/F/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/G/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/H/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/I/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/J/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/K/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/L/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/M/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/N/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/O/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/P/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/Q/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/R/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/S/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/T/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/U/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/V/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/W/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/X/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/Y/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/Z/route");
      }
    }

    SECTION("in lower- and upper-case alphanumeric") {
      auto generator = make("/my/desired/{b}/route");
      size_t expected_size{62};

      SECTION("generates correct number of entries.") {
        while(expected_size--) {
          next_uri = generator.next();
          REQUIRE(next_uri);
        }
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("gives correct range size.") {
        REQUIRE(generator.get_range_size() == expected_size);
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/8/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/9/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/A/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/B/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/C/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/D/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/E/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/F/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/G/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/H/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/I/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/J/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/K/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/L/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/M/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/N/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/O/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/P/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/Q/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/R/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/S/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/T/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/U/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/V/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/W/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/X/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/Y/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/Z/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/a/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/b/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/c/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/d/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/e/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/f/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/g/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/h/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/i/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/j/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/k/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/l/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/m/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/n/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/o/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/p/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/q/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/r/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/s/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/t/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/u/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/v/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/w/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/x/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/y/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/z/route");
      }
    }
  }
  
  SECTION("with 2 explicit ranges") {
    boost::optional<std::string> next_uri;

    SECTION("throws when") {
      SECTION("no closing bracket in explicit range") { REQUIRE_THROWS(make( "/my/{0:1}/desired/{1209:3298/route" )); }
      SECTION("no starting number in explicit range") { REQUIRE_THROWS(make( "/my/{0:1}/desired/{:3298}/route" )); }
      SECTION("no ending number in explicit range") { REQUIRE_THROWS(make( "/my/{0:1}/desired/{1209:}/route" )); }
      SECTION("ending number less than starting number") {
        REQUIRE_THROWS(make( "/my/{0:1}/desired/{1209:1208}/route" ));
      }
    }

    SECTION("from 0 to 1") {
      auto generator = make("/my/{0:1}/desired/{0:1}/route");

      SECTION("generates correct number of entries.") {
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("gives correct range size.") {
        REQUIRE(generator.get_range_size() == 4);
        next_uri = generator.next();
        REQUIRE(generator.get_range_size() == 4);
        next_uri = generator.next();
        REQUIRE(generator.get_range_size() == 4);
        next_uri = generator.next();
        REQUIRE(generator.get_range_size() == 4);
        next_uri = generator.next();
        REQUIRE(generator.get_range_size() == 4);
        next_uri = generator.next();
        REQUIRE(generator.get_range_size() == 4);
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/0/desired/0/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/0/desired/1/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/1/desired/0/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/1/desired/1/route");
      }

      SECTION("reports correct number of entries.") {
        REQUIRE(generator.get_range_size() == 4);
      }
    }

    SECTION("from 14 to 16") {
      auto generator = make("/my/{0:1}/desired/{14:16}/route");

      SECTION("generates correct number of entries.") {
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/0/desired/14/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/0/desired/15/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/0/desired/16/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/1/desired/14/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/1/desired/15/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/1/desired/16/route");
      }

      SECTION("reports correct number of entries.") {
        REQUIRE(generator.get_range_size() == 6);
      }
    }

    SECTION("from 1000000000 to 1000001000") {
      auto generator = make("/my/{0:1}/desired/{1000000000:1000001000}/route");

      SECTION("generates correct number of entries.") {
        auto number_of_entries_generated{ 0 };
        while (generator.next()) { ++number_of_entries_generated; }
        REQUIRE(number_of_entries_generated == 2002);
      }

      SECTION("reports correct number of entries.") {
        REQUIRE(generator.get_range_size() == 2002);
      }
    }
  }

  SECTION("with 2 implicit ranges") {
    boost::optional<std::string> next_uri;

    SECTION("throws when") {
      SECTION("no closing bracket") { REQUIRE_THROWS(make( "/my/{N}/desired/{b/route" )); }
      SECTION("no starting bracket") { REQUIRE_THROWS(make( "/my/{N}/desired/b}/route" )); }
      SECTION("no type not recognized") { REQUIRE_THROWS(make( "/my/{N}/desired/{z}/route" )); }
    }

    SECTION("in octal and decimal") {
      auto generator = make("/my/desired/{o}/route/{d}");
      size_t expected_size{ 80 };

      SECTION("generates correct number of entries.") {
        while (expected_size--) {
          next_uri = generator.next();
          REQUIRE(next_uri);
        }
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("gives correct range size.") {
        REQUIRE(generator.get_range_size() == expected_size);
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route/0");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route/1");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route/2");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route/3");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route/4");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route/5");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route/6");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route/7");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route/8");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/0/route/9");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route/0");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route/1");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route/2");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route/3");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route/4");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route/5");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route/6");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route/7");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route/8");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/1/route/9");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route/0");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route/1");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route/2");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route/3");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route/4");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route/5");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route/6");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route/7");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route/8");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/2/route/9");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route/0");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route/1");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route/2");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route/3");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route/4");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route/5");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route/6");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route/7");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route/8");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/3/route/9");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route/0");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route/1");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route/2");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route/3");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route/4");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route/5");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route/6");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route/7");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route/8");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/4/route/9");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route/0");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route/1");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route/2");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route/3");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route/4");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route/5");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route/6");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route/7");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route/8");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/5/route/9");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route/0");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route/1");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route/2");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route/3");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route/4");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route/5");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route/6");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route/7");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route/8");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/6/route/9");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route/0");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route/1");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route/2");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route/3");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route/4");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route/5");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route/6");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route/7");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route/8");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/desired/7/route/9");
      }
    }
  }

  SECTION("with 3 consecutive explicit ranges") {
    boost::optional<std::string> next_uri;

    SECTION("throws when") {
      SECTION("no closing bracket in explicit range") { REQUIRE_THROWS(make( "{0:1}{1209:3298{2:3}" )); }
      SECTION("no starting number in explicit range") { REQUIRE_THROWS(make( "{0:1}{:3298}{2:3}" )); }
      SECTION("no ending number in explicit range") { REQUIRE_THROWS(make( "{0:1}{1209:}{2:3}" )); }
      SECTION("ending number less than starting number") {
        REQUIRE_THROWS(make( "{0:1}{1209:1208}{2:3}" ));
      }
    }

    SECTION("from 0 to 1") {
      auto generator = make("{0:1}{0:1}{0:1}");

      SECTION("generates correct number of entries.") {
        size_t expected{ 8 };
        while(expected-- > 0) {
          next_uri = generator.next();
          REQUIRE(next_uri);
        }
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("gives correct range size.") {
        REQUIRE(generator.get_range_size() == 8);
        next_uri = generator.next();
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "000");
        next_uri = generator.next();
        REQUIRE(*next_uri == "001");
        next_uri = generator.next();
        REQUIRE(*next_uri == "010");
        next_uri = generator.next();
        REQUIRE(*next_uri == "011");
        next_uri = generator.next();
        REQUIRE(*next_uri == "100");
        next_uri = generator.next();
        REQUIRE(*next_uri == "101");
        next_uri = generator.next();
        REQUIRE(*next_uri == "110");
        next_uri = generator.next();
        REQUIRE(*next_uri == "111");
      }

      SECTION("reports correct number of entries.") {
        REQUIRE(generator.get_range_size() == 8);
      }
    }

    SECTION("from 14 to 16") {
      auto generator = make("/my/{0:1}/desired/{14:16}/route");

      SECTION("generates correct number of entries.") {
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE(next_uri);
        next_uri = generator.next();
        REQUIRE_FALSE(next_uri);
      }

      SECTION("generates correct entries.") {
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/0/desired/14/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/0/desired/15/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/0/desired/16/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/1/desired/14/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/1/desired/15/route");
        next_uri = generator.next();
        REQUIRE(*next_uri == "/my/1/desired/16/route");
      }

      SECTION("reports correct number of entries.") {
        REQUIRE(generator.get_range_size() == 6);
      }
    }

    SECTION("from 1000000000 to 1000001000") {
      auto generator = make("/my/{0:1}/desired/{1000000000:1000001000}/route");

      SECTION("generates correct number of entries.") {
        auto number_of_entries_generated{ 0 };
        while (generator.next()) { ++number_of_entries_generated; }
        REQUIRE(number_of_entries_generated == 2002);
      }

      SECTION("reports correct number of entries.") {
        REQUIRE(generator.get_range_size() == 2002);
      }
    }
  }
}
