#pragma once

#include <WebView2.h>
#include <wil/com.h>

#include <string>
#include <string_view>
#include <webview/basic_web_view.hpp>

// todo
// windef.h
struct tagRECT;
struct HWND__;

namespace gal::web_view
{
	namespace impl
	{
		class WebViewWindows;
	}

	template<>
	struct WebViewTypeDescriptor<impl::WebViewWindows>
	{
		using size_type = std::size_t;

		#if UNICODE || _UNICODE
		using char_type = wchar_t;
		#else
		using char_type = char;
		#endif
		using string_type = std::basic_string<char_type>;
		using string_view_type = std::basic_string_view<char_type>;
	};

	namespace impl
	{
		class WebViewWindows final : public BasicWebView<WebViewWindows>
		{
			friend BasicWebView<WebViewWindows>;

			bool do_initialize(
					size_type window_width,
					size_type window_height,
					bool is_window_resizable,
					string_view_type window_title);

			auto do_register_callback(callback_type callback) -> void;

			auto do_set_window_title(string_view_type new_title) -> void;

			auto do_set_window_title(string_type&& new_title) -> void;

			[[nodiscard]] auto do_is_running() const -> bool;

			auto do_shutdown() -> void;

			auto do_redirect_to(string_view_type target_url) -> bool;

			auto do_eval(string_view_type js_code) -> void;

		public:
			using dpi_type = std::uint32_t;
			using web_view_controller_type = wil::com_ptr<ICoreWebView2Controller>;
			using web_view_window_type = wil::com_ptr<ICoreWebView2>;

			struct window_info
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

				void write_rect(const tagRECT& target_rect);
			};

		private:
			bool is_ready_;

			bool is_fullscreen_;
			dpi_type dpi_;
			window_info window_info_;

			HWND__* window_;
			string_type window_title_;

			web_view_controller_type web_view_controller_;
			web_view_window_type web_view_window_;

		public:
			[[nodiscard]] auto is_ready() const noexcept -> bool { return is_ready_; }

			auto resize() -> void;

			auto set_dpi(dpi_type new_dpi, const tagRECT& rect) -> void;
		};
	}// namespace impl
}    // namespace gal::web_view
