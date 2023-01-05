find_package(PkgConfig REQUIRED)
pkg_check_modules(WebKit2GTK REQUIRED webkit2gtk-4.0)

target_include_directories(
		${PROJECT_NAME}
		SYSTEM
		PRIVATE
		${WebKit2GTK_INCLUDE_DIRS}
)
target_link_directories(
		${PROJECT_NAME}
		SYSTEM
		PRIVATE
		${WebKit2GTK_LIBRARY_DIRS}
)
target_link_libraries(
		${PROJECT_NAME}
		PRIVATE
		${WebKit2GTK_LIBRARIES}
)

set(${PROJECT_NAME_PREFIX}3RD_PARTY_DEPENDENCIES ${${PROJECT_NAME_PREFIX}3RD_PARTY_DEPENDENCIES} "webkit2gtk-4.0 ${WebKit2GTK_VERSION}" PARENT_SCOPE)
