
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <test.hpp>

int i8main(int argc, char** argv)
{
	int err;
	const char* arg = 0;

	// change for platform or usage
	constexpr auto argmax = 5;
	constexpr auto args_start_at = 1;

	if (argv && argc >= args_start_at) {
		for (int i = args_start_at; i < std::min(argc, argmax); ++i) {
			if (argv[i]) {
				// use the first valid string
				arg = argv[i];
				break;
			}
		}
	}

	if (!arg || std::strlen(arg) == 0) {
		err = libi8080_default_tests();
	} else {
		err = libi8080_user_test(arg);
	}

	system("pause");

	if (err) return EXIT_FAILURE;
	else return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
	return i8main(argc, argv);
}
