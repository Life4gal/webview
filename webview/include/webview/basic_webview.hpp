#pragma once

#include <functional>

namespace gal::webview
{
	template<typename DerivedImpl>
	class basic_webview
	{
	public:
		using impl_type = DerivedImpl;

		using size_type = impl_type::size_type;
		using string_type = impl_type::string_type;
		using string_view_type = impl_type::string_view_type;
		using callback_type = std::function<void(impl_type&, string_type&)>;

		bool initialize(
				size_type window_width,
				size_type window_height,
				bool is_window_resizable,
				string_view_type window_title,
				string_view_type window_default_context
				)
		{
			return static_cast<impl_type&>(*this).do_initialize(window_width, window_height, is_window_resizable, window_title, window_default_context);
		}

		void register_callback(callback_type callback)
		{
			static_cast<impl_type &>(*this).do_register_callback(std::move(callback));
		}
	};
}
