#pragma once
#include "Options.h"
#include <string>
#include <utility>
#include <optional>
//#include <boost/optional/optional_io.hpp>
#include <vector>
#include <memory>

struct Generator {
  virtual ~Generator() = default;
  virtual std::optional<std::string> next() = 0;
};

struct StdinGenerator : Generator {
  std::optional<std::string> next() override;
private:
  bool is_complete{};
};

struct Pattern {
  size_t start, end;
  enum class Type {
    Implicit,
    Explicit,
    Continuation
  } type;
  std::pair<std::string, std::string> tokens;
};

struct Range {
  virtual ~Range() = default;
  virtual bool increment_return_carry() = 0;
  virtual std::string get_current() const = 0;
  virtual size_t size() const = 0;
  virtual double log_size() const = 0;
  virtual void reset() = 0;
};

struct ExplicitRange : Range {
  ExplicitRange(size_t start, size_t end);
  bool increment_return_carry() override;
  std::string get_current() const override;
  size_t size() const override;
  double log_size() const override;
  void reset() override;
private:
  const size_t start, end;
  size_t current;
};

struct ImplicitRange : Range {
  ImplicitRange(const std::string& pattern, bool lead_zero);
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

struct ContinuationRange : Range {
  ContinuationRange(const Range& target);
  bool increment_return_carry() override;
  std::string get_current() const override;
  size_t size() const override;
  double log_size() const override;
  void reset() override;
private:
  const Range& target;
};

struct UriGenerator : Generator {
  UriGenerator(const std::string& pattern_in, bool lead_zero, bool is_exact);
  std::optional<std::string> next() override;
  double get_log_range_size() const;
  size_t get_range_size() const;
private:
  void increment_ranges();
  std::vector<std::string> literal_tokens;
  std::vector<std::unique_ptr<Range>> ranges;
  bool is_complete;
};