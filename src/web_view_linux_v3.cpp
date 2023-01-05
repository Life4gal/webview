#if defined(GAL_WEBVIEW_PLATFORM_LINUX)

#include <webview/impl/v3/web_view_linux_v3.hpp>

#include <gtk-3.0/gtk/gtk.h>
#include <webkitgtk-4.0/webkit2/webkit2.h>
#include <cassert>

namespace gal::web_view::impl
{
	inline namespace v3
	{
		WebViewLinux::WebViewLinux(
				const window_size_type window_width,
				const window_size_type window_height,
				string_type&&          window_title,
				const bool             window_is_fixed,
				const bool             window_is_fullscreen,
				const bool             web_view_use_dev_tools,
				string_type&&          index_url)
			: WebViewBase{
					  window_width,
					  window_height,
					  std::move(window_title),
					  window_is_fixed,
					  window_is_fullscreen,
					  web_view_use_dev_tools,
					  std::move(index_url)},
			  current_javascript_runnable_{false},
			  current_javascript_running_{false},
			  gtk_window_{nullptr},
			  gtk_web_view_{nullptr}
		{
			inject_javascript_code_ = "window.external={" GAL_WEBVIEW_METHOD_NAME ":arg=>window.webkit.messageHandlers.external.postMessage(arg)};";

			if (gtk_init_check(nullptr, nullptr) == FALSE) { return; }

			// Initialize GTK window
			gtk_window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);

			if (window_is_fixed_) { gtk_widget_set_size_request(gtk_window_, static_cast<gint>(window_width_), static_cast<gint>(window_height_)); }
			else { gtk_window_set_default_size(GTK_WINDOW(gtk_window_), static_cast<gint>(window_width_), static_cast<gint>(window_height_)); }

			gtk_window_set_resizable(GTK_WINDOW(gtk_window_), !window_is_fixed_);
			gtk_window_set_position(GTK_WINDOW(gtk_window_), GTK_WIN_POS_CENTER);

			service_state_ = ServiceStateResult::INITIALIZED;
		}

		auto WebViewLinux::do_set_window_title(const string_view_type title) const -> void { gtk_window_set_title(GTK_WINDOW(gtk_window_), title.data()); }

		auto WebViewLinux::do_set_window_fullscreen(const bool to_fullscreen) const -> void
		{
			if (to_fullscreen) { gtk_window_fullscreen(GTK_WINDOW(gtk_window_)); }
			else { gtk_window_unfullscreen(GTK_WINDOW(gtk_window_)); }
		}

		auto WebViewLinux::do_navigate(const string_view_type target_url) const -> NavigateResult
		{
			webkit_web_view_load_uri(WEBKIT_WEB_VIEW(gtk_web_view_), target_url.data());
			return NavigateResult::SUCCESS;
		}

		auto WebViewLinux::do_eval(const string_view_type javascript_code) -> void
		{
			while (!current_javascript_runnable_) { g_main_context_iteration(nullptr, TRUE); }

			current_javascript_running_ = true;
			webkit_web_view_run_javascript(
					WEBKIT_WEB_VIEW(gtk_web_view_),
					javascript_code.data(),
					nullptr,
					+[](
					[[maybe_unused]] GObject*      source_object,
					[[maybe_unused]] GAsyncResult* result,
					const gpointer                 arg) -> void
					{
						auto* wv = static_cast<WebViewLinux*>(arg);
						assert(wv && "Invalid web view!");
						wv->current_javascript_running_ = false;
					},
					this);

			while (current_javascript_running_) { g_main_context_iteration(nullptr, TRUE); }
		}

		auto WebViewLinux::do_service_start() -> ServiceStartResult
		{
			assert(service_state_ == ServiceStateResult::INITIALIZED && "Initialize service first!");

			// Add scrolling container
			auto* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
			gtk_container_add(GTK_CONTAINER(gtk_window_), scrolled);

			// Content manager
			auto* content_manager = webkit_user_content_manager_new();
			webkit_user_content_manager_register_script_message_handler(content_manager, "external");
			g_signal_connect(
					content_manager,
					"script-message-received::external",
					G_CALLBACK(
						+[](
							[[maybe_unused]] WebKitUserContentManager* webkit_cm,
							WebKitJavascriptResult* result,
							const gpointer arg) -> void
						{
						auto* wv = static_cast<WebViewLinux*>(arg);
						assert(wv && "Invalid web view!");
						if (wv->current_callback_)
						{
						auto* js_value = webkit_javascript_result_get_js_value(result);
						wv->current_callback_(*wv, string_type{jsc_value_to_string(js_value)});
						}
						}),
					this);

			// web view
			gtk_web_view_ = webkit_web_view_new_with_user_content_manager(content_manager);
			g_signal_connect(
					G_OBJECT(gtk_web_view_),
					"load-changed",
					G_CALLBACK(
						+[](
							[[maybe_unused]] WebKitWebView* webkit_wv,
							const WebKitLoadEvent event,
							const gpointer arg) -> void
						{
						if (event == WEBKIT_LOAD_FINISHED)
						{
						auto* wv = static_cast<WebViewLinux*>(arg);
						assert(wv && "Invalid web view!");
						wv->current_javascript_runnable_ = true;
						}
						}),
					this);
			gtk_container_add(GTK_CONTAINER(scrolled), gtk_web_view_);

			g_signal_connect(
					G_OBJECT(gtk_window_),
					"destroy",
					G_CALLBACK(
						+[](
							[[maybe_unused]] GtkWidget* window,
							const gpointer arg) -> void
						{
						auto* wv = static_cast<WebViewLinux*>(arg);
						assert(wv && "Invalid web view!");
						wv->shutdown();
						}),
					this);

			// dev tools
			if (web_view_use_dev_tools_)
			{
				auto* settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(gtk_web_view_));
				webkit_settings_set_enable_write_console_messages_to_stdout(settings, true);
				webkit_settings_set_enable_developer_extras(settings, true);
			}
			else
			{
				g_signal_connect(
						G_OBJECT(gtk_web_view_),
						"context-menu",
						G_CALLBACK(
							+[](
								[[maybe_unused]] WebKitWebView* webkit_wv,
								[[maybe_unused]] GtkWidget* widget,
								[[maybe_unused]] WebKitHitTestResult* result,
								[[maybe_unused]] gboolean b,
								[[maybe_unused]] gpointer arg) -> gboolean
							{
							// ignore it
							return TRUE;
							}),
						nullptr);
			}

			webkit_user_content_manager_add_script(
					content_manager,
					webkit_user_script_new(
							inject_javascript_code_.c_str(),
							WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
							WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
							nullptr,
							nullptr));

			// Monitor for fullscreen changes
			g_signal_connect(
					G_OBJECT(gtk_web_view_),
					"enter-fullscreen",
					G_CALLBACK(
						+[](
							[[maybe_unused]] WebKitWebView* webkit_wv,
							const gpointer arg) -> gboolean
						{
						auto* wv = static_cast<const WebViewLinux*>(arg);
						assert(wv && "Invalid web view!");
						if (wv->window_is_fixed_)
						{
						return FALSE;
						}
						return TRUE;
						}),
					this);
			g_signal_connect(
					G_OBJECT(gtk_web_view_),
					"leave-fullscreen",
					G_CALLBACK(
						+[](
							[[maybe_unused]] WebKitWebView* webkit_wv,
							const gpointer arg) -> gboolean
						{
						auto* wv = static_cast<const WebViewLinux*>(arg);
						assert(wv && "Invalid web view!");
						if (wv->window_is_fixed_)
						{
						return FALSE;
						}
						return TRUE;
						}),
					this);

			// Done initialization
			service_state_ = ServiceStateResult::RUNNING;

			set_window_title(window_title_);
			set_window_fullscreen(window_is_fullscreen_);
			// navigate to the url
			navigate(current_url_);

			// show
			gtk_widget_grab_focus(gtk_web_view_);
			gtk_widget_show_all(gtk_window_);

			return ServiceStartResult::SUCCESS;
		}

		auto WebViewLinux::do_iteration() const -> bool
		{
			gtk_main_iteration_do(true);
			return service_state_ != ServiceStateResult::SHUTDOWN;
		}

		auto WebViewLinux::do_shutdown() -> void { service_state_ = ServiceStateResult::SHUTDOWN; }
	}
}

#endif
