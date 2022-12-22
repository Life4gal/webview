project(
		WebView-unit-test
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

target_compile_definitions(
		${PROJECT_NAME}
		PUBLIC

		BOOST_UT_DISABLE_MODULE
)

CPM_link_libraries_DECL()
include(${${PROJECT_NAME_PREFIX}CMAKE_3RDPARTY_PATH}/boost-ext-ut.cmake)
CPM_link_libraries_LINK(${PROJECT_NAME})

target_link_libraries(
		${PROJECT_NAME}
		PRIVATE
		gal::WebView
)

set_compile_property(${PROJECT_NAME})
turn_off_warning(${PROJECT_NAME})

#add_test(
#		NAME ${PROJECT_NAME}
#		COMMAND ${PROJECT_NAME}
#)
