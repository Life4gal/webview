# Use the package PkgConfig to detect webkit2gtk headers/library files
find_package(PkgConfig REQUIRED)
pkg_check_modules(WebKit2GTK REQUIRED webkit2gtk-4.0)

# Setup CMake to use webkit2gtk, tell the compiler where to look for headers
# and to the linker where to look for libraries
list(APPEND GAL_WEBVIEW_HEADERS ${WebKit2GTK_INCLUDE_DIRS})
list(APPEND GAL_WEBVIEW_LINK_DIRS ${WebKit2GTK_LIBRARY_DIRS})

# Add other flags to the compiler
list(APPEND GAL_WEBVIEW_DEFINITIONS ${WebKit2GTK_CFLAGS_OTHER})

# Link the target to the webkit2gtk libraries
list(APPEND GAL_WEBVIEW_LIBS ${WebKit2GTK_LIBRARIES})
