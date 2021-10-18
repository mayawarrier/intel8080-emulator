
#ifndef RUN8080_UTIL_HPP
#define RUN8080_UTIL_HPP

#include <cstddef>
#include <cstring>
#include <cassert>
#include <string>
#include <vector>
#include <utility>
#include <type_traits>
#include <functional>

namespace run8080 {
namespace util {

// Prompt user for [yn] response.
bool prompt_yesno(const char prompt[], const bool* default = nullptr);

// Prompt user for string response.
std::string prompt_string(const char prompt[], const char* default = nullptr);

// Read part or all of a binary file. Returns true if successful.
bool read_bin_file(const char fpath[], 
    std::size_t foffset, char* buf, std::size_t buflen,
    // If successful, contains { # bytes read, did reach EOF? }
    std::pair<std::size_t, bool>& out_info);

// Write a binary file. Returns true if successful.
bool write_bin_file(const char fpath[], char* data, std::size_t len);

// Returns true if string is one of the given chars.
inline bool str_is_one_of(const std::string& str, const char chars[]) {
    return str.length() == 1 && str.find_first_of(chars) != std::string::npos;
}

template<typename T>
class array_enumerator
{
private:
    T* arr;
    int start, cur;
    std::function<bool(const array_enumerator<T>&)> in_bounds;

public:
    array_enumerator(T* arr, int start_index, 
        std::function<bool(const array_enumerator<T>&)> in_bounds) :
        arr(arr), start(start_index), cur(start_index), in_bounds(in_bounds)
    {}

    inline const T current(void) const {
        assert(in_bounds(*this) && "Past end.");
        return arr[cur];
    }
    inline int current_index(void) const noexcept { return cur; }

    inline bool advance(void) noexcept {
        if (in_bounds(*this)) {
            cur++;
            return in_bounds(*this);
        } 
        else return false;
    }   
    inline void reset(void) noexcept { cur = start; }

public:
    array_enumerator(T* arr, int start_index, int end_index) :
        arr(arr), start(start_index), cur(start_index)
    {
        in_bounds = [=](const array_enumerator<T>& e) {
            return e.current_index() <= end_index;
        };
    }
};

using opt_callback = std::function<bool(array_enumerator<char*>& args, void* udata)>;

// Make an opt_callback that takes no args and returns success/failure.
template <typename function>
auto make_opt_callback(function& f) -> 
typename std::enable_if<std::is_same<bool, decltype(f())>::value, opt_callback>::type
{
    return [&](array_enumerator<char*>& args, void* udata) -> bool {
        (void)udata;
        args.advance(); // expect no args
        return f();
    };
}

// Make an opt_callback that takes no args and always succeeds.
template <typename function>
auto make_opt_callback(function& f) ->
typename std::enable_if<std::is_same<void, decltype(f())>::value, opt_callback>::type
{
    return [&](array_enumerator<char*>& args, void* udata) -> bool {
        (void)udata;
        args.advance(); // expect no args
        f(); return true;
    };
}

struct opt_info
{
    const char* const full_name;
    const char* const short_name;
    const opt_callback callback;

    opt_info(
        // Full name (eg. --file).
        char* full_name,
        // Short name (eg. -f). Optional.
        char* short_name,
        // Return true if operation is successful.
        // Callback must advance enumerator
        // beyond last extracted argument.
        opt_callback callback) :
        full_name(full_name),
        short_name(short_name),
        callback(callback)
    {
        assert(full_name && std::strlen(full_name) > 0);
        if (short_name) assert(std::strlen(short_name) > 0);
    }

    inline bool has_name(const char* iname) const noexcept {
        return iname && (std::strcmp(iname, full_name) == 0 ||
            (short_name && std::strcmp(iname, short_name) == 0));
    }
};

struct do_opts_info
{
    int nopts_found;
    // Indices of non-option args, sorted in ascending order.
    std::vector<int> remaining_arg_indices;
    int last_extracted_arg_index;
};

// Parse and handle all cmd-line options.
// Returns true if all operations were successful.
bool do_opts(int argc, char** argv,
    const opt_info possible_opts[], int n_possible_opts, 
    void* udata, do_opts_info& out_info);


namespace sigint_pending
{
// (Re)start recording SIGINTs.
// Returns true if successful.
// Not atomic.
bool init(void);

// Acquire exclusive access.
void acquire(void);
// Release exclusive access.
void release(void);

// Returns true if a SIGINT is pending
// (i.e. received but not handled).
bool get(void);

// set(false) to indicate that a
// pending SIGINT has been handled.
// Must call acquire() first.
void set(bool value);
}

}}

#endif
