#pragma once
#include <map>
#include <string>
#include <vector>

namespace abrade {

/// Request candidate generated from a URI pattern or stdin input.
///
/// The CLI currently uses URI-only candidates. Headers and contents are kept as
/// explicit future extension points so request-level features can grow without
/// changing the scraper orchestration shape.
struct Candidate {
  /// Returns the user-facing identifier used in logs and output records.
  std::string_view description() const { return uri; }

  /// URI target path, including any query string.
  std::string uri;
  /// Future extension point for candidate-specific request headers.
  std::map<std::string, std::string> headers;
  /// Future extension point for candidate-specific request bodies.
  std::vector<char> contents;
};
} // namespace abrade
