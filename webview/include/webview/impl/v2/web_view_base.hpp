#pragma once

#include <type_traits>
#include <string>
#include <string_view>
#include <functional>
#include <optional>

#include "WebView2.h"

namespace gal::web_view
{
	inline namespace v2
	{
		enum class ServiceStartResult : std::uint8_t
		{
			SUCCESS,

			STATE_NOT_INITIALIZED,
			SERVICE_INITIALIZE_FAILED,

		};

		enum class ServiceStateResult : std::uint8_t
		{
			UNINITIALIZED,
			INITIALIZED,
			RUNNING,
			SHUTDOWN,
		};

		enum class NavigateResult : std::uint8_t
		{
			SUCCESS,

			SERVICE_NOT_READY_YET,
			NAVIGATE_FAILED,
		};

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
			using service_start_result_type = ServiceStartResult;
			// Service state
			using service_state_result_type = ServiceStateResult;
			// Navigate to url
			using navigate_result_type = NavigateResult;
			// Inject JS code
			using inject_javascript_result_type = void;
			// Eval JS
			using javascript_execution_result_type = bool;
			// Iteration service
			using service_iteration_result_type = bool;
			// Shutdown service
			using service_shutdown_result_type = void;
		};

		template<typename T>
		struct IsWebViewTypeDescriptor : std::false_type {};

		template<typename T>
		struct IsWebViewTypeDescriptor<WebViewTypeDescriptor<T>> : std::true_type {};

		namespace detail
		{
			template<typename StringViewType>
			[[nodiscard]] consteval auto generate_default_url() -> StringViewType
			{
				if constexpr (std::is_same_v<typename StringViewType::value_type, char>)
				{
					return StringViewType{
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
				}
				else
				{
					return StringViewType{
							LR"(data:text/html,
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
				}
			}
		}

		template<typename Descriptor>
		struct WebViewConstructArgs;

		template<typename Descriptor>
			requires IsWebViewTypeDescriptor<Descriptor>::value
		struct WebViewConstructArgs<Descriptor>
		{
			using descriptor_type = Descriptor;

			using window_size_type = typename descriptor_type::window_size_type;
			using string_type = typename descriptor_type::string_type;
			using string_view_type = typename descriptor_type::string_view_type;
			using url_type = typename descriptor_type::url_type;
			using url_view_type = typename descriptor_type::url_view_type;

			constexpr static string_view_type default_url{detail::generate_default_url<string_view_type>()};

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
			using impl_type = ImplType;

			[[nodiscard]] constexpr auto rep() & noexcept -> impl_type& { return static_cast<impl_type&>(*this); }

			[[nodiscard]] constexpr auto rep() const & noexcept -> const impl_type& { return static_cast<const impl_type&>(*this); }

		public:
			using descriptor_type = std::conditional_t<
				WebViewTypeDescriptor<impl_type>::value,
				WebViewTypeDescriptor<impl_type>,
				WebViewTypeDescriptor<void>>;
			using construct_args_type = WebViewConstructArgs<descriptor_type>;

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

		protected:
			window_size_type window_width_;
			window_size_type window_height_;
			string_type      window_title_;

			bool                      window_is_fixed_;
			bool                      window_is_fullscreen_;
			bool                      web_view_use_dev_tools_;
			service_state_result_type service_state_;

			url_type                 current_url_;
			javascript_callback_type current_callback_;
			javascript_code_type     inject_javascript_code_;

			constexpr explicit WebViewBase(construct_args_type&& args)
				: window_width_{args.window_width},
				  window_height_{args.window_height},
				  window_title_{args.window_title},
				  window_is_fixed_{args.window_is_fixed},
				  window_is_fullscreen_{args.window_is_fullscreen},
				  web_view_use_dev_tools_{args.web_view_use_dev_tools},
				  service_state_{service_state_result_type::UNINITIALIZED},
				  current_url_{args.index_url.value_or(construct_args_type::default_url)} {}

		public:
			auto register_javascript_callback(javascript_callback_type&& callback) -> register_javascript_callback_result_type
			{
				current_callback_.swap(callback);
				if constexpr (requires { rep().post_register_javascript_callback(std::declval<javascript_callback_type&>()); }) { return rep().post_register_javascript_callback(current_callback_); }
				else
				{
					if constexpr (!std::is_same_v<register_javascript_callback_result_type, void>) { return register_javascript_callback_result_type{}; }
					return;
				}
			}

			auto set_window_title(string_type&& title) -> void
			{
				if (service_state_ != service_state_result_type::RUNNING) { window_title_.swap(title); }
				else
				{
					if constexpr (requires { rep.do_set_window_title(std::declval<string_type&&>()); }) { rep().do_set_window_title(std::move(title)); }
					else { rep().do_set_window_title(string_view_type{title}); }
				}
			}

			auto set_window_title(string_view_type title) -> void
			{
				if (service_state_ != service_state_result_type::RUNNING) { window_title_.swap(title); }
				else
				{
					if constexpr (requires { rep.do_set_window_title(std::declval<string_view_type>()); }) { rep().do_set_window_title(title); }
					else { rep().do_set_window_title(string_type{title}); }
				}
			}

			auto set_window_fullscreen(const bool to_fullscreen) noexcept(noexcept(std::declval<impl_type&>().do_set_window_fullscreen(to_fullscreen))) -> set_window_fullscreen_result_type
			{
				if (window_is_fullscreen_ != to_fullscreen)
				{
					window_is_fullscreen_ = to_fullscreen;
					return rep().do_set_window_fullscreen(window_is_fullscreen_);
				}

				if constexpr (!std::is_same_v<register_javascript_callback_result_type, void>) { return set_window_fullscreen_result_type{}; }

				return;
			}

			[[nodiscard]] constexpr auto service_state() const noexcept
				-> service_state_result_type { return service_state_; }

			auto navigate(url_type&& target_url)
				-> navigate_result_type
			{
				if (service_state_ != service_state_result_type::RUNNING)
				{
					current_url_.swap(target_url);
					return navigate_result_type::SERVICE_NOT_READY_YET;
				}

				if constexpr (requires { rep.do_navigate(std::declval<url_type&&>()); }) { return rep().do_navigate(std::move(target_url)); }
				else { return rep().do_navigate(string_view_type{target_url}); }
			}

			auto navigate(const url_view_type target_url)
				-> navigate_result_type
			{
				if (service_state_ != service_state_result_type::RUNNING)
				{
					current_url_ = target_url;
					return navigate_result_type::SERVICE_NOT_READY_YET;
				}

				if constexpr (requires { rep.do_navigate(std::declval<url_view_type>()); }) { return rep().do_navigate(target_url); }
				else { return rep().do_navigate(string_type{target_url}); }
			}

			auto inject(javascript_code_view_type inject_javascript_code)
				-> inject_javascript_result_type
			{
				// todo: IIFE?
				if constexpr (std::is_same_v<typename string_view_type::value_type, char>)
				{
					constexpr string_view_type iife_left{"(() => {"};
					constexpr string_view_type iife_right{"})()"};

					inject_javascript_code_.append(iife_left).append(inject_javascript_code).append(iife_right);

					if constexpr (requires { rep().post_inject(std::declval<javascript_code_type&>()); }) { return rep().post_inject(inject_javascript_code_); }
					else
					{
						if constexpr (!std::is_same_v<inject_javascript_result_type, void>) { return inject_javascript_result_type{}; }
						return;
					}
				}
				else
				{
					constexpr string_view_type iife_left{L"(() => {"};
					constexpr string_view_type iife_right{L"})()"};

					inject_javascript_code_.append(iife_left).append(inject_javascript_code).append(iife_right);

					if constexpr (requires { rep().post_inject(std::declval<javascript_code_type&>()); }) { return rep().post_inject(inject_javascript_code_); }
					else
					{
						if constexpr (!std::is_same_v<inject_javascript_result_type, void>) { return inject_javascript_result_type{}; }
						return;
					}
				}
			}

			auto eval(javascript_code_view_type javascript_code) noexcept(noexcept(std::declval<impl_type&>().do_eval(javascript_code)))
				-> javascript_execution_result_type { return rep().do_eval(javascript_code); }

			auto service_start() noexcept(noexcept(std::declval<impl_type&>().do_service_start())) -> service_start_result_type
			{
				// if (service_state_ != service_state_result_type::INITIALIZED) { return service_start_result_type::STATE_NOT_INITIALIZED; }

				return rep().do_service_start();
			}

			auto iteration() noexcept(noexcept(std::declval<impl_type&>().do_iteration()))
				-> service_iteration_result_type { return rep().do_iteration(); }

			auto shutdown() noexcept(noexcept(std::declval<impl_type&>().do_shutdown()))
				-> service_shutdown_result_type { return rep().do_shutdown(); }
		};
	}

}
