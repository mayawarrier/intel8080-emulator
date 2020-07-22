
#ifndef FRONTEND_TEST_H
#define FRONTEND_TEST_H

#include <string>

int i8080_run_default_tests();

int i8080_run_user_test(std::string test_path);

void i8080_dump_stats(struct i8080& i8080);

#endif
