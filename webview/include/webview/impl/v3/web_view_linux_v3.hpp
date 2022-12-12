#pragma once

#ifdef GAL_WEBVIEW_COMPILER_GNU

#include <webview/impl/v3/web_view_base.hpp>

// #include <gtk-3.0/gtk/gtkwidget.h>
// gtktypes.h
struct _GtkWidget;

namespace gal::web_view::impl
{
	inline namespace v3
	{
		class WebViewLinux final : public WebViewBase<WebViewLinux>
		{
			friend WebViewBase;

		public:
			using native_window_type = _GtkWidget*;

		private:
			bool current_javascript_runnable_;
			bool current_javascript_running_;

			native_window_type gtk_window_;
			native_window_type gtk_web_view_;

		public:
			// using WebViewBase::WebViewBase;

			WebViewLinux(
					window_size_type window_width,
					window_size_type window_height,
					string_type&&    window_title,
					bool             window_is_fixed        = false,
					bool             window_is_fullscreen   = false,
					bool             web_view_use_dev_tools = false,
					string_type&&    index_url              = string_type{default_index_url});

		private:
			auto do_set_window_title(string_view_type title) const -> void;

			auto do_set_window_fullscreen(bool to_fullscreen) const -> void;

			[[nodiscard]] auto do_navigate(string_view_type target_url) const -> NavigateResult;

			auto do_eval(string_view_type javascript_code) -> void;

			auto do_service_start() -> ServiceStartResult;

			auto do_iteration() const -> bool;

			auto do_shutdown() -> void;
		};
	}
}

#endif
