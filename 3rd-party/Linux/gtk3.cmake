# Use the package PkgConfig to detect GTK+ headers/library files
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# Setup CMake to use GTK+, tell the compiler where to look for headers
# and to the linker where to look for libraries
list(APPEND GAL_WEBVIEW_HEADERS ${GTK3_INCLUDE_DIRS})
list(APPEND GAL_WEBVIEW_LINK_DIRS ${GTK3_LIBRARY_DIRS})

# Add other flags to the compiler
list(APPEND GAL_WEBVIEW_DEFINITIONS ${GTK3_CFLAGS_OTHER})

# Link the target to the GTK+ libraries
list(APPEND GAL_WEBVIEW_LIBS ${GTK3_LIBRARIES})
