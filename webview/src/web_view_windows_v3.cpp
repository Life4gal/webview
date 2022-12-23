#if defined(GAL_WEBVIEW_COMPILER_MSVC) or defined(GAL_WEBVIEW_COMPILER_CLANG_CL)

#include <webview/impl/v3/web_view_windows_v3.hpp>

#include <bit>
#include <filesystem>
#include <memory>
#include <cassert>
#include <windows.h>
#include <shellscalingapi.h>
#include <wrl.h>

namespace
{
	using web_view_windows = gal::web_view::impl::WebViewWindows;
	using library_type = std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&::FreeLibrary)>;

	using raw_char_type =
	std::conditional_t<
		std::is_same_v<std::remove_cvref_t<decltype(std::declval<decltype(TEXT(""))>()[0])>, char>,
		char,
		wchar_t>;
	using raw_string_pointer =
	std::conditional_t<
		std::is_same_v<raw_char_type, char>,
		LPCSTR,
		LPCWSTR>;

	template<typename StringView>
	[[nodiscard]] auto to_wchar_string(const StringView& string) -> std::basic_string_view<raw_char_type> requires std::is_same_v<typename StringView::value_type, raw_char_type>
	{
		// just return it
		return string;
	}

	namespace detail
	{
		template<typename T, bool>
		struct converter;

		template<typename T>
		struct converter<T, true>
		{
			[[nodiscard]] constexpr auto operator()(const T& s) const // -> std::basic_string<raw_char_type>
			{
				return std::filesystem::path{s}.wstring();
			}
		};

		template<typename T>
		struct converter<T, false>
		{
			[[nodiscard]] constexpr auto operator()(const T& s) const // -> std::basic_string<raw_char_type>
			{
				return std::filesystem::path{s}.string();
			}
		};
	}

	template<typename StringView>
	[[nodiscard]] auto to_wchar_string(const StringView& string) -> std::basic_string<raw_char_type> requires(!std::is_same_v<typename StringView::value_type, raw_char_type>)
	{
		// convert required
		return detail::converter<StringView, std::is_same_v<raw_char_type, wchar_t>>{}(string);

		// if constexpr (std::is_same_v<raw_char_type, wchar_t>) { return std::filesystem::path{string}.wstring(); }
		// else { return std::filesystem::path{string}.string(); }
	}

	template<typename RawString>
	[[nodiscard]] auto from_wchar_string(const RawString& string) -> std::basic_string_view<char> requires std::is_same_v<raw_char_type, char>
	{
		// just return it
		return {string};
	}

	template<typename RawString>
	[[nodiscard]] auto from_wchar_string(const RawString& string) -> std::basic_string<char> requires std::is_same_v<raw_char_type, wchar_t>
	{
		// convert required
		return std::filesystem::path{string}.string();
	}

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
			const HWND   window,
			const UINT   msg,
			const WPARAM w_param,
			const LPARAM l_param) -> LRESULT
	{
		auto* web_view = reinterpret_cast<web_view_windows*>(GetWindowLongPtr(window, GWLP_USERDATA));// NOLINT(performance-no-int-to-ptr)

		switch (msg)
		{
			case WM_SIZE:
			{
				if (web_view && web_view->service_state() == gal::web_view::ServiceStateResult::RUNNING) { web_view->resize(); }
				return DefWindowProc(window, msg, w_param, l_param);
			}
			case WM_DPICHANGED:
			{
				if (web_view)
				{
					const auto  current_dpi  = HIWORD(w_param);
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
			default: { return DefWindowProc(window, msg, w_param, l_param); }
		}

		return 0;
	}
}// namespace

namespace gal::web_view::impl
{
	inline namespace v3
	{
		void WebViewWindows::saved_window_info::write_rect(const rect_type& target_rect) { rect = std::bit_cast<fake_rect>(target_rect); }

		WebViewWindows::WebViewWindows(
				const window_size_type window_width,
				const window_size_type window_height,
				string_type&&          window_title,
				const bool             window_is_fixed,
				const bool             window_is_fullscreen,
				const bool             web_view_use_dev_tools,
				const bool             is_temp_environment,
				string_type&&          index_url)
			: WebViewBase{
					  window_width,
					  window_height,
					  std::move(window_title),
					  window_is_fixed,
					  window_is_fullscreen,
					  web_view_use_dev_tools,
					  std::move(index_url)},
			  is_temp_environment_{is_temp_environment},
			  dpi_{0},
			  window_info_{},
			  window_{nullptr}
		{
			inject_javascript_code_ = "window.external." GAL_WEBVIEW_METHOD_NAME "=arg=>window.chrome.webview.postMessage(arg);";

			HMODULE handle;
			if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, nullptr, &handle) == 0 || !handle)
			{
				// todo
				MessageBox(nullptr, TEXT("Call to GetModuleHandleEx failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
				return;
			}

			// create a win32 window
			const WNDCLASSEX window{
					.cbSize = sizeof(WNDCLASSEX),
					.style = 0,
					.lpfnWndProc = WndProcedure,
					.cbClsExtra = 0,
					.cbWndExtra = 0,
					.hInstance = handle,
					.hIcon = LoadIcon(nullptr, IDI_APPLICATION),
					.hCursor = LoadCursor(nullptr, IDC_ARROW),
					.hbrBackground = reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1)),// NOLINT(performance-no-int-to-ptr)
					.lpszMenuName = nullptr,
					.lpszClassName = TEXT("webview_windows"),
					.hIconSm = LoadIcon(nullptr, IDI_APPLICATION)};

			if (!RegisterClassEx(&window))
			{
				// todo
				MessageBox(nullptr, TEXT("Call to RegisterClassEx failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
				return;
			}

			// Set default DPI awareness
			set_dpi_awareness();

			window_ = CreateWindow(
					TEXT("webview_windows"),
					to_wchar_string(window_title_).data(),
					WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					static_cast<int>(window_width_),
					static_cast<int>(window_height_),
					nullptr,
					nullptr,
					handle,
					nullptr);

			if (!window_)
			{
				// todo
				MessageBox(nullptr, TEXT("Window Registration Failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
				return;
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
					MulDiv(static_cast<int>(window_width_), static_cast<int>(dpi_), USER_DEFAULT_SCREEN_DPI),
					MulDiv(static_cast<int>(window_height_), static_cast<int>(dpi_), USER_DEFAULT_SCREEN_DPI),
					SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

			if (window_is_fixed_)
			{
				auto style = GetWindowLongPtr(window_, GWL_STYLE);
				style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
				SetWindowLongPtr(window_, GWL_STYLE, style);
			}

			// Used with GetWindowLongPtr in WndProcedure
			SetWindowLongPtr(window_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

			service_state_ = ServiceStateResult::INITIALIZED;
		}

		auto WebViewWindows::do_set_window_title(const string_view_type title) const -> void
		{
			SetWindowText(window_, to_wchar_string(title).data());
		}

		auto WebViewWindows::do_set_window_fullscreen(const bool to_fullscreen) -> void
		{
			if (to_fullscreen)
			{
				// Store window style before going fullscreen
				window_info_.style    = GetWindowLongPtr(window_, GWL_STYLE);
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

		auto WebViewWindows::do_navigate(const string_view_type target_url) const -> NavigateResult
		{
			if (FAILED(web_view_window_->Navigate(to_wchar_string(target_url).data())))
			{
				// todo
				MessageBox(nullptr, TEXT("Navigate failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
				return NavigateResult::NAVIGATE_FAILED;
			}
			return NavigateResult::SUCCESS;
		}

		auto WebViewWindows::do_eval(const string_view_type javascript_code) const -> void
		{
			// Schedule an async task to get the document URL
			web_view_window_->ExecuteScript(
					to_wchar_string(javascript_code).data(),
					Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
							[](const HRESULT error_code, const LPCWSTR result_object_as_json) -> HRESULT
							{
								// todo
								if (FAILED(error_code)) { (void)result_object_as_json; }
								return S_OK;
							})
					.Get());
		}

		auto WebViewWindows::do_service_start() -> ServiceStartResult
		{
			assert(service_state_ == ServiceStateResult::INITIALIZED && "Initialize service first!");

			ShowWindow(window_, SW_SHOWDEFAULT);
			UpdateWindow(window_);
			SetFocus(window_);

			// Set to single-thread
			if (FAILED(CoInitializeEx(
				nullptr,
				COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) { return ServiceStartResult::SERVICE_INITIALIZE_FAILED; }

			auto on_web_message_received =
					[this](
					[[maybe_unused]] ICoreWebView2*           web_view,
					ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
			{
				if (current_callback_)
				{
					LPWSTR message;
					// args->get_WebMessageAsJson(&message);
					if (const auto result = args->TryGetWebMessageAsString(&message);
						FAILED(result))
					{
						// todo
						return result;
					}

					current_callback_(*this, from_wchar_string(message));

					CoTaskMemFree(message);
				}
				return S_OK;
			};

			auto on_web_view_controller_create =
					[this, on_web_message_received](
					const HRESULT            error_code,
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
				if (web_view_use_dev_tools_)
				{
					if (FAILED(settings->put_AreDevToolsEnabled(true)))
					{
						// todo
					}
				}

				// Resize WebView
				resize();

				web_view_window_->AddScriptToExecuteOnDocumentCreated(
						to_wchar_string(inject_javascript_code_).data(),
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
				service_state_ = ServiceStateResult::RUNNING;

				if (window_is_fullscreen_) { do_set_window_fullscreen(true); }

				// navigate to the pending url
				(void)do_navigate(current_url_);

				return S_OK;
			};

			auto on_create_environment =
					[this, on_web_view_controller_create](
					const HRESULT             error_code,
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
					std::basic_string<raw_char_type> app_data_path{};
					app_data_path.resize(MAX_PATH);
					if (const auto read_length = GetEnvironmentVariable(TEXT("APPDATA"), app_data_path.data(), static_cast<DWORD>(app_data_path.size()));
						read_length != 0)
					{
						app_data_path.resize(read_length);
						// app_data_path.shrink_to_fit();
					}

					std::filesystem::path path{std::move(app_data_path)};
					assert(std::filesystem::exists(path) && "Invalid app data path!");
					return path;
				}();

				// Get executable file name
				std::basic_string<raw_char_type> exe_name{};
				exe_name.resize(MAX_PATH);
				if (const auto read_length = GetModuleFileName(nullptr, exe_name.data(), MAX_PATH);
					read_length != 0)
				{
					exe_name.resize(read_length);
					// exe_name.shrink_to_fit();
				}

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
				return ServiceStartResult::SERVICE_INITIALIZE_FAILED;
			}

			return ServiceStartResult::SUCCESS;
		}

		auto WebViewWindows::do_iteration() const -> bool
		{
			(void)this;
			MSG        msg;
			const auto is_running = GetMessage(&msg, nullptr, 0, 0);
			if (is_running)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			return is_running;
		}

		auto WebViewWindows::do_shutdown() const -> void
		{
			(void)this;
			PostQuitMessage(WM_QUIT);
			CoUninitialize();
		}

		auto WebViewWindows::resize() const -> void
		{
			RECT rect;
			GetClientRect(window_, &rect);
			web_view_controller_->put_Bounds(rect);
		}

		auto WebViewWindows::set_dpi(dpi_type new_dpi, const tagRECT& rect) -> void
		{
			const auto old_dpi = std::exchange(dpi_, new_dpi);

			if (window_is_fullscreen_)
			{
				// Scale the saved window size by the change in DPI
				auto       saved_rect = std::bit_cast<RECT>(window_info_.rect);
				const auto old_width  = saved_rect.right - saved_rect.left;
				const auto old_height = saved_rect.bottom - saved_rect.top;
				saved_rect.right      = saved_rect.left + MulDiv(static_cast<int>(old_width), static_cast<int>(new_dpi), static_cast<int>(old_dpi));
				saved_rect.bottom     = saved_rect.top + MulDiv(static_cast<int>(old_height), static_cast<int>(new_dpi), static_cast<int>(old_dpi));

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
	}
}

#endif
