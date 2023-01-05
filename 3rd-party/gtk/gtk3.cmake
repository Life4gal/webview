find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

target_include_directories(
		${PROJECT_NAME}
		SYSTEM
		PRIVATE
		${GTK3_INCLUDE_DIRS}
)
target_link_directories(
		${PROJECT_NAME}
		PRIVATE
		${GTK3_LIBRARY_DIRS}
)
target_link_libraries(
		${PROJECT_NAME}
		SYSTEM
		PRIVATE
		${GTK3_LIBRARIES}
)

set(${PROJECT_NAME_PREFIX}3RD_PARTY_DEPENDENCIES ${${PROJECT_NAME_PREFIX}3RD_PARTY_DEPENDENCIES} "gtk+-3.0 ${GTK3_VERSION}" PARENT_SCOPE)
