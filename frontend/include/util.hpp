
#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstddef>
#include <string>
#include <utility>

namespace testi8080 {
namespace util {

// Prompt user for [yn] response.
bool prompt_yesno(const char prompt[], const bool* default = nullptr);

// Prompt user for string response.
std::string prompt_string(const char prompt[], const char* default = nullptr);

// Read part or all of a binary file. Returns {reached eof, # bytes read}.
std::pair<bool, std::size_t> read_bin_file(const char fpath[], std::size_t offset, char* buf, std::size_t buflen);

// Write a binary file.
bool write_bin_file(const char fpath[], char* data, std::size_t len);

}}

#endif
