#include <webview/basic_webview.hpp>
#include <iostream>
#include <span>

auto main() -> int
{
	std::cout << "Hello webview!"
		<< "\nCompiler Name: " << GAL_WEBVIEW_COMPILER_NAME
		<< "\nCompiler Version: " << GAL_WEBVIEW_COMPILER_VERSION
		<< "\nCTP Version: " << GAL_WEBVIEW_VERSION;
}
