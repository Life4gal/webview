#pragma once

#include <type_traits>
#include <string>
#include <string_view>
#include <functional>

namespace gal::web_view::impl
{
	inline namespace v3
	{
		template<typename ImplType>
		class WebViewBase
		{
			using impl_type = ImplType;

			[[nodiscard]] constexpr auto rep() & noexcept -> impl_type& { return static_cast<impl_type&>(*this); }

			[[nodiscard]] constexpr auto rep() const & noexcept -> const impl_type& { return static_cast<const impl_type&>(*this); }

		public:
			using window_size_type = int;
			using string_type = std::string;
			using string_view_type = std::string_view;
			using javascript_callback_type = std::function<auto(impl_type& /* web_view */, string_type&& /* string */) -> void>;

			// Start service
			enum class ServiceStartResult : std::uint8_t
			{
				SUCCESS,

				STATE_NOT_INITIALIZED,
				SERVICE_INITIALIZE_FAILED,

			};

			// Service state
			enum class ServiceStateResult : std::uint8_t
			{
				UNINITIALIZED,
				INITIALIZED,
				RUNNING,
				SHUTDOWN,
			};

			// Navigate to url
			enum class NavigateResult : std::uint8_t
			{
				SUCCESS,

				SERVICE_NOT_READY_YET,
				NAVIGATE_FAILED,
			};

			constexpr static window_size_type default_window_width{800};
			constexpr static window_size_type default_window_height{600};
			constexpr static string_view_type default_index_url{
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
							</html>)"
			};

		protected:
			window_size_type window_width_;
			window_size_type window_height_;
			string_type      window_title_;
			bool             window_is_fixed_;
			bool             window_is_fullscreen_;
			bool             web_view_use_dev_tools_;

			ServiceStateResult service_state_;

			string_type              current_url_;
			javascript_callback_type current_callback_;
			string_type              inject_javascript_code_;

			constexpr WebViewBase(
					const window_size_type window_width,
					const window_size_type window_height,
					string_type&&          window_title,
					const bool             window_is_fixed        = false,
					const bool             window_is_fullscreen   = false,
					const bool             web_view_use_dev_tools = false,
					string_type&&          index_url              = string_type{default_index_url}
					)
				: window_width_{window_width},
				  window_height_{window_height},
				  window_title_{std::move(window_title)},
				  window_is_fixed_{window_is_fixed},
				  window_is_fullscreen_{window_is_fullscreen},
				  web_view_use_dev_tools_{web_view_use_dev_tools},
				  service_state_{ServiceStateResult::UNINITIALIZED},
				  current_url_{std::move(index_url)} { }

		public:
			~WebViewBase() noexcept = default;

			WebViewBase(const WebViewBase&)                    = delete;
			WebViewBase(WebViewBase&&)                         = delete;
			auto operator=(const WebViewBase&) -> WebViewBase& = delete;
			auto operator=(WebViewBase&&) -> WebViewBase&      = delete;

			auto register_javascript_callback(javascript_callback_type&& callback) -> void
			{
				current_callback_.swap(callback);
				if constexpr (requires { rep().post_register_javascript_callback(std::declval<javascript_callback_type&>()); }) { rep().post_register_javascript_callback(current_callback_); }
			}

			auto set_window_title(string_type&& title) -> void
			{
				if (service_state_ != ServiceStateResult::RUNNING) { window_title_.swap(title); }
				else
				{
					if constexpr (requires { rep.do_set_window_title(std::declval<string_type&&>()); }) { rep().do_set_window_title(std::move(title)); }
					else { rep().do_set_window_title(string_view_type{title}); }
				}
			}

			auto set_window_title(string_view_type title) -> void
			{
				if (service_state_ != ServiceStateResult::RUNNING) { window_title_ = title; }
				else
				{
					if constexpr (requires { rep.do_set_window_title(std::declval<string_view_type>()); }) { rep().do_set_window_title(title); }
					else { rep().do_set_window_title(string_type{title}); }
				}
			}

			[[nodiscard]] constexpr auto service_state() const noexcept -> ServiceStateResult { return service_state_; }

			auto navigate(string_type&& target_url) -> NavigateResult
			{
				if (service_state_ != ServiceStateResult::RUNNING)
				{
					current_url_.swap(target_url);
					return NavigateResult::SERVICE_NOT_READY_YET;
				}

				if constexpr (requires { rep.do_navigate(std::declval<string_type&&>()); }) { return rep().do_navigate(std::move(target_url)); }
				else { return rep().do_navigate(string_view_type{target_url}); }
			}

			auto navigate(const string_view_type target_url) -> NavigateResult
			{
				if (service_state_ != ServiceStateResult::RUNNING)
				{
					current_url_ = target_url;
					return NavigateResult::SERVICE_NOT_READY_YET;
				}

				if constexpr (requires { rep.do_navigate(std::declval<string_view_type>()); }) { return rep().do_navigate(target_url); }
				else { return rep().do_navigate(string_type{target_url}); }
			}

			auto inject(const string_view_type inject_javascript_code) -> void
			{
				// todo: IIFE?
				constexpr string_view_type iife_left{"(() => {"};
				constexpr string_view_type iife_right{"})()"};

				inject_javascript_code_.append(iife_left).append(inject_javascript_code).append(iife_right);

				if constexpr (requires { rep().post_inject(std::declval<string_type&>()); }) { rep().post_inject(inject_javascript_code_); }
			}

			auto eval(string_view_type javascript_code) noexcept(noexcept(std::declval<impl_type&>().do_eval(javascript_code)))
				-> void { return rep().do_eval(javascript_code); }

			auto service_start() noexcept(noexcept(std::declval<impl_type&>().do_service_start())) -> ServiceStartResult
			{
				// if (service_state_ != service_state_result_type::INITIALIZED) { return service_start_result_type::STATE_NOT_INITIALIZED; }

				return rep().do_service_start();
			}

			auto iteration() noexcept(noexcept(std::declval<impl_type&>().do_iteration()))
				-> bool { return rep().do_iteration(); }

			auto shutdown() noexcept(noexcept(std::declval<impl_type&>().do_shutdown()))
				-> void { return rep().do_shutdown(); }
		};
	}
}
