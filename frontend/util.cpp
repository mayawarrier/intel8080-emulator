
#include <cstddef>
#include <cassert>

#include <limits>
#include <string>
#include <fstream>
#include <iostream>

#include <util.hpp>

static inline bool str_is_one_of(const std::string& str, const char chars[]) {
	return str.length() == 1 && str.find_first_of(chars) != std::string::npos;
}

namespace testi8080 {
namespace util {

bool prompt_yesno(const char prompt[], const bool* default_val)
{
    std::string line;
    if (!default_val) {
        while (true) {
            std::cout << prompt << " [yn] ";
            std::getline(std::cin, line);
            if (str_is_one_of(line, "YyNn")) return line == "Y" || line == "y";
        }
    } else {
        std::cout << prompt << " [yn] (default:" << *default_val ? "y) " : "n) ";
        std::getline(std::cin, line);
        if (str_is_one_of(line, "YyNn")) return line == "Y" || line == "y";
        else return *default_val;
    }
}

std::string prompt_string(const char prompt[], const char* default_val)
{
    std::string line;
    if (!default_val) {
        while (true) {
            std::cout << prompt << " ";
            std::getline(std::cin, line);
            if (line.length() > 0) return line;
        }
    } else {
        std::cout << prompt << " (default:" << default_val << ") ";
        std::getline(std::cin, line);
        return line.length() > 0 ? line : default_val;
    }
}

// Returns {reached eof, # bytes read}
std::pair<bool, std::size_t> read_bin_file(const char path[], std::size_t offset, char* buf, std::size_t buflen)
{
    constexpr auto res_failure = std::make_pair(false, (std::size_t)0);

    bool args_ok = buf &&
        (sizeof(std::size_t) < sizeof(std::streamoff) || 
            offset <= (std::size_t)std::numeric_limits<std::streamoff>::max()) &&
        (sizeof(std::size_t) < sizeof(std::streamsize) || 
            buflen <= (std::size_t)std::numeric_limits<std::streamsize>::max());
    if (!args_ok) return res_failure;

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "\nCould not open " << path << ".";
        return res_failure;
    }

    file.seekg((std::streamoff)offset, std::ios::beg); // only okay for binary files!
    assert(file.good() && "Invalid offset");

    file.read(buf, (std::streamsize)buflen);
    std::size_t nread = file.gcount();                 // only okay for binary files!

    if (file.eof()) return std::make_pair(true, nread);
    else if (file.good()) return std::make_pair(false, nread); // more to read
    else {
        assert(file.fail());
        std::cerr << "\nCould not read from " << path << ".";
        return res_failure;
    }
}

bool write_bin_file(const char path[], char* data, std::size_t len)
{
    bool args_ok = data &&
        (sizeof(std::size_t) < sizeof(std::streamsize) || 
            len <= (std::size_t)std::numeric_limits<std::streamsize>::max());
    if (!args_ok) return false;

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "\nCould not open " << path << ".";
        return false;
    }

    file.write(data, len);
    if (file.fail()) {
        std::cerr << "\nCould not write to " << path << ".";
        return false;
    }
    return true;
}

}}
