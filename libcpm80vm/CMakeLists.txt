
# Cmake libi8080 first!

option(LINK_I8080_STATIC "Link libcpm80vm against libi8080 statically." OFF)

add_library(cpm80vm src/vm.c)
target_include_directories(cpm80vm
	PUBLIC
		$<INSTALL_INTERFACE:include/cpm80vm>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/cpm80vm>
	PRIVATE include/
)

if(LINK_I8080_STATIC)
	target_link_libraries(cpm80vm STATIC i8080)
else()
	target_link_libraries(cpm80vm i8080)
endif()
