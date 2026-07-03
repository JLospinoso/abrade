#pragma once
#include <vector>
#include <map>
#include <string>

/// Request candidate generated from a URI pattern.
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
