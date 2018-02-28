#include <boost/lexical_cast.hpp>
#include "Generator.h"
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <iostream>

using namespace std;

namespace {
  const string octal{"01234567"}; // o
  const string digits{"0123456789"}; // d
  const string hex_lower{"0123456789abcdef"}; // h
  const string hex_upper{"0123456789ABCDEF"}; // H
  const string alpha_lower{"abcdefghijklmnopqrstuvwxyz"}; // a
  const string alpha_upper{"ABCDEFGHIJKLMNOPQRSTUVWXYZ"}; // A
  const string alphanumeric_lower{"0123456789abcdefghijklmnopqrstuvwxyz"}; // n
  const string alphanumeric_upper{"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"}; // N
  const string alphanumeric{"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"}; // b

  const string* get_domain(char element) {
    switch (element) {
    case 'o':
      return &octal;
    case 'd':
      return &digits;
    case 'h':
      return &hex_lower;
    case 'H':
      return &hex_upper;
    case 'a':
      return &alpha_lower;
    case 'A':
      return &alpha_upper;
    case 'n':
      return &alphanumeric_lower;
    case 'N':
      return &alphanumeric_upper;
    case 'b':
      return &alphanumeric;
    default:
      throw runtime_error{ string{"Unknown implicit range element: "} +element };
    }
  }

  std::optional<Pattern> parse_next_pattern(const string& input, size_t start) {
    Pattern pattern;
    pattern.type = Pattern::Type::Implicit;
    pattern.start = input.find('{', start);
    if (pattern.start == string::npos) {
      if (input.find('}', start) != string::npos) throw runtime_error{ "Unmatched closing brace found (})." };
      return nullopt;
    }
    pattern.end = input.find('}', pattern.start + 1);
    if (pattern.end == string::npos)
      throw runtime_error{ "Unmatched open brace at " + to_string(pattern.start) };
    if (pattern.end == pattern.start + 1) {
      pattern.type = Pattern::Type::Continuation;
      return pattern;
    }
    for (auto element_iter = input.begin() + pattern.start + 1;
         element_iter < input.begin() + pattern.end;
         ++element_iter) {
      const auto element = *element_iter;
      if (element == ':') { pattern.type = Pattern::Type::Explicit; }
      else {
        auto& target = pattern.type == Pattern::Type::Implicit ? pattern.tokens.first : pattern.tokens.second;
        target.push_back(element);
      }
    }
    return move(pattern);
  }

}


ImplicitRange::ImplicitRange(const string& pattern, bool leading_zeros)
  : leading_zeros {leading_zeros} {
  for (auto element : pattern) {
    domains.emplace_back(get_domain(element));
    digits.emplace_back(0);
  }
}

void ImplicitRange::reset() { }

double ImplicitRange::log_size() const {
  return std::accumulate(domains.begin(), domains.end(), double{0},
                         [](const auto& a, const auto& b) {
                         return a + log(b->size());
                       });
}

size_t ImplicitRange::size() const {
  return std::accumulate(domains.begin(), domains.end(), size_t{1},
                         [](const auto& a, const auto& b) {
                         auto size = b->size();
                         auto new_result = a * size;
                         if (size != 0 && new_result / size != a) throw overflow_error{"Range size too large for size_t. Use logarithm."};
                         return new_result;
                       });
}

bool ImplicitRange::increment_return_carry(size_t pivot) {
  ++digits[pivot];
  if (digits[pivot] < domains[pivot]->size()) return false;
  digits[pivot] = 0;
  return true;
}

bool ImplicitRange::increment_return_carry() {
  auto pivot = digits.size() - 1;
  while (increment_return_carry(pivot)) {
    if (pivot == 0) return true;
    --pivot;
  }
  return false;
}

string ImplicitRange::get_current() const {
  string result;
  auto is_leading_zero{true};
  for (size_t i{}; i < digits.size(); i++) {
    const auto digit = digits[i];
    is_leading_zero &= !leading_zeros && digit == 0 && i != digits.size() - 1;
    if (is_leading_zero) continue;
    const auto& domain = *domains[i];
    const auto element = domain[digit];
    result += element;
  }
  return result;
}

ExplicitRange::ExplicitRange(size_t start, size_t end) : start{start}, end{end} {
  if (end < start) throw runtime_error{ "End of pattern cannot be less than start." };
  current = start;
}

void ExplicitRange::reset() { current = start; }

double ExplicitRange::log_size() const { return log(size()); }

size_t ExplicitRange::size() const { return end - start + 1; }

bool ExplicitRange::increment_return_carry() { return current++ == end; }

string ExplicitRange::get_current() const { return to_string(current); }

UriGenerator::UriGenerator(const string& input, bool lead_zero, bool is_telescoping) : is_complete{false} {
  size_t index{};
  while (auto pattern = parse_next_pattern(input, index)) {
    const auto literal_length = pattern->start - index;
    literal_tokens.emplace_back(input.substr(index, literal_length));
    switch (pattern->type) {
    case Pattern::Type::Explicit:
      size_t start, end;
      try { start = boost::lexical_cast<size_t>(pattern->tokens.first); }
      catch (boost::bad_lexical_cast) { throw runtime_error{ "Unable to parse pattern " + pattern->tokens.first }; }
      try { end = boost::lexical_cast<size_t>(pattern->tokens.second); }
      catch (boost::bad_lexical_cast) { throw runtime_error{ "Unable to parse pattern " + pattern->tokens.second }; }
      ranges.emplace_back(make_unique<ExplicitRange>(start, end));
      break;
    case Pattern::Type::Implicit:
      if (is_telescoping) {
        ranges.emplace_back(make_unique<TelescopingRange>(pattern->tokens.first, lead_zero));
      }
      else {
        ranges.emplace_back(make_unique<ImplicitRange>(pattern->tokens.first, lead_zero));
      }
      break;

    case Pattern::Type::Continuation:
      if (ranges.size() == 0) throw runtime_error{ "Cannot start with a continuation pattern {}." };
      ranges.emplace_back(make_unique<ContinuationRange>(*ranges.back()));
      break;
    default:
      throw runtime_error{ "Unknown range type encountered." };
    }
    index = pattern->end + 1;
  }
  literal_tokens.emplace_back(input.substr(index));
}

std::optional<string> StdinGenerator::next() {
  if (is_complete) return nullopt;
  string next_line;
  if (getline(cin, next_line)) return next_line;
  is_complete = true;
  return nullopt;
}

std::optional<string> UriGenerator::next() {
  if (is_complete) return nullopt;
  string result;
  for (size_t i{}; i < ranges.size(); i++) {
    result.append(literal_tokens[i]);
    result.append(ranges[i]->get_current());
  }
  result.append(literal_tokens.back());
  increment_ranges();
  return result;
}

void UriGenerator::increment_ranges() {
  if (ranges.empty()) {
    is_complete = true;
    return;
  }
  auto pivot = ranges.size() - 1;
  while (ranges[pivot]->increment_return_carry()) {
    ranges[pivot]->reset();
    if (pivot == 0) {
      is_complete = true;
      return;
    }
    --pivot;
  }
}

size_t UriGenerator::get_range_size() const {
  return std::accumulate(ranges.begin(), ranges.end(),
                         size_t{1},
                         [](const auto& a, const auto& b) { return a * b->size(); });
}

double UriGenerator::get_log_range_size() const {
  return log(std::accumulate(ranges.begin(), ranges.end(),
                             double{},
                             [](const auto& a, const auto& b) { return a + exp(b->log_size()); }));
}

TelescopingRange::TelescopingRange(const string& pattern_template, bool lead_zero) : index{} {
  for (size_t index{}; index < pattern_template.size(); index++) {
    const auto start = pattern_template.size() - index - 1;
    const auto length = index + 1;
    ranges.emplace_back(ImplicitRange{pattern_template.substr(start, length), lead_zero});
  }
}

bool TelescopingRange::increment_return_carry() {
  if (ranges[index].increment_return_carry()) {
    ++index;
    if (ranges.size() == index) return true;
  }
  return false;
}

string TelescopingRange::get_current() const {
  return ranges[index].get_current();
}

size_t TelescopingRange::size() const {
  return std::accumulate(ranges.begin(), ranges.end(),
                         size_t{},
                         [](const auto& a, const auto& b) { return a + b.size(); });
}

double TelescopingRange::log_size() const {
  return std::accumulate(ranges.begin(), ranges.end(),
                         double{},
                         [](const auto& a, const auto& b) { return a + b.log_size(); });
}

void TelescopingRange::reset() {
  for (auto& range : ranges) {
    range.reset();
  }
  index = 0;
}

ContinuationRange::ContinuationRange(const Range& target) : target{target} { }

bool ContinuationRange::increment_return_carry() {
  return true;
}

string ContinuationRange::get_current() const {
  return target.get_current();
}

size_t ContinuationRange::size() const {
  return 1;
}

double ContinuationRange::log_size() const {
  return 0;
}

void ContinuationRange::reset() { }
