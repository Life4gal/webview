#include <shellscalingapi.h>
#include <windows.h>

#include <bit>
#include <cassert>
#include <memory>
#include <type_traits>
#include <webview/impl/web_view_windows.hpp>

namespace
{
	using web_view_windows = gal::web_view::impl::WebViewWindows;
	using library_type = std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&::FreeLibrary)>;

	[[nodiscard]] auto load_library(const LPCSTR filename) -> library_type
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
			const HWND window,
			const UINT msg,
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
					const auto current_dpi = HIWORD(w_param);
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
	auto WebViewWindows::do_initialize(
			const size_type window_width,
			const size_type window_height,
			const bool is_window_resizable,
			const string_view_type window_title) -> bool
	{
		HMODULE handle;
		if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, nullptr, &handle) == 0 || !handle)
		{
			// todo
			return false;
		}

		window_title_ = window_title;

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
			MessageBox(nullptr, TEXT("Call to RegisterClassEx failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
			return false;
		}

		// Set default DPI awareness
		set_dpi_awareness();

		window_ = CreateWindow(
				TEXT("webview_windows"),
				window_title_.c_str(),
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

		ShowWindow(window_, SW_SHOWDEFAULT);
		UpdateWindow(window_);
		SetFocus(window_);

		return true;
	}

	// ReSharper disable once CppMemberFunctionMayBeConst
	auto WebViewWindows::do_shutdown() -> void
	{
		(void)this;
		PostQuitMessage(WM_QUIT);
		CoUninitialize();
	}

	auto WebViewWindows::window_info::write_rect(const tagRECT& target_rect) -> void
	{
		// if the size does not match, this will not compile.
		rect = std::bit_cast<fake_rect>(target_rect);
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

		if (is_fullscreen_)
		{
			// Scale the saved window size by the change in DPI
			auto saved_rect = std::bit_cast<RECT>(window_info_.rect);
			const auto old_width = saved_rect.right - saved_rect.left;
			const auto old_height = saved_rect.bottom - saved_rect.top;
			saved_rect.right = saved_rect.left + MulDiv(static_cast<int>(old_width), static_cast<int>(new_dpi), static_cast<int>(old_dpi));
			saved_rect.bottom = saved_rect.top + MulDiv(static_cast<int>(old_height), static_cast<int>(new_dpi), static_cast<int>(old_dpi));

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
