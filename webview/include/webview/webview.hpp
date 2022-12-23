#pragma once

#include <webview/impl/v3/web_view_linux_v3.hpp>
#include <webview/impl/v3/web_view_windows_v3.hpp>

namespace gal::web_view
{
	#if defined(GAL_WEBVIEW_PLATFORM_WINDOWS)

	using WebView = impl::WebViewWindows;

	#elif defined(GAL_WEBVIEW_PLATFORM_LINUX)

	using WebView = impl::WebViewLinux;

	#else

	#error "Unknown Platform!"

	#endif
}// namespace gal::web_view
