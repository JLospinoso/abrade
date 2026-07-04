#pragma once
#include <abrade/candidate.hpp>
#include <abrade/options.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace abrade {

/// Produces candidate URI targets for the scraper.
///
/// Implementations return request targets such as `/items/1` or
/// `/search?q=abc`. The generator owns ordering and completion state; callers
/// repeatedly call `next()` until it returns `std::nullopt`.
struct Generator {
  Generator() = default;
  Generator(const Generator&) = default;
  Generator(Generator&&) = default;
  Generator& operator=(const Generator&) = default;
  Generator& operator=(Generator&&) = default;
  virtual ~Generator() = default;
  /// Returns the next URI target, or `std::nullopt` after the input is exhausted.
  virtual std::optional<std::string> next() = 0;
};

/// Generates one URI target per line from standard input.
///
/// Lines are returned exactly as read, without URI normalization. This lets
/// upstream tools control candidate generation while Abrade handles transport,
/// query execution, and output.
struct StdinGenerator : Generator {
  std::optional<std::string> next() override;

private:
  bool is_complete{};
};

/// Parsed brace token inside a URI pattern template.
///
/// This is an internal parser value used while building `UriGenerator`. `start`
/// and `end` are offsets in the original pattern string. `tokens` stores either
/// the explicit range endpoints or the implicit range domain symbols.
struct Pattern {
  size_t start{};
  size_t end{};
  enum class Type { Implicit, Explicit, Continuation } type{Type::Implicit};
  std::pair<std::string, std::string> tokens;
};

/// Counter domain used by `UriGenerator` pattern expansion.
///
/// Ranges expose both exact and logarithmic cardinality. Exact cardinality is
/// useful for ordinary runs; logarithmic cardinality keeps very large generated
/// spaces inspectable without overflowing `size_t`.
struct Range {
  Range() = default;
  Range(const Range&) = default;
  Range(Range&&) = default;
  Range& operator=(const Range&) = default;
  Range& operator=(Range&&) = default;
  virtual ~Range() = default;
  /// Advances the range and returns true when it carried past the final value.
  virtual bool increment_return_carry() = 0;
  /// Returns the current textual value for insertion into the URI target.
  virtual std::string get_current() const = 0;
  /// Returns exact cardinality, or throws if the value cannot fit in `size_t`.
  virtual size_t size() const = 0;
  /// Returns natural-log cardinality.
  virtual double log_size() const = 0;
  /// Resets the range to its first value.
  virtual void reset() = 0;
};

/// Inclusive numeric range such as `{1:10}`.
///
/// Values are emitted as base-10 numbers with no zero padding. The constructor
/// rejects descending ranges.
struct ExplicitRange : Range {
  ExplicitRange(size_t range_start, size_t range_end);
  bool increment_return_carry() override;
  std::string get_current() const override;
  size_t size() const override;
  double log_size() const override;
  void reset() override;

private:
  const size_t start, end;
  size_t current;
};

/// Character-domain range such as `{ddd}`, `{hh}`, or `{a}`.
///
/// Each pattern character selects a domain. The rightmost character increments
/// fastest. When `preserve_leading_zeros` is false, leading zero-domain
/// positions are suppressed except for the final position.
struct ImplicitRange : Range {
  ImplicitRange(const std::string& pattern, bool preserve_leading_zeros);
  bool increment_return_carry() override;
  std::string get_current() const override;
  size_t size() const override;
  double log_size() const override;
  void reset() override;

private:
  bool increment_return_carry(size_t pivot);
  std::vector<std::string_view> domains;
  std::vector<size_t> digits;
  const bool leading_zeros;
};

/// Expands suffixes of an implicit pattern from shortest to longest.
///
/// `{ddd}` in telescoping mode behaves like `{d}`, then `{dd}`, then `{ddd}`.
/// This is useful when the target identifier width is unknown.
struct TelescopingRange : Range {
  TelescopingRange(const std::string& pattern, bool lead_zero);
  bool increment_return_carry() override;
  std::string get_current() const override;
  size_t size() const override;
  double log_size() const override;
  void reset() override;

private:
  std::vector<ImplicitRange> ranges;
  size_t index;
};

/// Mirrors the current value of a previous range.
///
/// Continuation ranges back the `{}` pattern form. They never add cardinality;
/// they only read the target range's current textual value.
struct ContinuationRange : Range {
  explicit ContinuationRange(const Range& target_range);
  bool increment_return_carry() override;
  std::string get_current() const override;
  size_t size() const override;
  double log_size() const override;
  void reset() override;

private:
  const Range& target;
};

/// Generates URI targets from Abrade's brace-pattern syntax.
///
/// Literal text is interleaved with `Range` instances. Multiple independent
/// ranges form a Cartesian product, with the rightmost range changing fastest.
struct UriGenerator : Generator {
  UriGenerator(const std::string& input, bool lead_zero, bool is_telescoping);
  std::optional<std::string> next() override;
  /// Returns the natural log of the generated URI target count.
  double get_log_range_size() const;
  /// Returns the exact generated URI target count, or throws on `size_t` overflow.
  size_t get_range_size() const;

private:
  void increment_ranges();
  std::vector<std::string> literal_tokens;
  std::vector<std::unique_ptr<Range>> ranges;
  bool is_complete;
};
} // namespace abrade
