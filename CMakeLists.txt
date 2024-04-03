cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)



project(canvas VERSION 0.1.0 LANGUAGES CXX C)


add_library(
	POWRPROF
	SHARED
	main.cpp
	)




target_link_libraries(
	POWRPROF
	${CMAKE_CURRENT_SOURCE_DIR}/lib/libmem.lib)