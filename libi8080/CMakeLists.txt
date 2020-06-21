
# Target can build as ANSI C, so no need to specify compiler version.

option(libi8080_NEED_INTR_LOCK "Allow interrupts to be sent from a separate thread. Enables i8080_interrupt() thread synchronization." OFF)

add_library(i8080 src/i8080.c)
target_include_directories(i8080 
	PUBLIC
		$<INSTALL_INTERFACE:include/i8080>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/i8080>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(libi8080_NEED_INTR_LOCK)
    find_package(Threads REQUIRED)
	target_link_libraries(i8080 PUBLIC Threads::Threads)
	target_compile_definitions(i8080 PRIVATE NEED_INTR_LOCK=1)
endif()
