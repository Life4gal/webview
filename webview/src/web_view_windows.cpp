#ifdef GAL_WEBVIEW_COMPILER_MSVC

#include <shellscalingapi.h>
#include <windows.h>
#include <wrl.h>

#include <bit>
#include <cassert>
#include <filesystem>
#include <memory>
#include <type_traits>
#include <webview/impl/web_view_windows.hpp>

namespace
{
	using web_view_windows = gal::web_view::impl::WebViewWindows;
	using library_type	   = std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&::FreeLibrary)>;
	using raw_string_pointer =
			std::conditional_t<
					std::is_same_v<std::remove_cvref_t<decltype(std::declval<decltype(TEXT(""))>()[0])>, char>,
					LPCSTR,
					LPCWSTR>;

	[[nodiscard]] auto load_library(const raw_string_pointer filename) -> library_type
	{
		return library_type{
				LoadLibrary(filename),
				&::FreeLibrary};
	}

	auto set_dpi_awareness() -> void
	{
		// Windows 10
		const auto user32_library = load_library(TEXT("User32.dll"));
		if (auto* dpi_ac = reinterpret_cast<decltype(SetProcessDpiAwarenessContext)*>(GetProcAddress(user32_library.get(), "SetProcessDpiAwarenessContext")))// NOLINT(clang-diagnostic-cast-function-type)
		{
			dpi_ac(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
			return;
		}

		// Windows 8.1
		const auto sh_core_library = load_library(TEXT("ShCore.dll"));
		if (auto* dpi_a = reinterpret_cast<decltype(SetProcessDpiAwareness)*>(GetProcAddress(sh_core_library.get(), "SetProcessDpiAwareness")))// NOLINT(clang-diagnostic-cast-function-type)
		{
			dpi_a(PROCESS_PER_MONITOR_DPI_AWARE);
			return;
		}

		// Windows Vista
		SetProcessDPIAware();// Equivalent to DPI_AWARENESS_CONTEXT_SYSTEM_AWARE
	}

	auto get_current_dpi(const HWND window) -> web_view_windows::dpi_type
	{
		// Windows 10
		const auto user32_library = load_library(TEXT("User32.dll"));
		if (auto* dpi_fw = reinterpret_cast<decltype(GetDpiForWindow)*>(GetProcAddress(user32_library.get(), "GetDpiForWindow")))// NOLINT(clang-diagnostic-cast-function-type)
		{
			return static_cast<web_view_windows::dpi_type>(dpi_fw(window));
		}

		// Windows 8.1
		const auto sh_core_library = load_library(TEXT("ShCore.dll"));
		if (auto* dpi_fm = reinterpret_cast<decltype(GetDpiForMonitor)*>(GetProcAddress(sh_core_library.get(), "GetDpiForMonitor")))// NOLINT(clang-diagnostic-cast-function-type)
		{
			UINT dpi;
			if (const auto monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
				SUCCEEDED(dpi_fm(monitor, MDT_EFFECTIVE_DPI, &dpi, nullptr))) { return static_cast<web_view_windows::dpi_type>(dpi); }
		}

		// Windows 2000
		if (const auto hdc = GetDC(window))
		{
			const auto dpi = GetDeviceCaps(hdc, LOGPIXELSX);
			ReleaseDC(window, hdc);
			return dpi;
		}

		return USER_DEFAULT_SCREEN_DPI;
	}

	auto CALLBACK WndProcedure(
			const HWND	 window,
			const UINT	 msg,
			const WPARAM w_param,
			const LPARAM l_param) -> LRESULT
	{
		auto* web_view = reinterpret_cast<web_view_windows*>(GetWindowLongPtr(window, GWLP_USERDATA));// NOLINT(performance-no-int-to-ptr)

		switch (msg)
		{
			case WM_SIZE:
			{
				if (web_view && web_view->is_ready()) { web_view->resize(); }
				return DefWindowProc(window, msg, w_param, l_param);
			}
			case WM_DPICHANGED:
			{
				if (web_view)
				{
					const auto	current_dpi	 = HIWORD(w_param);
					const auto& current_rect = *reinterpret_cast<RECT*>(l_param);// NOLINT(performance-no-int-to-ptr)
					web_view->set_dpi(static_cast<web_view_windows::dpi_type>(current_dpi), current_rect);
				}
				break;
			}
			case WM_DESTROY:
			{
				if (web_view) { web_view->shutdown(); }
				break;
			}
			default:
			{
				return DefWindowProc(window, msg, w_param, l_param);
			}
		}

		return 0;
	}
}// namespace

namespace gal::web_view::impl
{
	String::String(const char* pointer, const impl_type::size_type length)
		: string_{std::filesystem::path{pointer, pointer + length}} {}

	String::String(const char* pointer)
		: string_{std::filesystem::path{pointer}} {}

	auto WebViewWindows::do_initialize(
			const size_type window_width,
			const size_type window_height,
			string_type&&	window_title,
			const bool		is_window_resizable,
			const bool		is_fullscreen,
			const bool		is_dev_tools_enabled,
			const bool		is_temp_environment) -> bool
	{
		is_dev_tools_enabled_  = is_dev_tools_enabled;
		current_is_fullscreen_ = is_fullscreen;
		is_temp_environment_   = is_temp_environment;

		return initialize_win32_windows(
				window_width,
				window_height,
				std::move(window_title),
				is_window_resizable);
	}

	auto WebViewWindows::do_register_callback(callback_type callback) -> void { callback_.swap(callback); }

	auto WebViewWindows::do_set_window_title(string_type&& new_title) -> void { window_title_ = std::move(new_title); }

	auto WebViewWindows::do_set_window_title(const string_view_type new_title) -> void { do_set_window_title(string_type{new_title}); }

	auto WebViewWindows::do_set_window_fullscreen(const bool to_fullscreen) -> void
	{
		if (current_is_fullscreen_ == to_fullscreen) { return; }

		current_is_fullscreen_ = to_fullscreen;

		if (current_is_fullscreen_)
		{
			// Store window style before going fullscreen
			window_info_.style	  = GetWindowLongPtr(window_, GWL_STYLE);
			window_info_.ex_style = GetWindowLongPtr(window_, GWL_EXSTYLE);
			RECT rect;
			GetWindowRect(window_, &rect);
			window_info_.write_rect(rect);

			// Set new window style
			SetWindowLongPtr(
					window_,
					GWL_STYLE,
					window_info_.style & ~(WS_CAPTION | WS_THICKFRAME));
			SetWindowLongPtr(
					window_,
					GWL_EXSTYLE,
					window_info_.ex_style & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

			// Get monitor size
			MONITORINFO monitor_info;
			monitor_info.cbSize = sizeof(monitor_info);
			GetMonitorInfo(MonitorFromWindow(window_, MONITOR_DEFAULTTONEAREST), &monitor_info);

			// Set window size to monitor size
			SetWindowPos(
					window_,
					nullptr,
					monitor_info.rcMonitor.left,
					monitor_info.rcMonitor.top,
					monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
					monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
					SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		}
		else
		{
			// Restore window style
			SetWindowLongPtr(window_, GWL_STYLE, window_info_.style);
			SetWindowLongPtr(window_, GWL_EXSTYLE, window_info_.ex_style);

			// Restore window size
			const auto [left, top, right, bottom] = std::bit_cast<RECT>(window_info_.rect);
			SetWindowPos(
					window_,
					nullptr,
					left,
					top,
					right - left,
					bottom - top,
					SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		}
	}

	auto WebViewWindows::do_run() -> bool
	{
		ShowWindow(window_, SW_SHOWDEFAULT);
		UpdateWindow(window_);
		SetFocus(window_);

		// Set to single-thread
		if (FAILED(CoInitializeEx(
					nullptr,
					COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
		{
			// todo
			return false;
		}

		auto on_web_message_received =
				[this](
						[[maybe_unused]] ICoreWebView2*			  web_view,
						ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
		{
			if (callback_)
			{
				LPWSTR message;
				if (const auto result = args->TryGetWebMessageAsString(&message);
					FAILED(result))
				{
					// todo
					return result;
				}

				callback_(*this, std::filesystem::path{message}.string());

				CoTaskMemFree(message);
			}
			return S_OK;
		};

		auto on_web_view_controller_create =
				[this, on_web_message_received](
						const HRESULT			 error_code,
						ICoreWebView2Controller* controller) -> HRESULT
		{
			if (FAILED(error_code)) { return error_code; }

			if (controller)
			{
				web_view_controller_ = controller;
				web_view_controller_->get_CoreWebView2(&web_view_window_);
			}

			wil::com_ptr<ICoreWebView2Settings> settings;
			web_view_window_->get_Settings(&settings);
			if (is_dev_tools_enabled_)
			{
				if (FAILED(settings->put_AreDevToolsEnabled(true)))
				{
					// todo
				}
			}

			// Resize WebView
			resize();

			web_view_window_->AddScriptToExecuteOnDocumentCreated(
					preload_js_code_.operator String::impl_type&().data(),
					Microsoft::WRL::Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
							[](const HRESULT created_error_code, const LPCWSTR id) -> HRESULT
							{
								// todo
								if (FAILED(created_error_code)) { (void)id; }
								return S_OK;
							})
							.Get());

			web_view_window_->add_WebMessageReceived(
					Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(on_web_message_received).Get(),
					nullptr);

			// Done initialization, set properties
			is_ready_ = true;

			if (current_is_fullscreen_)
			{
				current_is_fullscreen_ = false;
				set_window_fullscreen(true);
			}

			// navigate to the pending url
			redirect_to(pending_redirect_url_);

			return S_OK;
		};

		auto on_create_environment =
				[this, on_web_view_controller_create](
						const HRESULT			  error_code,
						ICoreWebView2Environment* environment) -> HRESULT
		{
			if (error_code == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
			{
				MessageBox(nullptr, TEXT("Could not find Edge installation!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
				return error_code;
			}

			// Create WebView2 controller
			return environment->CreateCoreWebView2Controller(
					window_,
					Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(on_web_view_controller_create).Get());
		};

		const auto environment_path = [this]
		{
			const auto base_path = [this]
			{
				if (is_temp_environment_) { return std::filesystem::temp_directory_path(); }

				// Get AppData path
				DWORD  app_data_path_length{1 << 15};
				String app_data_path{};
				auto& app_data_path_rep = app_data_path.operator String::impl_type&();
				app_data_path_rep.resize(static_cast<String::size_type>(app_data_path_length));
				app_data_path_length = GetEnvironmentVariable(TEXT("APPDATA"), app_data_path_rep.data(), app_data_path_length);

				if (app_data_path_length != 0) { app_data_path_rep.shrink_to_fit(); }

				std::filesystem::path path{app_data_path_rep};
				assert(std::filesystem::exists(path) && "Invalid app data path!");
				return path;
			}();

			// Get executable file name
			String::value_type exe_name[MAX_PATH];
			GetModuleFileName(nullptr, exe_name, MAX_PATH);

			return base_path / std::filesystem::path{exe_name}.stem();
		}();

		// Create WebView2 environment
		if (FAILED(CreateCoreWebView2EnvironmentWithOptions(
					nullptr,
					environment_path.c_str(),
					nullptr,
					Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(on_create_environment).Get())))
		{
			CoUninitialize();
			return false;
		}

		return true;
	}

	auto WebViewWindows::do_is_running() const -> bool
	{
		(void)this;
		MSG		   msg;
		const auto is_running = GetMessage(&msg, nullptr, 0, 0);
		if (is_running)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return is_running;
	}

	// ReSharper disable once CppMemberFunctionMayBeConst
	auto WebViewWindows::do_shutdown() -> void
	{
		(void)this;
		PostQuitMessage(WM_QUIT);
		CoUninitialize();
	}

	// ReSharper disable once CppMemberFunctionMayBeConst
	auto WebViewWindows::do_redirect_to(const string_view_type target_url) -> bool
	{
		if (!is_ready())
		{
			pending_redirect_url_.operator String::impl_type&().assign(target_url);
			return false;
		}

		if (FAILED(web_view_window_->Navigate(target_url.operator const StringView::impl_type&().data())))
		{
			// todo
			MessageBox(nullptr, TEXT("Navigate failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
			return false;
		}
		return true;
	}

	auto WebViewWindows::do_redirect_to(const std::string_view target_url) -> bool
	{
		const auto real_url = string_type{std::filesystem::path{target_url}};
		return do_redirect_to(real_url);
	}

	// ReSharper disable once CppMemberFunctionMayBeConst
	auto WebViewWindows::do_eval(const string_view_type js_code) -> void
	{
		// Schedule an async task to get the document URL
		web_view_window_->ExecuteScript(
				js_code.operator const StringView::impl_type&().data(),
				Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
						[](const HRESULT error_code, const LPCWSTR result_object_as_json) -> HRESULT
						{
							// todo
							if (FAILED(error_code)) { (void)result_object_as_json; }
							return S_OK;
						})
						.Get());
	}

	auto WebViewWindows::do_eval(const std::string_view js_code) -> void
	{
		const auto real_code = string_type{std::filesystem::path{js_code}};
		return do_eval(real_code);
	}

	auto WebViewWindows::do_preload(const string_view_type js_code) -> void
	{
		// todo: IIFE?
		constexpr string_view_type iife_left{TEXT("(() => {")};
		constexpr string_view_type iife_right{TEXT("})()")};

		preload_js_code_.		   operator String::impl_type&().append(iife_left).append(js_code).append(iife_right);
	}

	auto WebViewWindows::window_info::write_rect(const tagRECT& target_rect) -> void
	{
		// if the size does not match, this will not compile.
		rect = std::bit_cast<fake_rect>(target_rect);
	}

	WebViewWindows::WebViewWindows()
		: is_ready_{false},
		  is_dev_tools_enabled_{false},
		  is_temp_environment_{false},
		  current_is_fullscreen_{false},
		  dpi_{0},
		  window_info_{},
		  window_{nullptr},
		  preload_js_code_{TEXT("window.external." GAL_WEBVIEW_METHOD_NAME "=arg=>window.chrome.webview.postMessage(arg);")} {}

	auto WebViewWindows::initialize_win32_windows(
			const size_type window_width,
			const size_type window_height,
			string_type&&	window_title,
			const bool		is_window_resizable) -> bool
	{
		HMODULE handle;
		if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, nullptr, &handle) == 0 || !handle)
		{
			// todo
			return false;
		}

		window_title_.swap(window_title);

		// create a win32 window
		const WNDCLASSEX window{
				.cbSize		   = sizeof(WNDCLASSEX),
				.style		   = 0,
				.lpfnWndProc   = WndProcedure,
				.cbClsExtra	   = 0,
				.cbWndExtra	   = 0,
				.hInstance	   = handle,
				.hIcon		   = LoadIcon(nullptr, IDI_APPLICATION),
				.hCursor	   = LoadCursor(nullptr, IDC_ARROW),
				.hbrBackground = reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1)),// NOLINT(performance-no-int-to-ptr)
				.lpszMenuName  = nullptr,
				.lpszClassName = TEXT("webview_windows"),
				.hIconSm	   = LoadIcon(nullptr, IDI_APPLICATION)};

		if (!RegisterClassEx(&window))
		{
			MessageBox(nullptr, TEXT("Call to RegisterClassEx failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
			return false;
		}

		// Set default DPI awareness
		set_dpi_awareness();

		window_ = CreateWindow(
				TEXT("webview_windows"),
				window_title_.operator String::impl_type&().c_str(),
				WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				static_cast<int>(window_width),
				static_cast<int>(window_height),
				nullptr,
				nullptr,
				handle,
				nullptr);

		if (!window_)
		{
			MessageBox(nullptr, TEXT("Window Registration Failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
			return false;
		}

		// Scale window based on DPI
		dpi_ = get_current_dpi(window_);

		RECT rect;
		GetWindowRect(window_, &rect);
		SetWindowPos(
				window_,
				nullptr,
				rect.left,
				rect.top,
				MulDiv(static_cast<int>(window_width), static_cast<int>(dpi_), USER_DEFAULT_SCREEN_DPI),
				MulDiv(static_cast<int>(window_height), static_cast<int>(dpi_), USER_DEFAULT_SCREEN_DPI),
				SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

		if (!is_window_resizable)
		{
			auto style = GetWindowLongPtr(window_, GWL_STYLE);
			style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
			SetWindowLongPtr(window_, GWL_STYLE, style);
		}

		// Used with GetWindowLongPtr in WndProcedure
		SetWindowLongPtr(window_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

		return true;
	}

	// ReSharper disable once CppMemberFunctionMayBeConst
	auto WebViewWindows::resize() -> void
	{
		RECT rect;
		GetClientRect(window_, &rect);
		web_view_controller_->put_Bounds(rect);
	}

	auto WebViewWindows::set_dpi(dpi_type new_dpi, const tagRECT& rect) -> void
	{
		const auto old_dpi = std::exchange(dpi_, new_dpi);

		if (current_is_fullscreen_)
		{
			// Scale the saved window size by the change in DPI
			auto	   saved_rect = std::bit_cast<RECT>(window_info_.rect);
			const auto old_width  = saved_rect.right - saved_rect.left;
			const auto old_height = saved_rect.bottom - saved_rect.top;
			saved_rect.right	  = saved_rect.left + MulDiv(static_cast<int>(old_width), static_cast<int>(new_dpi), static_cast<int>(old_dpi));
			saved_rect.bottom	  = saved_rect.top + MulDiv(static_cast<int>(old_height), static_cast<int>(new_dpi), static_cast<int>(old_dpi));

			window_info_.write_rect(saved_rect);
		}
		else
		{
			SetWindowPos(
					window_,
					nullptr,
					rect.left,
					rect.top,
					rect.right - rect.left,
					rect.bottom - rect.top,
					SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		}
	}
}// namespace gal::web_view::impl

#endif
