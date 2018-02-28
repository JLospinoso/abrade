#pragma once
#include <vector>
#include <map>
#include <string>

struct Candidate {
  std::string_view description() const { return uri; }
  std::string uri;
  std::map<std::string, std::string> headers;
  std::vector<char> contents;
};
