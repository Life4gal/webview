#pragma once

#ifdef GAL_WEBVIEW_COMPILER_GNU

#include <webview/impl/web_view_base.hpp>

// #include <gtk-3.0/gtk/gtkwidget.h>
// gtktypes.h
struct _GtkWidget;

namespace gal::web_view
{
	namespace impl
	{
		class WebViewLinux;
	}

	enum class ServiceStartResult : std::uint8_t
	{
		SUCCESS,
		INITIALIZE_FAILED,
	};

	enum class ServiceStateResult : std::uint8_t
	{
		UNINITIALIZED,
		INITIALIZING,
		RUNNING,
		SHUTDOWN,
	};

	enum class NavigateResult :std::uint8_t
	{
		SUCCESS,
		SERVICE_NOT_READY_YET,
	};

	template<>
	struct WebViewTypeDescriptor<impl::WebViewLinux> : public WebViewTypeDescriptor<void>, public std::true_type
	{
		using service_start_result_type = ServiceStartResult;
		using service_state_result_type = ServiceStateResult;
		using navigate_result_type = NavigateResult;
	};

	namespace impl
	{
		class WebViewLinux final : public WebViewBase<WebViewLinux>
		{
			friend WebViewBase;

		public:
			explicit WebViewLinux(construct_args_type&& args);

		private:
			window_size_type window_width_;
			window_size_type window_height_;
			string_type      window_title_;

			bool window_is_fixed_;
			bool window_is_fullscreen_;
			bool web_view_use_dev_tools_;

			bool                      current_javascript_runnable_;
			service_state_result_type service_state_;

			bool                     current_javascript_running_;
			url_type                 current_url_;
			javascript_callback_type current_callback_;
			javascript_code_type     inject_javascript_code_;

			_GtkWidget* gtk_window_;
			_GtkWidget* gtk_web_view_;

			auto do_register_javascript_callback(javascript_callback_type&& callback) noexcept
				-> register_javascript_callback_result_type { current_callback_.swap(callback); }

			auto						 do_set_window_title(string_type&& title) noexcept -> void;

			auto						 do_set_window_title(string_view_type title) -> void;

			auto do_set_window_fullscreen(bool to_fullscreen) -> set_window_fullscreen_result_type;

			auto do_service_start() -> service_start_result_type;

			[[nodiscard]] constexpr auto do_service_state() const
				-> service_state_result_type { return service_state_; }

			auto do_navigate(url_type&& target_url) -> navigate_result_type;

			auto do_navigate(url_view_type target_url) -> navigate_result_type;

			auto do_inject(javascript_code_type&& inject_javascript_code) -> inject_javascript_result_type;

			auto do_inject(javascript_code_view_type inject_javascript_code) -> inject_javascript_result_type;

			auto do_eval(javascript_code_type&& javascript_code) -> javascript_execution_result_type;

			auto do_eval(javascript_code_view_type javascript_code) -> javascript_execution_result_type;

			[[nodiscard]] auto do_iteration() const -> service_iteration_result_type;

			auto do_shutdown() -> service_shutdown_result_type;
		};
	}
}

#endif
