#pragma once

#include <functional>

namespace gal::web_view
{
	template<typename T>
	struct WebViewTypeDescriptor;

	template<typename DerivedImpl>
	class BasicWebView
	{
	public:
		using impl_type = DerivedImpl;
		using descriptor_type = WebViewTypeDescriptor<impl_type>;

		using size_type = typename descriptor_type::size_type;
		using string_type = typename descriptor_type::string_type;
		using string_view_type = typename descriptor_type::string_view_type;
		using callback_type = std::function<auto(impl_type&, string_type&) -> void>;

		auto initialize(
				size_type window_width,
				size_type window_height,
				bool is_window_resizable,
				string_view_type window_title) -> bool { return static_cast<impl_type&>(*this).do_initialize(window_width, window_height, is_window_resizable, window_title); }

		auto register_callback(callback_type callback) -> void { static_cast<impl_type&>(*this).do_register_callback(std::move(callback)); }

		auto set_window_title(string_view_type new_title) -> void { static_cast<impl_type&>(*this).do_set_window_title(new_title); }

		auto set_window_title(string_type&& new_title) -> void { static_cast<impl_type&>(*this).do_set_window_title(std::move(new_title)); }

		[[nodiscard]] auto is_running() const -> bool { return static_cast<impl_type&>(*this).do_is_running(); }

		auto shutdown() -> void { static_cast<impl_type&>(*this).do_shutdown(); }

		auto redirect_to(string_view_type target_url) -> bool { return static_cast<impl_type&>(*this).do_redirect_to(target_url); }

		auto eval(string_view_type js_code) -> void { static_cast<impl_type&>(*this).do_eval(js_code); }
	};
}// namespace gal::web_view
