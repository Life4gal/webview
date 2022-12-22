function(set_compile_property target_name)
	set_target_properties(
			${target_name}
			PROPERTIES
			CXX_STANDARD 20
			CXX_STANDARD_REQUIRED TRUE
	)

	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
		set_target_properties(
				${target_name}
				PROPERTIES
				COMPILE_OPTIONS "/W4;/WX"
		)
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set_target_properties(
				${target_name}
				PROPERTIES
				COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror"
		)
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		if ("${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC")
			# clang-cl
			set_target_properties(
					${target_name}
					PROPERTIES
					COMPILE_OPTIONS "/W4;/WX"
			)
		else ()
			# clang
			set_target_properties(
					${target_name}
					PROPERTIES
					COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Werror"
			)
		endif ()
	else ()
		message(FATAL_ERROR "Unknown Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
	endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
endfunction(set_compile_property)
