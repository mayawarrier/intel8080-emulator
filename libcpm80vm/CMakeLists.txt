
cmake_minimum_required(VERSION 3.1.3)

# Target is ANSI C or higher, depends on libi8080
add_library(cpm80vm src/vm.c src/strutil.c)
target_include_directories(cpm80vm
	PUBLIC
		$<INSTALL_INTERFACE:include/cpm80vm>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/cpm80vm>
	PRIVATE include/ include/cpm80vm/internal/
)
target_link_libraries(cpm80vm PUBLIC i8080)

#if (BUILD_CPM80_SIMPLE_VM)
#	add_library(simple_vm OBJECT impl/simple_vm_c90_noalloc.c impl/simple_vm_c90_noalloc.h)
#
#	target_link_libraries(simple_vm PRIVATE i8080)
#	# needs libcpm80vm's private includes
#	target_include_directories(simple_vm PRIVATE include include/cpm80vm)
#endif()