#pragma once

#if COMMONEX_WIN32_PLATFROM
#include "Win32Platform.h"
#else 
#include "FreeBSDPlatform.h"
#endif

namespace CommonEx {
#if COMMONEX_WIN32_PLATFROM
	using CurrentAsyncIOSystem = Win32AsyncIO;
	
	template<typename ...Types>
	inline void Alert(const char* Format, Types... Args)  noexcept {
		const std::string Msg = fmt::format(Format, Args...);
#if COMMONEX_WIN32_PLATFROM
		MessageBoxA(
			NULL,
			Msg.c_str(),
			"Message",
			MB_OK);
#else 
		static_assert(false, "@TODO");
#endif
	}

	template<typename ...Types>
	inline void Alert(const wchar_t* Format, Types... Args)  noexcept {
		const std::wstring Msg = fmt::format(Format, Args...);
#if COMMONEX_WIN32_PLATFROM

		MessageBoxW(
			NULL,
			Msg.c_str(),
			L"Message",
			MB_OK);
#else 
		static_assert(false, "@TODO");
#endif
	}


#else 
	static_assert(false, "@TODO");
#endif
}