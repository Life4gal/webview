#include <charconv>
#include <filesystem>
#include <fstream>
#include <webview/webview.hpp>

template<typename FunctionType>
struct y_combinator
{
	using function_type = FunctionType;

	function_type function;

	template<typename... Args>
	constexpr auto operator()(Args&&... args) const
		noexcept(std::is_nothrow_invocable_v<function_type, decltype(*this), Args...>) -> decltype(auto)
	{
		// we pass ourselves to f, then the arguments.
		// the lambda should take the first argument as `auto&& recurse` or similar.
		return function(*this, std::forward<Args>(args)...);
	}
};

template<typename F>
y_combinator(F) -> y_combinator<std::decay_t<F>>;

#define JS_SEND_ARG_METHOD_NAME "to_native"
#define JS_RECEIVE_RESULT_METHOD_NAME "from_native"
#define JS_SHUTDOWN_METHOD_NAME "shutdown"


auto build_html() -> bool
{
	constexpr std::string_view head_part{
			R"(
		<!DOCTYPE html>
		<html lang="en">
		<head>
		<meta charset="UTF-8" />
		<meta name="viewport" content="width=device-width, initial-scale=1.0" />
		<meta http-equiv="X-UA-Compatible" content="ie=edge" />
		<title>callback test</title>
		</head>
		<body>

		<input type="text" id="input" />
		<button id="btn">Calculate</button>
		<div id="ans"></div>

		<script type="text/javascript">
		)"};
	constexpr std::string_view foot_part{
			R"(
		</script>
		</body>
		</html>
		)"};

	constexpr std::string_view script_part{
			"\n"
			"const input = document.getElementById('input');\n"
			"   const btn = document.getElementById('btn');\n"
			"   const ans = document.getElementById('ans');\n"
			"\n"
			"   input.onkeydown = function(e) {\n"
			"       if (e.key === 'Enter') {\n"
			"           " JS_SEND_ARG_METHOD_NAME
			"();\n"
			"       }\n"
			"   };\n"
			"   btn.onclick = " JS_SEND_ARG_METHOD_NAME
			";"
			"   function " JS_SEND_ARG_METHOD_NAME
			"() {\n"
			"       if (!input.value) {\n"
			"           input.value = 0;\n"
			"       }\n"
			"\n"
			"       // Windows \n"
			"       // window.chrome.webview.postMessage(input.value);\n"
			"       // Linux\n"
			"       // window.webkit.messageHandlers.external.postMessage(input.value);\n"
			"       window.external." GAL_WEBVIEW_METHOD_NAME
			"(input.value);\n"
			"   }\n"
			"\n"
			"   function " JS_RECEIVE_RESULT_METHOD_NAME
			"(result) {\n"
			"       ans.innerHTML = 'Factorial: ' + result;\n"
			"   }"
			"\n"};

	const std::filesystem::path html_path{CALLBACK_TEST_HTML_PATH};
	std::filesystem::create_directories(html_path.parent_path());

	std::ofstream file{html_path, std::ios::out | std::ios::trunc};
	if (!file.is_open()) { return false; }

	file.write(head_part.data(), head_part.size());
	file.write(script_part.data(), script_part.size());
	file.write(foot_part.data(), foot_part.size());
	file.close();
	return true;
}

#ifdef GAL_WEBVIEW_PLATFORM_WINDOWS
auto __stdcall WinMain(
		_In_ const HINSTANCE /* hInstance */,
		_In_opt_ HINSTANCE /* hPrevInstance */,
		_In_ LPSTR /* lpCmdLine */,
		_In_ int/* nShowCmd */
		) -> int
	#else
auto main(
		int /* argc */,
		char* /* argv */[]
		) -> int
	#endif
{
	if (!build_html())
	{
		// todo
		return -1;
	}

	gal::web_view::WebView web_view{
			/*.window_width = */600,
			                    /*.window_height = */ 800,
			                    /*.window_title = */ "hello web view",
			                    /*.window_is_fixed = */ false,
			                    /*.window_is_fullscreen = */ false,
			                    /*.web_view_use_dev_tools = */ true,
			                    /*.index_url = */ {},
	};

	gal::web_view::WebView::string_type target_url{"file:///"};
	target_url.append(CALLBACK_TEST_HTML_PATH);
	web_view.navigate(target_url);

	web_view.register_javascript_callback(
			[](gal::web_view::WebView& wv, gal::web_view::WebView::string_type&& arg) -> void
			{
				constexpr auto  factorial = y_combinator{
						[](auto self, std::size_t n) -> std::size_t
						{
							if (n <= 1) { return 1; }
							return n * self(n - 1);
						}};

				if (arg == JS_SHUTDOWN_METHOD_NAME)
				{
					wv.shutdown();
					return;
				}

				std::size_t num;
				if (const auto [ptr, ec] = std::from_chars(
							arg.c_str(),
							arg.c_str() + arg.size(),
							num);
					ec != std::errc{} || ptr != arg.c_str() + arg.size())
				{
					const gal::web_view::WebView::string_type js{JS_RECEIVE_RESULT_METHOD_NAME "(\"cannot eval '" + arg + "' for factorial!\")"};
					wv.eval(js);
				}
				else
				{
					const gal::web_view::WebView::string_type js{JS_RECEIVE_RESULT_METHOD_NAME "(" + std::to_string(factorial(num)) + ")"};
					wv.eval(js);
				}
			});

	if (web_view.service_start() != gal::web_view::ServiceStartResult::SUCCESS) { return -2; }

	while (web_view.iteration()) { }

	return 0;
}
