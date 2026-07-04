#include <abrade/generator.hpp>
#include <algorithm>
#include <array>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace abrade {

using namespace std;

namespace {
struct DomainDefinition {
  char symbol{};
  string_view values;
};

constexpr array domain_definitions{
    DomainDefinition{.symbol = 'o', .values = "01234567"},
    DomainDefinition{.symbol = 'd', .values = "0123456789"},
    DomainDefinition{.symbol = 'h', .values = "0123456789abcdef"},
    DomainDefinition{.symbol = 'H', .values = "0123456789ABCDEF"},
    DomainDefinition{.symbol = 'a', .values = "abcdefghijklmnopqrstuvwxyz"},
    DomainDefinition{.symbol = 'A', .values = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"},
    DomainDefinition{.symbol = 'n', .values = "0123456789abcdefghijklmnopqrstuvwxyz"},
    DomainDefinition{.symbol = 'N', .values = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"},
    DomainDefinition{.symbol = 'b',
                     .values = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"},
};

size_t checked_add(size_t left, size_t right) {
  if (right > std::numeric_limits<size_t>::max() - left) {
    throw overflow_error{"Range size too large for size_t. Use logarithm."};
  }
  return left + right;
}

size_t checked_multiply(size_t left, size_t right) {
  if (right != 0 && left > std::numeric_limits<size_t>::max() / right) {
    throw overflow_error{"Range size too large for size_t. Use logarithm."};
  }
  return left * right;
}

string_view get_domain(char element) {
  const auto domain =
      ranges::find_if(domain_definitions, [element](const auto& definition) noexcept {
        return definition.symbol == element;
      });
  if (domain == domain_definitions.end()) {
    throw runtime_error{string{"Unknown implicit range element: "} + element};
  }
  return domain->values;
}

std::optional<Pattern> parse_next_pattern(const string& input, size_t start) {
  Pattern pattern;
  pattern.type = Pattern::Type::Implicit;
  pattern.start = input.find('{', start);
  if (pattern.start == string::npos) {
    if (input.find('}', start) != string::npos) {
      throw runtime_error{"Unmatched closing brace found (})."};
    }
    return nullopt;
  }
  pattern.end = input.find('}', pattern.start + 1);
  if (pattern.end == string::npos) {
    throw runtime_error{"Unmatched open brace at " + to_string(pattern.start)};
  }
  if (pattern.end == pattern.start + 1) {
    pattern.type = Pattern::Type::Continuation;
    return pattern;
  }
  const auto token_start =
      std::next(input.begin(), static_cast<string::difference_type>(pattern.start + 1));
  const auto token_end =
      std::next(input.begin(), static_cast<string::difference_type>(pattern.end));
  for (auto element_iter = token_start; element_iter < token_end; ++element_iter) {
    const auto element = *element_iter;
    if (element == ':') {
      pattern.type = Pattern::Type::Explicit;
    } else {
      auto& target =
          pattern.type == Pattern::Type::Implicit ? pattern.tokens.first : pattern.tokens.second;
      target.push_back(element);
    }
  }
  return pattern;
}

} // namespace

ImplicitRange::ImplicitRange(const string& pattern, bool preserve_leading_zeros)
    : leading_zeros{preserve_leading_zeros} {
  for (auto element : pattern) {
    domains.emplace_back(get_domain(element));
    digits.emplace_back(0);
  }
}

void ImplicitRange::reset() {}

double ImplicitRange::log_size() const {
  return std::accumulate(domains.begin(), domains.end(), double{0},
                         [](const auto& a, const auto& b) { return a + log(b.size()); });
}

size_t ImplicitRange::size() const {
  return std::accumulate(domains.begin(), domains.end(), size_t{1},
                         [](const auto& a, const auto& b) {
                           auto size = b.size();
                           return checked_multiply(a, size);
                         });
}

bool ImplicitRange::increment_return_carry(size_t pivot) {
  auto& digit = digits.at(pivot);
  ++digit;
  if (digit < domains.at(pivot).size()) {
    return false;
  }
  digit = 0;
  return true;
}

bool ImplicitRange::increment_return_carry() {
  auto pivot = digits.size() - 1;
  while (increment_return_carry(pivot)) {
    if (pivot == 0) {
      return true;
    }
    --pivot;
  }
  return false;
}

string ImplicitRange::get_current() const {
  string result;
  auto is_leading_zero{true};
  for (size_t i{}; i < digits.size(); i++) {
    const auto digit = digits.at(i);
    is_leading_zero &= !leading_zeros && digit == 0 && i != digits.size() - 1;
    if (is_leading_zero) {
      continue;
    }
    const auto domain = domains.at(i);
    const auto element = domain.at(digit);
    result += element;
  }
  return result;
}

ExplicitRange::ExplicitRange(size_t range_start, size_t range_end)
    : start{range_start}, end{range_end} {
  if (range_end < range_start) {
    throw runtime_error{"End of pattern cannot be less than start."};
  }
  current = range_start;
}

void ExplicitRange::reset() { current = start; }

double ExplicitRange::log_size() const {
  const auto cardinality = static_cast<long double>(end) - static_cast<long double>(start) + 1.0L;
  return static_cast<double>(log(cardinality));
}

size_t ExplicitRange::size() const { return checked_add(end - start, 1); }

bool ExplicitRange::increment_return_carry() { return current++ == end; }

string ExplicitRange::get_current() const { return to_string(current); }

UriGenerator::UriGenerator(const string& input, bool lead_zero, bool is_telescoping)
    : is_complete{false} {
  size_t index{};
  while (auto pattern = parse_next_pattern(input, index)) {
    const auto literal_length = pattern->start - index;
    literal_tokens.emplace_back(input.substr(index, literal_length));
    switch (pattern->type) {
    case Pattern::Type::Explicit: {
      size_t start{};
      size_t end{};
      try {
        start = boost::lexical_cast<size_t>(pattern->tokens.first);
      } catch (const boost::bad_lexical_cast&) {
        throw runtime_error{"Unable to parse pattern " + pattern->tokens.first};
      }
      try {
        end = boost::lexical_cast<size_t>(pattern->tokens.second);
      } catch (const boost::bad_lexical_cast&) {
        throw runtime_error{"Unable to parse pattern " + pattern->tokens.second};
      }
      ranges.emplace_back(make_unique<ExplicitRange>(start, end));
      break;
    }
    case Pattern::Type::Implicit:
      if (is_telescoping) {
        ranges.emplace_back(make_unique<TelescopingRange>(pattern->tokens.first, lead_zero));
      } else {
        ranges.emplace_back(make_unique<ImplicitRange>(pattern->tokens.first, lead_zero));
      }
      break;

    case Pattern::Type::Continuation:
      if (ranges.empty()) {
        throw runtime_error{"Cannot start with a continuation pattern {}."};
      }
      ranges.emplace_back(make_unique<ContinuationRange>(*ranges.back()));
      break;
    default:
      throw runtime_error{"Unknown range type encountered."};
    }
    index = pattern->end + 1;
  }
  literal_tokens.emplace_back(input.substr(index));
}

std::optional<string> StdinGenerator::next() {
  if (is_complete) {
    return nullopt;
  }
  string next_line;
  if (getline(cin, next_line)) {
    return next_line;
  }
  is_complete = true;
  return nullopt;
}

std::optional<string> UriGenerator::next() {
  if (is_complete) {
    return nullopt;
  }
  string result;
  for (size_t i{}; i < ranges.size(); i++) {
    result.append(literal_tokens.at(i));
    result.append(ranges.at(i)->get_current());
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
  while (ranges.at(pivot)->increment_return_carry()) {
    (*ranges.at(pivot)).reset();
    if (pivot == 0) {
      is_complete = true;
      return;
    }
    --pivot;
  }
}

size_t UriGenerator::get_range_size() const {
  return std::accumulate(ranges.begin(), ranges.end(), size_t{1}, [](const auto& a, const auto& b) {
    return checked_multiply(a, b->size());
  });
}

double UriGenerator::get_log_range_size() const {
  return std::accumulate(ranges.begin(), ranges.end(), double{},
                         [](const auto& a, const auto& b) { return a + b->log_size(); });
}

TelescopingRange::TelescopingRange(const string& pattern_template, bool lead_zero) : index{} {
  for (size_t offset{}; offset < pattern_template.size(); offset++) {
    const auto start = pattern_template.size() - offset - 1;
    const auto length = offset + 1;
    ranges.emplace_back(pattern_template.substr(start, length), lead_zero);
  }
}

bool TelescopingRange::increment_return_carry() {
  if (ranges.at(index).increment_return_carry()) {
    ++index;
    if (ranges.size() == index) {
      return true;
    }
  }
  return false;
}

string TelescopingRange::get_current() const { return ranges.at(index).get_current(); }

size_t TelescopingRange::size() const {
  return std::accumulate(ranges.begin(), ranges.end(), size_t{},
                         [](const auto& a, const auto& b) { return checked_add(a, b.size()); });
}

double TelescopingRange::log_size() const {
  const auto largest = std::ranges::max_element(ranges, [](const auto& left, const auto& right) {
    return left.log_size() < right.log_size();
  });
  if (largest == ranges.end()) {
    return 0;
  }

  const auto max_log = largest->log_size();
  const auto scaled_sum = std::accumulate(ranges.begin(), ranges.end(), double{},
                                          [max_log](const auto& sum, const auto& range) {
                                            return sum + exp(range.log_size() - max_log);
                                          });
  return max_log + log(scaled_sum);
}

void TelescopingRange::reset() {
  for (auto& range : ranges) {
    range.reset();
  }
  index = 0;
}

ContinuationRange::ContinuationRange(const Range& target_range) : target{target_range} {}

bool ContinuationRange::increment_return_carry() { return true; }

string ContinuationRange::get_current() const { return target.get_current(); }

size_t ContinuationRange::size() const { return 1; }

double ContinuationRange::log_size() const { return 0; }

void ContinuationRange::reset() {}
} // namespace abrade
