
#include <cstddef>
#include <cassert>
#include <csignal>

#include <limits>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <type_traits>
#include <atomic>

#include <util.hpp>

static volatile std::sig_atomic_t SIGINT_PENDING;
static std::atomic_flag SIGINT_PENDING_lock;

static inline void acquire_SIGINT_PENDING(void)
{
    while (SIGINT_PENDING_lock.test_and_set(std::memory_order_acquire));
}
static inline void release_SIGINT_PENDING(void)
{
    SIGINT_PENDING_lock.clear(std::memory_order_release);
}

extern "C" void sigint_handler(int sig) {
    (void)sig;
    acquire_SIGINT_PENDING();
    SIGINT_PENDING = 1;
    release_SIGINT_PENDING();
}

namespace run8080 {
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

// Returns true if A can be cast to B without data loss.
template <typename A, typename B>
static constexpr typename std::enable_if<std::is_unsigned<A>::value && std::is_arithmetic<B>::value, bool>::type
can_cast(A value) noexcept {
    return value <= (typename std::make_unsigned<B>::type)std::numeric_limits<B>::max();
}

bool read_bin_file(const char fpath[], std::size_t foffset, 
    char* buf, std::size_t buflen, std::pair<std::size_t, bool>& out_info)
{
    assert((buf &&
        can_cast<std::size_t, std::streamoff>(foffset) &&
        can_cast<std::size_t, std::streamsize>(buflen)));
        
    std::ifstream file(fpath, std::ios::binary);
    if (!file) {
        std::cerr << "\nCould not open " << fpath << ".";
        return false;
    }

    file.seekg((std::streamoff)foffset, std::ios::beg); // only okay for binary files!
    assert(file.good() && "Invalid offset");

    file.read(buf, (std::streamsize)buflen);
    std::size_t nread = file.gcount();                 // only okay for binary files!

    if (file.eof()) { out_info = std::make_pair(nread, true); }
    else if (file.good()) { out_info = std::make_pair(nread, false); }
    else {
        assert(file.fail());
        std::cerr << "\nCould not read from " << fpath << ".";
        return false;
    }

    return true;
}

bool write_bin_file(const char path[], char* data, std::size_t len)
{
    assert((data && can_cast<std::size_t, std::streamsize>(len)));

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "\nCould not open " << path << ".";
        return false;
    }

    file.write(data, (std::streamsize)len);
    if (file.fail()) {
        std::cerr << "\nCould not write to " << path << ".";
        return false;
    }

    return true;
}

bool do_opts(int argc, char** argv,
    const opt_info possible_opts[], int n_possible_opts,
    void* udata, do_opts_info& out_info)
{
    auto get_opt = [&](const char* name) -> const opt_info* 
    {
        const opt_info* match = nullptr;
        for (int i = 0; i < n_possible_opts; ++i) {
            const opt_info* opt = possible_opts + i;
            if (opt->has_name(name)) {
                match = opt;
                break;
            }
        }
        return match;
    };

    out_info.nopts_found = 0;
    out_info.remaining_arg_indices.clear();
    out_info.last_extracted_arg_index = -1;
    if (!argv || argc < 2) return true; // first arg is program name

    bool success = true;   
    for (int arg_index = 1; arg_index < argc;)
    {
        char* arg = argv[arg_index++];
        const opt_info* opt = get_opt(arg);
        if (!opt) {
            out_info.remaining_arg_indices.push_back(arg_index - 1);
            continue;
        }

        array_enumerator<char*> opt_args_enumerator(argv, arg_index, [&]
        (const array_enumerator<char*>& e) {
            // valid until next option or end of argv
            return e.current_index() < argc && !get_opt(e.current());
        });

        success &= opt->callback(opt_args_enumerator, udata);
        arg_index = opt_args_enumerator.current_index(); // past option args
        out_info.last_extracted_arg_index = arg_index - 1;
        out_info.nopts_found++;
    }

    return success;
}

namespace sigint_pending
{
bool init(void)
{
    SIGINT_PENDING = 0;
    SIGINT_PENDING_lock.clear(std::memory_order_relaxed);
    return std::signal(SIGINT, sigint_handler) != SIG_ERR;
}

void acquire(void) { acquire_SIGINT_PENDING(); }
void release(void) { release_SIGINT_PENDING(); }

void set(bool value) { SIGINT_PENDING = value ? 1 : 0; }
bool get(void) { return SIGINT_PENDING == 1 ? true : false; }
}

}}
