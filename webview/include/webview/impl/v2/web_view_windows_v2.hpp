#pragma once

#ifdef GAL_WEBVIEW_COMPILER_MSVC

#ifndef UNICODE
		#warning "Currently only supports unicode on Windows platform"
		#define UNICODE
#endif

#include <webview/impl/v2/web_view_base.hpp>
#include <wil/com.h>

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
				: String{string.c_str(), string.size()} { }

			template<typename CharTraits>
			explicit(false) String(const std::basic_string_view<char, CharTraits>& string_view)
				: String{string_view.data(), string_view.size()} { }

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
				: string_view_{std::forward<Arg>(arg)} { }

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
	struct WebViewTypeDescriptor<impl::WebViewWindows> : public WebViewTypeDescriptor<void>, public std::true_type
	{
		using string_type = impl::String;
		using string_view_type = impl::StringView;

		// todo: wstring or string?
		template<typename ImplType>
		using javascript_callback_type = std::function<auto(ImplType&, string_type::impl_type&&) -> void>;

		using url_type = string_type;
		using url_view_type = string_view_type;
		using javascript_code_type = string_type;
		using javascript_code_view_type = string_view_type;
	};

	struct WebViewConstructArgsWindows : public WebViewConstructArgs<WebViewTypeDescriptor<impl::WebViewWindows>>
	{
		bool is_temp_environment;
	};

	namespace impl
	{
		class WebViewWindows final : public WebViewBase<WebViewWindows>
		{
			friend WebViewBase;

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
			bool is_temp_environment_;

			dpi_type    dpi_;
			window_info window_info_;

			HWND__* window_;

			web_view_controller_type web_view_controller_;
			web_view_window_type     web_view_window_;

		public:
			explicit WebViewWindows(WebViewConstructArgsWindows&& args);

		private:
			auto do_set_window_title(string_view_type title) -> void;

			auto do_set_window_fullscreen(bool to_fullscreen) -> set_window_fullscreen_result_type;

			auto do_navigate(url_view_type target_url) -> navigate_result_type;

			[[nodiscard]] auto do_eval(javascript_code_view_type javascript_code) const -> javascript_execution_result_type;

			auto do_service_start() -> service_start_result_type;

			auto do_iteration() -> service_iteration_result_type;

			auto do_shutdown() -> service_shutdown_result_type;

		public:
			// INTERNAL USE ONLY, DO NOT USE IT!!!
			auto resize() -> void;

			// INTERNAL USE ONLY, DO NOT USE IT!!!
			auto set_dpi(dpi_type new_dpi, const tagRECT& rect) -> void;
		};
	}
}

#endif
