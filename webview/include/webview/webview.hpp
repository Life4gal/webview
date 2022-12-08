#pragma once

#ifdef GAL_WEBVIEW_COMPILER_MSVC

	#include <webview/impl/web_view_windows.hpp>

#else

// todo

#endif

namespace gal::web_view
{
#ifdef GAL_WEBVIEW_COMPILER_MSVC

	using web_view = impl::WebViewWindows;

#else

	// todo

#endif
}// namespace gal::web_view
