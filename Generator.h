#pragma once
#include "Options.h"
#include <string>
#include <utility>
#include <optional>
#include <vector>
#include <memory>
#include "Candidate.h"

/// Source of candidate URI paths for the scraper.
struct Generator {
  virtual ~Generator() = default;
  /// Returns the next URI path, or std::nullopt when generation is complete.
  virtual std::optional<std::string> next() = 0;
};

/// Generates one URI path per line from standard input.
struct StdinGenerator : Generator {
  std::optional<std::string> next() override;
private:
  bool is_complete{};
};

/// Parsed pattern token inside a URI generation template.
struct Pattern {
  size_t start{};
  size_t end{};
  enum class Type {
    Implicit,
    Explicit,
    Continuation
  } type{Type::Implicit};
  std::pair<std::string, std::string> tokens;
};

/// Counter domain used by UriGenerator pattern expansions.
struct Range {
  virtual ~Range() = default;
  virtual bool increment_return_carry() = 0;
  virtual std::string get_current() const = 0;
  virtual size_t size() const = 0;
  virtual double log_size() const = 0;
  virtual void reset() = 0;
};

/// Numeric inclusive range such as {1:10}.
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

/// Character-domain range such as {ddd}, {hh}, or {a}.
struct ImplicitRange : Range {
  ImplicitRange(const std::string& pattern, bool preserve_leading_zeros);
  bool increment_return_carry() override;
  std::string get_current() const override;
  size_t size() const override;
  double log_size() const override;
  void reset() override;
private:
  bool increment_return_carry(size_t digit);
  std::vector<const std::string*> domains;
  std::vector<size_t> digits;
  const bool leading_zeros;
};

/// Expands the suffixes of an implicit pattern, shortest to longest.
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

/// Mirrors the current value of the previous range.
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

/// Generates URI paths from Abrade's brace-pattern syntax.
struct UriGenerator : Generator {
  UriGenerator(const std::string& pattern_in, bool lead_zero, bool is_telescoping);
  std::optional<std::string> next() override;
  /// Returns the natural log of the number of generated URI paths.
  double get_log_range_size() const;
  /// Returns the exact number of generated URI paths, or throws on size_t overflow.
  size_t get_range_size() const;
private:
  void increment_ranges();
  std::vector<std::string> literal_tokens;
  std::vector<std::unique_ptr<Range>> ranges;
  bool is_complete;
};
