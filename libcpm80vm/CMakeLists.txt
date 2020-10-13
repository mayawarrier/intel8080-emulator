
cmake_minimum_required(VERSION 3.1.3)

# Depends on libi8080
add_library(cpm80vm src/vm.c src/strutil.c)
target_include_directories(cpm80vm
	PUBLIC
		$<INSTALL_INTERFACE:include/cpm80vm>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/cpm80vm>
	PRIVATE include/ include/cpm80vm/internal/
)
target_link_libraries(cpm80vm PUBLIC i8080)

# Depends on libcpm80vm
if (BUILD_EXAMPLES)
	add_library(test_vm examples/test_vm/test_vm.c)
	target_include_directories(test_vm PUBLIC examples/test_vm)
	target_link_libraries(test_vm PUBLIC cpm80vm)
endif()
