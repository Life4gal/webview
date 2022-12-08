#pragma once

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
		using callback_type = typename descriptor_type::callback_type;

		// Initialize web view window
		auto initialize(
				size_type window_width,
				size_type window_height,
				string_type&& window_title,
				bool is_window_resizable,
				bool is_fullscreen,
				bool is_dev_tools_enabled = false,
				// If it is a temporary environment, the settings are placed in the temporary folder. Otherwise, they are placed in %APPDATA%
				bool is_temp_environment = true
				) -> bool
		{
			return static_cast<impl_type&>(*this).do_initialize(
					window_width,
					window_height,
					std::move(window_title),
					is_window_resizable,
					is_fullscreen,
					is_dev_tools_enabled,
					is_temp_environment
					);
		}

		// JS callback
		auto register_callback(callback_type callback) -> void { static_cast<impl_type&>(*this).do_register_callback(std::move(callback)); }

		// Set title of window
		auto set_window_title(string_view_type new_title) -> void { static_cast<impl_type&>(*this).do_set_window_title(new_title); }

		// Set title of window
		auto set_window_title(string_type&& new_title) -> void { static_cast<impl_type&>(*this).do_set_window_title(std::move(new_title)); }

		// Set fullscreen
		auto set_window_fullscreen(bool to_fullscreen) -> void { static_cast<impl_type&>(*this).do_set_window_fullscreen(to_fullscreen); }

		// Run web view service
		auto run() -> bool { return static_cast<impl_type&>(*this).do_run(); }

		// Is main loop running
		[[nodiscard]] auto is_running() const -> bool { return static_cast<const impl_type&>(*this).do_is_running(); }

		// Stop main loop
		auto shutdown() -> void { static_cast<impl_type&>(*this).do_shutdown(); }

		// Navigate to URL
		auto redirect_to(string_view_type target_url) -> bool { return static_cast<impl_type&>(*this).do_redirect_to(target_url); }

		// Navigate to URL
		auto redirect_to(std::string_view target_url) -> bool requires (!std::is_same_v<string_view_type, std::string_view>) { return static_cast<impl_type&>(*this).do_redirect_to(target_url); }

		// Eval JS
		auto eval(string_view_type js_code) -> void { static_cast<impl_type&>(*this).do_eval(js_code); }

		// Eval JS
		auto eval(std::string_view js_code) -> void requires (!std::is_same_v<string_view_type, std::string_view>) { static_cast<impl_type&>(*this).do_eval(js_code); }

		// Eval JS before page loads
		auto preload(string_view_type js_code) -> void { static_cast<impl_type&>(*this).do_preload(js_code); }
	};
}// namespace gal::web_view
