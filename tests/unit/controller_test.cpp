#include <abrade/controller.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace abrade;

TEST_CASE("FixedController") {
  SECTION("keeps the configured concurrency after completions") {
    FixedController controller{7, 1};

    REQUIRE(controller.recommended_coroutines() == 7);
    controller.register_completion(3);
    REQUIRE(controller.recommended_coroutines() == 7);
  }
}

TEST_CASE("AdaptiveController") {
  SECTION("raises low-sample recommendations without exceeding the maximum") {
    AdaptiveController controller{2, 4, 1, 1, 3};

    controller.register_completion(2);
    REQUIRE(controller.recommended_coroutines() == 3);
    controller.register_completion(2);
    REQUIRE(controller.recommended_coroutines() == 3);
  }

  SECTION("honors a single-point concurrency window") {
    AdaptiveController controller{1, 1, 1, 1, 1};

    controller.register_completion(1);
    controller.register_completion(1);

    REQUIRE(controller.recommended_coroutines() == 1);
  }
}
