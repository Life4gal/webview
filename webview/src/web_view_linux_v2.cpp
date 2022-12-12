#ifdef GAL_WEBVIEW_COMPILER_GNU

#include <webview/impl/web_view_linux_v2.hpp>

#include <gtk-3.0/gtk/gtk.h>
#include <webkitgtk-4.0/webkit2/webkit2.h>
#include <cassert>

namespace gal::web_view::impl
{
	WebViewLinux::WebViewLinux(construct_args_type&& args)
		: window_width_{args.window_width},
		  window_height_{args.window_height},
		  window_title_{std::move(args.window_title)},
		  window_is_fixed_{args.window_is_fixed},
		  window_is_fullscreen_{args.window_is_fullscreen},
		  web_view_use_dev_tools_{args.web_view_use_dev_tools},
		  current_javascript_runnable_{false},
		  service_state_{ServiceStateResult::UNINITIALIZED},
		  current_javascript_running_{false},
		  current_url_{args.index_url.value_or(url_type{construct_args_type::default_url})},
		  inject_javascript_code_{"window.external={" GAL_WEBVIEW_METHOD_NAME ":arg=>window.webkit.messageHandlers.external.postMessage(arg)};"},
		  gtk_window_{nullptr},
		  gtk_web_view_{nullptr} { }

	auto WebViewLinux::do_set_window_title(string_type&& title) noexcept -> void
	{
		if (service_state() != ServiceStateResult::RUNNING) { window_title_.swap(title); }
		else { gtk_window_set_title(GTK_WINDOW(gtk_window_), title.data()); }
	}

	auto WebViewLinux::do_set_window_title(const string_view_type title) -> void
	{
		if (service_state() != ServiceStateResult::RUNNING) { window_title_ = title; }
		else { gtk_window_set_title(GTK_WINDOW(gtk_window_), title.data()); }
	}

	auto WebViewLinux::do_set_window_fullscreen(const bool to_fullscreen) -> set_window_fullscreen_result_type
	{
		window_is_fullscreen_ = to_fullscreen;

		if (window_is_fullscreen_) { gtk_window_fullscreen(GTK_WINDOW(gtk_window_)); }
		else { gtk_window_unfullscreen(GTK_WINDOW(gtk_window_)); }
	}

	auto WebViewLinux::do_service_start() -> service_start_result_type
	{
		if (gtk_init_check(nullptr, nullptr) == FALSE) { return ServiceStartResult::INITIALIZE_FAILED; }

		service_state_ = ServiceStateResult::INITIALIZING;

		// Initialize GTK window
		gtk_window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);

		if (window_is_fixed_) { gtk_widget_set_size_request(gtk_window_, static_cast<gint>(window_width_), static_cast<gint>(window_height_)); }
		else { gtk_window_set_default_size(GTK_WINDOW(gtk_window_), static_cast<gint>(window_width_), static_cast<gint>(window_height_)); }

		gtk_window_set_resizable(GTK_WINDOW(gtk_window_), !window_is_fixed_);
		gtk_window_set_position(GTK_WINDOW(gtk_window_), GTK_WIN_POS_CENTER);

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

	auto WebViewLinux::do_navigate(url_type&& target_url) -> navigate_result_type { return do_navigate(string_view_type{target_url}); }

	auto WebViewLinux::do_navigate(const url_view_type target_url) -> navigate_result_type
	{
		if (service_state() != ServiceStateResult::RUNNING)
		{
			current_url_ = target_url;
			return NavigateResult::SERVICE_NOT_READY_YET;
		}

		webkit_web_view_load_uri(WEBKIT_WEB_VIEW(gtk_web_view_), target_url.data());
		return NavigateResult::SUCCESS;
	}

	auto WebViewLinux::do_inject(javascript_code_type&& inject_javascript_code) -> inject_javascript_result_type { do_inject(javascript_code_view_type{inject_javascript_code}); }

	auto WebViewLinux::do_inject(const javascript_code_view_type inject_javascript_code) -> inject_javascript_result_type
	{
		// todo: IIFE?
		constexpr string_view_type iife_left{"(() => {"};
		constexpr string_view_type iife_right{"})()"};

		inject_javascript_code_.append(iife_left).append(inject_javascript_code).append(iife_right);
	}

	auto WebViewLinux::do_eval(javascript_code_type&& javascript_code) -> javascript_execution_result_type { return do_eval(javascript_code_view_type{javascript_code}); }

	auto WebViewLinux::do_eval(const javascript_code_view_type javascript_code) -> javascript_execution_result_type
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
		return true;
	}

	auto WebViewLinux::do_iteration() const -> service_iteration_result_type
	{
		gtk_main_iteration_do(true);
		return service_state_ != ServiceStateResult::SHUTDOWN;
	}

	auto WebViewLinux::do_shutdown() -> service_shutdown_result_type { service_state_ = ServiceStateResult::SHUTDOWN; }

}

#endif
