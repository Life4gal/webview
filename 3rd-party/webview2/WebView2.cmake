set(WEBVIEW2_NAME Microsoft.Web.WebView2)
set(WEBVIEW2_VERSION 1.0.864.35)

set(WIL_NAME Microsoft.Windows.ImplementationLibrary)
set(WIL_VERSION 1.0.210204.1)

set(WEBVIEW2_PATH ${CMAKE_BINARY_DIR}/packages/${WEBVIEW2_NAME}.${WEBVIEW2_VERSION}/build/native)
set(WIL_PATH ${CMAKE_BINARY_DIR}/packages/${WIL_NAME}.${WIL_VERSION})

nuget_install(WebView2 ${${PROJECT_NAME_PREFIX}3RD_PARTY_PATH}/webview2/WebView2.config.in)

if (WEBVIEW_PUBLIC_WEBVIEW2)
	target_include_directories(
			${PROJECT_NAME}
			SYSTEM
			PUBLIC
			$<BUILD_INTERFACE:${WEBVIEW2_PATH}/include>
			$<BUILD_INTERFACE:${WIL_PATH}/include>
	)
else ()
	target_include_directories(
			${PROJECT_NAME}
			SYSTEM
			PRIVATE
			${WEBVIEW2_PATH}/include
			${WIL_PATH}/include
	)
endif (WEBVIEW_PUBLIC_WEBVIEW2)

if (WEBVIEW_STATIC_WEBVIEW2)
	target_link_libraries(
			${PROJECT_NAME}
			PRIVATE
			${WEBVIEW2_PATH}/x64/WebView2LoaderStatic.lib
	)
else ()
	target_link_libraries(
			${PROJECT_NAME}
			PRIVATE
			${WEBVIEW2_PATH}/x64/WebView2Loader.dll.lib
	)
	configure_file(
			${WEBVIEW2_PATH}/x64/WebView2Loader.dll
			#${CMAKE_BINARY_DIR}/WebView2Loader.dll
			${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/WebView2Loader.dll
			COPYONLY
	)
endif (WEBVIEW_STATIC_WEBVIEW2)

set(${PROJECT_NAME_PREFIX}3RD_PARTY_DEPENDENCIES ${${PROJECT_NAME_PREFIX}3RD_PARTY_DEPENDENCIES} "${WEBVIEW2_NAME} ${WEBVIEW2_VERSION}")
set(${PROJECT_NAME_PREFIX}3RD_PARTY_DEPENDENCIES ${${PROJECT_NAME_PREFIX}3RD_PARTY_DEPENDENCIES} "${WIL_NAME} ${WIL_VERSION}")
