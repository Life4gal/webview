#pragma once

#if defined(GAL_WEBVIEW_COMPILER_MSVC)

	#include <webview/impl/web_view_windows.hpp>

#elif defined(GAL_WEBVIEW_COMPILER_GNU)

#include <webview/impl/web_view_linux_v2.hpp>

#else

// todo

#endif

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
