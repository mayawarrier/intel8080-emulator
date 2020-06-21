
# Cmake libi8080 first!
# Target can build as ANSI C, so no need to specify compiler version.

option(libcpm80vm_LINK_STATIC_libi8080 "Link libcpm80vm against libi8080 statically." OFF)

add_library(cpm80vm src/cpm80_vm.c src/cpm80_bios.c)
target_include_directories(cpm80vm
	PUBLIC
		$<INSTALL_INTERFACE:include/cpm80vm>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/cpm80vm>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(libcpm80vm_LINK_STATIC_libi8080)
	target_link_libraries(cpm80vm STATIC i8080)
else()
	target_link_libraries(cpm80vm i8080)
endif()
