#pragma once

#ifndef UNICODE
	#warning "Currently only supports unicode on Windows platform"
#define UNICODE
#endif

#include <WebView2.h>
#include <wil/com.h>

#include <functional>
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

		class StringView;

		class String
		{
		public:
			using impl_type = std::wstring;

			using value_type = impl_type::value_type;
			using size_type = impl_type::size_type;
			using pointer = impl_type::pointer;
			using const_pointer = impl_type::const_pointer;

		private:
			impl_type string_;

		public:
			template<typename... Args>
				requires std::is_constructible_v<impl_type, Args...> && (sizeof...(Args) != 1)
			constexpr explicit String(Args&&... args) noexcept(std::is_nothrow_constructible_v<impl_type, Args...>)
				: string_{std::forward<Args>(args)...} { }

			template<typename Arg>
				requires std::is_constructible_v<impl_type, Arg>
			constexpr explicit(!std::is_convertible_v<Arg, impl_type>) String(Arg&& arg) noexcept(std::is_nothrow_constructible_v<impl_type, Arg>)
				: string_{std::forward<Arg>(arg)} { }

			constexpr explicit String(const StringView& string_view);

			explicit(false) String(const char* pointer, size_type length);
			explicit(false) String(const char* pointer);

			template<typename CharTraits, typename Allocator>
			explicit(false) String(const std::basic_string<char, CharTraits, Allocator>& string)
				: String{string.c_str(), string.size()} {}

			template<typename CharTraits>
			explicit(false) String(const std::basic_string_view<char, CharTraits>& string_view)
				: String{string_view.data(), string_view.size()} {}

			constexpr explicit(false) operator impl_type&() noexcept { return string_; }

			constexpr explicit(false) operator const impl_type&() const noexcept { return string_; }

			constexpr explicit(false) operator impl_type&&() && noexcept { return std::move(string_); }

			constexpr explicit(false) operator StringView() const noexcept;

			constexpr void swap(String& other) noexcept { string_.swap(other.string_); }
		};

		class StringView
		{
		public:
			using impl_type = std::wstring_view;

			using value_type = impl_type::value_type;
			using size_type = impl_type::size_type;
			using pointer = impl_type::pointer;
			using const_pointer = impl_type::const_pointer;

		private:
			impl_type string_view_;

		public:
			template<typename... Args>
				requires std::is_constructible_v<impl_type, Args...> && (sizeof...(Args) != 1)
			constexpr explicit StringView(Args&&... args) noexcept(std::is_nothrow_constructible_v<impl_type, Args...>)
				: string_view_{std::forward<Args>(args)...} { }

			template<typename Arg>
				requires std::is_constructible_v<impl_type, Arg>
			constexpr explicit(!std::is_convertible_v<Arg, impl_type>) StringView(Arg&& arg) noexcept(std::is_nothrow_constructible_v<impl_type, Arg>)
				: string_view_{std::forward<Arg>(arg)} {}

			// constexpr explicit(false) StringView(const String& string) = delete;// use String.operator StringView()
			// 	: string_view_{string.operator const String::impl_type&()} {}

			// explicit(false) StringView(const char* pointer, size_type length) = delete;
			// explicit(false) StringView(const char* pointer) = delete;

			constexpr explicit(false) operator impl_type&() noexcept { return string_view_; }

			constexpr explicit(false) operator const impl_type&() const noexcept { return string_view_; }
		};

		constexpr String::String(const StringView& string_view)
			: string_{string_view} {}

		constexpr String::operator StringView() const noexcept { return {string_}; }
	}

	template<>
	struct WebViewTypeDescriptor<impl::WebViewWindows>
	{
		using size_type = std::size_t;

		using string_type = impl::String;
		using string_view_type = impl::StringView;
		// arg type is std::string
		using callback_type = std::function<auto(impl::WebViewWindows&, std::string&&) -> void>;
	};

	namespace impl
	{
		class WebViewWindows final : public BasicWebView<WebViewWindows>
		{
			friend BasicWebView<WebViewWindows>;

			auto do_initialize(
					size_type window_width,
					size_type window_height,
					string_type&& window_title,
					bool is_window_resizable,
					bool is_fullscreen,
					bool is_dev_tools_enabled,
					bool is_temp_environment
					) -> bool;

			auto do_register_callback(callback_type callback) -> void;

			auto do_set_window_title(string_view_type new_title) -> void;

			auto do_set_window_title(string_type&& new_title) -> void;

			auto do_set_window_fullscreen(bool to_fullscreen) -> void;

			auto do_run() -> bool;

			[[nodiscard]] auto do_is_running() const -> bool;

			auto do_shutdown() -> void;

			auto do_redirect_to(string_view_type target_url) -> bool;

			auto do_redirect_to(std::string_view target_url) -> bool;

			auto do_eval(string_view_type js_code) -> void;

			auto do_eval(std::string_view js_code) -> void;

			auto do_preload(string_view_type js_code) -> void;

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

			WebViewWindows();

		private:
			bool is_ready_;
			bool is_dev_tools_enabled_;
			bool is_temp_environment_;

			bool current_is_fullscreen_;
			dpi_type dpi_;
			window_info window_info_;

			HWND__* window_;
			string_type window_title_;
			string_type preload_js_code_;
			string_type pending_redirect_url_;
			callback_type callback_;

			web_view_controller_type web_view_controller_;
			web_view_window_type web_view_window_;

			auto initialize_win32_windows(
					size_type window_width,
					size_type window_height,
					string_type&& window_title,
					bool is_window_resizable
					) -> bool;

		public:
			[[nodiscard]] auto is_ready() const noexcept -> bool { return is_ready_; }

			auto resize() -> void;

			auto set_dpi(dpi_type new_dpi, const tagRECT& rect) -> void;
		};
	}// namespace impl
}    // namespace gal::web_view
