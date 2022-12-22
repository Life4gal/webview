project(
		WebView-standalone-test
		LANGUAGES CXX
)

function(setup_project project_name html_path)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		# windows & msvc
		# set(CMAKE_WIN32_EXECUTABLE ${WIN32})
		set_target_properties(
				${project_name}
				PROPERTIES
				WIN32_EXECUTABLE ${WIN32}
		)
	else()
	endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

	target_link_libraries(
		${project_name}
		PRIVATE
		gal::WebView
	)

	#add_custom_command(
	#	TARGET ${project_name} POST_BUILD
	#	COMMAND ${CMAKE_COMMAND} -E copy
	#	${html_path}
	#	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
	#)

	set_compile_property(${project_name})
	turn_off_warning(${project_name})
endfunction(setup_project)

add_executable(
	${PROJECT_NAME}-callback
	callback/main.cpp
)
target_compile_definitions(
	${PROJECT_NAME}-callback 
	PUBLIC 
	CALLBACK_TEST_HTML_PATH="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/callback/callback.html"
	#$<$<CXX_COMPILER_ID:MSVC>:L"file:///${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/callback/callback.html">
	#$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:"file:///${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/callback/callback.html">
)
setup_project(${PROJECT_NAME}-callback ${PROJECT_SOURCE_DIR}/callback/callback.html)

#add_test(
#		NAME ${PROJECT_NAME}-callback
#		COMMAND ${PROJECT_NAME}-callback
#)
