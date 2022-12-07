project(
		WebView-standalone-test
		LANGUAGES CXX
)

file(
		GLOB_RECURSE
		${PROJECT_NAME}_SOURCE
		CONFIGURE_DEPENDS

		src/*.cpp
)

add_executable(
		${PROJECT_NAME}
		
		${${PROJECT_NAME}_SOURCE}
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	# windows & msvc
#	set(CMAKE_WIN32_EXECUTABLE ${WIN32})
	set_target_properties(
			${PROJECT_NAME}
			PROPERTIES
			WIN32_EXECUTABLE ${WIN32}
	)
else()
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

target_include_directories(
		${PROJECT_NAME}
		PUBLIC
		${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(
		${PROJECT_NAME}
		PRIVATE
		gal::WebView
)

set_compile_options_private(${PROJECT_NAME})
turn_off_warning(${PROJECT_NAME})
