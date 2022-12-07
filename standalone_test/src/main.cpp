#include <iostream>
#include <thread>
#include <chrono>
#include <webview/webview.hpp>

#if GAL_WEBVIEW_COMPILER_MSVC
auto __stdcall WinMain(HINSTANCE,
                       HINSTANCE,
                       LPSTR,
                       int) -> int
#else
auto main(int argc, char* argv[]) -> int
#endif
{
	std::cout << "Hello web view!"
			<< "\nCompiler Name: " << GAL_WEBVIEW_COMPILER_NAME
			<< "\nCompiler Version: " << GAL_WEBVIEW_COMPILER_VERSION
			<< "\nCTP Version: " << GAL_WEBVIEW_VERSION;

	gal::web_view::web_view web_view{};

	web_view.initialize(
			800,
			600,
			true,
			"hello world");
}
