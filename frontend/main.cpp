
#include <iostream>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <test.hpp>
#include "i8080.h"

int i8main(int argc, char** argv)
{
	int err;
	std::string test_path;

	// change for platform or usage
	const int argmax = 5;
	const int args_start_at = 1;

	if (argv && argc >= args_start_at) {
		for (int i = args_start_at; i < std::min(argc, argmax); ++i) {
			if (argv[i]) {
				// use the first valid string
				test_path = argv[i];
				break;
			}
		}
	}

	if (test_path.empty()) {
		err = libi8080_default_tests();
	} else {
		err = libi8080_user_test(test_path);
	}

	if (err) return EXIT_FAILURE;
	else return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
	return i8main(argc, argv);
}
