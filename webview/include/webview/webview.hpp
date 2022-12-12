#pragma once

#include <webview/impl/v3/web_view_linux_v3.hpp>
#include <webview/impl/v3/web_view_windows_v3.hpp>

namespace gal::web_view
{
	#if defined(GAL_WEBVIEW_COMPILER_MSVC)

	using web_view = impl::WebViewWindows;

	#elif defined(GAL_WEBVIEW_COMPILER_GNU)

	using web_view = impl::WebViewLinux;

	#else

	// todo

	#endif
}// namespace gal::web_view
