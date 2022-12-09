#pragma once

#include <type_traits>
#include <string>
#include <string_view>
#include <functional>
#include <optional>

namespace gal::web_view
{
	template<typename T>
	struct WebViewTypeDescriptor : std::false_type { };

	template<>
	struct WebViewTypeDescriptor<void>
	{
		using window_size_type = int;
		using string_type = std::string;
		using string_view_type = std::string_view;

		// template<typename ImplType, typename... Args>
		// using javascript_callback_type = std::function<auto(ImplType& /*self*/, Args&&... /*args*/) -> void>;
		template<typename ImplType>
		using javascript_callback_type = std::function<auto(ImplType&, string_type&&) -> void>;

		using url_type = string_type;
		using url_view_type = string_view_type;
		using javascript_code_type = string_type;
		using javascript_code_view_type = string_view_type;

		// Register callback
		using register_javascript_callback_result_type = void;
		// Set fullscreen
		using set_window_fullscreen_result_type = void;

		// Start service
		using service_start_result_type = bool;
		// Service state
		using service_state_result_type = bool;
		// Navigate to url
		using navigate_result_type = bool;
		// Inject JS code
		using inject_javascript_result_type = void;
		// Eval JS
		using javascript_execution_result_type = bool;
		// Iteration service
		using service_iteration_result_type = bool;
		// Shutdown service
		using service_shutdown_result_type = void;
	};

	template<typename ImplType>
	struct WebViewConstructArgs
	{
	private:
		using descriptor_type = std::conditional_t<
			WebViewTypeDescriptor<ImplType>::value,
			WebViewTypeDescriptor<ImplType>,
			WebViewTypeDescriptor<void>>;

	public:
		using window_size_type = typename descriptor_type::window_size_type;
		using string_type = typename descriptor_type::string_type;
		using string_view_type = typename descriptor_type::string_view_type;
		using url_type = typename descriptor_type::url_type;
		using url_view_type = typename descriptor_type::url_view_type;

		constexpr static url_view_type default_url{
				R"(data:text/html,
				<!DOCTYPE html>
				<html lang="en">
				<head>
				<meta charset="utf-8">
				<meta http-equiv="X-UA-Compatible" content="IE=edge">
				</head>
				<body>
				<div id="app"></div>
				<script type="text/javascript"></script>
				</body>
				</html>)"};

		window_size_type window_width;
		window_size_type window_height;
		string_type      window_title;
		bool             window_is_fixed;
		bool             window_is_fullscreen;
		bool             web_view_use_dev_tools;

		std::optional<url_type> index_url{};
	};

	template<typename ImplType>
	class WebViewBase
	{
	protected:
		using impl_type = ImplType;
		using descriptor_type = std::conditional_t<
			WebViewTypeDescriptor<impl_type>::value,
			WebViewTypeDescriptor<impl_type>,
			WebViewTypeDescriptor<void>>;
		using construct_args_type = WebViewConstructArgs<impl_type>;

	private:
		[[nodiscard]] constexpr auto rep() & noexcept -> impl_type& { return static_cast<impl_type&>(*this); }

		[[nodiscard]] constexpr auto rep() const & noexcept -> const impl_type& { return static_cast<const impl_type&>(*this); }

	public:
		using window_size_type = typename descriptor_type::window_size_type;
		using string_type = typename descriptor_type::string_type;
		using string_view_type = typename descriptor_type::string_view_type;

		using javascript_callback_type = typename descriptor_type::template javascript_callback_type<impl_type>;

		using url_type = typename descriptor_type::url_type;
		using url_view_type = typename descriptor_type::url_view_type;
		using javascript_code_type = typename descriptor_type::javascript_code_type;
		using javascript_code_view_type = typename descriptor_type::javascript_code_view_type;

		// Register callback
		using register_javascript_callback_result_type = typename descriptor_type::register_javascript_callback_result_type;
		// Set fullscreen
		using set_window_fullscreen_result_type = typename descriptor_type::set_window_fullscreen_result_type;

		// Start service
		using service_start_result_type = typename descriptor_type::service_start_result_type;
		// Service state
		using service_state_result_type = typename descriptor_type::service_state_result_type;
		// Navigate to url
		using navigate_result_type = typename descriptor_type::navigate_result_type;
		// Inject JS code
		using inject_javascript_result_type = typename descriptor_type::inject_javascript_result_type;
		// Eval JS
		using javascript_execution_result_type = typename descriptor_type::javascript_execution_result_type;
		// Iteration service
		using service_iteration_result_type = typename descriptor_type::service_iteration_result_type;
		// Shutdown service
		using service_shutdown_result_type = typename descriptor_type::service_shutdown_result_type;

		constexpr auto register_javascript_callback(javascript_callback_type&& callback)
			noexcept(noexcept(std::declval<impl_type&>().do_register_javascript_callback(std::move(callback))))
			-> register_javascript_callback_result_type { return rep().do_register_javascript_callback(std::move(callback)); }

		constexpr auto set_window_title(string_type&& title)
			noexcept(noexcept(std::declval<impl_type&>().do_set_window_title(std::move(title)))) -> void { return rep().do_set_window_title(std::move(title)); }

		constexpr auto set_window_title(string_view_type title)
			noexcept(noexcept(std::declval<impl_type&>().do_set_window_title(title))) -> void { return rep().do_set_window_title(title); }

		constexpr auto set_window_fullscreen(bool to_fullscreen)
			noexcept(noexcept(std::declval<impl_type&>().do_set_window_fullscreen(to_fullscreen))) -> set_window_fullscreen_result_type { return rep().do_set_window_fullscreen(to_fullscreen); }

		constexpr auto service_start()
			noexcept(noexcept(std::declval<impl_type&>().do_service_start()))
			-> service_start_result_type { return rep().do_service_start(); }

		[[nodiscard]] constexpr auto service_state() const
			noexcept(noexcept(std::declval<const impl_type&>().do_service_state()))
			-> service_state_result_type { return rep().do_service_state(); }

		constexpr auto navigate(url_type&& target_url)
			noexcept(noexcept(std::declval<impl_type&>().do_navigate(std::move(target_url))))
			-> navigate_result_type { return rep().do_navigate(std::move(target_url)); }

		constexpr auto navigate(const url_view_type target_url)
			noexcept(noexcept(std::declval<impl_type&>().do_navigate(target_url)))
			-> navigate_result_type { return rep().do_navigate(target_url); }

		constexpr auto inject(javascript_code_type&& inject_javascript_code) noexcept(noexcept(std::declval<impl_type&>().do_inject(std::move(inject_javascript_code))))
			-> inject_javascript_result_type { return rep().do_navigate(std::move(inject_javascript_code)); }

		constexpr auto inject(javascript_code_view_type inject_javascript_code) noexcept(noexcept(std::declval<impl_type&>().do_inject(inject_javascript_code)))
			-> inject_javascript_result_type { return rep().do_navigate(inject_javascript_code); }

		constexpr auto eval(javascript_code_type&& javascript_code)
			noexcept(noexcept(std::declval<impl_type&>().do_eval(std::move(javascript_code))))
			-> javascript_execution_result_type { return rep().do_eval(std::move(javascript_code)); }

		constexpr auto eval(javascript_code_view_type javascript_code)
			noexcept(noexcept(std::declval<impl_type&>().do_eval(javascript_code)))
			-> javascript_execution_result_type { return rep().do_eval(javascript_code); }

		constexpr auto iteration()
			noexcept(noexcept(std::declval<impl_type&>().do_iteration()))
			-> service_iteration_result_type { return rep().do_iteration(); }

		constexpr auto shutdown()
			noexcept(noexcept(std::declval<impl_type&>().do_shutdown()))
			-> service_shutdown_result_type { return rep().do_shutdown(); }
	};
}
