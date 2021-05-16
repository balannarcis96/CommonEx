#pragma once
/**
 * @file Platform.h
 *
 * @brief CommonEx Platform specific abstractions
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

#if COMMONEX_WIN32_PLATFROM
#include "Win32Platform.h"
#else 
#include "FreeBSDPlatform.h"
#endif

namespace CommonEx
{
#if COMMONEX_WIN32_PLATFROM
	using CurrentAsyncIOSystem = Win32AsyncIO;

	template<typename ...Types>
	inline void Alert(const char* Format, Types... Args)  noexcept
	{
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
	inline void Alert(const wchar_t* Format, Types... Args)  noexcept
	{
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

	FORCEINLINE int32_t GGetLastError() noexcept
	{
		return (int32_t)GetLastError();
	}

#else 
	static_assert(false, "@TODO");
#endif

	//UTF16 -> UTF8
	template<size_t N>
	inline bool GWideCharToMultiByte(const wchar_t(&InBuffer)[N], char* OutBuffer, int32_t OutBufferSize) noexcept;

	//UTF8 -> UTF16
	template<size_t N>
	inline bool GMultiByteToWideChar(const char(&InBuffer)[N], wchar_t* OutBuffer, int32_t OutBufferSize) noexcept;

	//UTF16 -> UTF8
	template<size_t N, size_t M>
	inline bool GWideCharToMultiByte(const wchar_t(&InBuffer)[N], char(&OutBuffer)[M]) noexcept;

	//UTF8 -> UTF16
	template<size_t N, size_t M>
	inline bool GMultiByteToWideChar(const char(&InBuffer)[N], wchar_t(&OutBuffer)[M]) noexcept;

#if COMMONEX_WIN32_PLATFROM

	//UTF16 -> UTF8
	template<size_t N>
	inline bool GWideCharToMultiByte(const wchar_t(&InBuffer)[N], char* OutBuffer, int32_t OutBufferSize) noexcept
	{
		int32_t result = 0;
		if ((result = ::WideCharToMultiByte(CP_UTF8, 0, InBuffer, (int32_t)wcsnlen_s(InBuffer, N), OutBuffer, OutBufferSize, 0, 0)) == 0)
		{
			return false;
		}

		OutBuffer[result] = '\0';

		return true;
	}

	//UTF8 -> UTF16
	template<size_t N>
	inline bool GMultiByteToWideChar(const char(&InBuffer)[N], wchar_t* OutBuffer, int32_t OutBufferSize) noexcept
	{
		int32_t result = 0;
		if ((result = ::MultiByteToWideChar(CP_UTF8, 0, InBuffer, (int32_t)strnlen_s(InBuffer, N), OutBuffer, OutBufferSize)) == 0)
		{
			return false;
		}

		OutBuffer[result] = '\0';

		return true;
	}

	//UTF16 -> UTF8
	template<size_t N, size_t M>
	inline bool GWideCharToMultiByte(const wchar_t(&InBuffer)[N], char(&OutBuffer)[M]) noexcept
	{
		int32_t result = 0;
		if ((result = ::WideCharToMultiByte(CP_UTF8, 0, InBuffer, (int32_t)wcsnlen_s(InBuffer, N), OutBuffer, (int32_t)M, 0, 0)) == 0)
		{
			return false;
		}

		OutBuffer[result] = '\0';

		return true;
	}

	//UTF8 -> UTF16
	template<size_t N, size_t M>
	inline bool GMultiByteToWideChar(const char(&InBuffer)[N], wchar_t(&OutBuffer)[M]) noexcept
	{
		int32_t result = 0;
		if ((result = ::MultiByteToWideChar(CP_UTF8, 0, InBuffer, (int32_t)strnlen_s(InBuffer, N), OutBuffer, (int32_t)M)) == 0)
		{
			return false;
		}

		OutBuffer[result] = '\0';

		return true;
	}

#else 
	static_assert(false, "@TODO");
#endif

}

#include <mysql.h>
#include <my_command.h>
