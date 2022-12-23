#pragma once

#if defined(GAL_WEBVIEW_PLATFORM_WINDOWS)

#ifndef UNICODE
		// #warning "Currently only supports unicode on Windows platform"
		#pragma message("Currently only supports unicode on Windows platform")
		#define UNICODE
#endif

#include <webview/impl/v3/web_view_base.hpp>
#include <wil/com.h>
#include <WebView2.h>

// windef.h
struct tagRECT;
struct HWND__;

namespace gal::web_view::impl
{
	inline namespace v3
	{
		class WebViewWindows final : public WebViewBase<WebViewWindows>
		{
			friend WebViewBase;

		public:
			using web_view_controller_type = wil::com_ptr<ICoreWebView2Controller>;
			using web_view_window_type = wil::com_ptr<ICoreWebView2>;

			using dpi_type = std::uint32_t;
			using rect_type = tagRECT;
			using native_window_type = HWND__*;

			struct saved_window_info
			{
				// GetWindowLongPtr(window_, GWL_STYLE)
				LONG_PTR style;
				// GetWindowLongPtr(window_, GWL_EXSTYLE)
				LONG_PTR ex_style;

				// todo: rect
				struct fake_rect
				{
					std::int32_t rect[4];
				};

				fake_rect rect;

				void write_rect(const rect_type& target_rect);
			};

		private:
			bool is_temp_environment_;

			dpi_type          dpi_;
			saved_window_info window_info_;

			native_window_type window_;

			web_view_controller_type web_view_controller_;
			web_view_window_type     web_view_window_;

		public:
			// using WebViewBase::WebViewBase;

			WebViewWindows(
					window_size_type window_width,
					window_size_type window_height,
					string_type&&    window_title,
					bool             window_is_fixed        = false,
					bool             window_is_fullscreen   = false,
					bool             web_view_use_dev_tools = false,
					bool             is_temp_environment    = false,
					string_type&&    index_url              = string_type{default_index_url});

		private:
			auto do_set_window_title(string_view_type title) const -> void;

			auto do_set_window_fullscreen(bool to_fullscreen) -> void;

			auto do_navigate(string_view_type target_url) const -> NavigateResult;

			auto do_eval(string_view_type javascript_code) const -> void;

			auto do_service_start() -> ServiceStartResult;

			auto do_iteration() const -> bool;

			auto do_shutdown() const -> void;

		public:
			// INTERNAL USE ONLY, DO NOT USE IT!!!
			auto resize() const -> void;

			// INTERNAL USE ONLY, DO NOT USE IT!!!
			auto set_dpi(dpi_type new_dpi, const tagRECT& rect) -> void;
		};
	}
}

#endif
