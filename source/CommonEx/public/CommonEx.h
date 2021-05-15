#pragma once

//Verbose logging
#ifndef RVERBOSE
#define RVERBOSE 1
#endif

//Verbose Transport Layer(enable logging)
#ifndef VERBOSE_TLAYER
#define VERBOSE_TLAYER 1
#endif

//Enable memory statitics
#ifndef MEMEX_STATISTICS
#define MEMEX_STATISTICS 1
#endif

//Enable buffers statitics
#ifndef COMMONEX_BUFFERS_STATISTICS
#define COMMONEX_BUFFERS_STATISTICS 1
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define COMMONEX_WIN32_PLATFROM 1
#else
//@TODO 
#endif

#ifndef COMMONEX_USE_DEFAULTS
#define COMMONEX_USE_DEFAULTS 1
#endif

/*------------------------------------------------------------
	Format library (fmt)
		Author:		fmtlib
		Url:		https://github.com/fmtlib/fmt
		Licence:	https://github.com/fmtlib/fmt/blob/master/LICENSE.rst
  ------------------------------------------------------------*/
#include <fmt/core.h>
#include <fmt/color.h>

#undef CONCAT_
#define CONCAT_(x,y) x##y

#undef CONCAT
#define CONCAT(x,y) CONCAT_(x,y)

#undef STRINGIFY
#define STRINGIFY(x) #x

#undef TOSTRING
#define TOSTRING(x) STRINGIFY(x)

#ifdef _MSC_VER
#define noop __noop
#define NOVTABLE __declspec(novtable)
#else 
#define noop 
#define NOVTABLE noop
#endif

#ifndef _LIKELY
#if _HAS_CXX20
#define _LIKELY [[likely]]
#else
#define _LIKELY 
#endif
#endif

#ifndef _FALLTHROUGH
#if _HAS_CXX20
#define _FALLTHROUGH [[fallthrough]]
#else
#define _FALLTHROUGH
#endif
#endif

// Standard libs
#include <string>
#include <string_view>
#include <type_traits>
#include <cstdint>
#include <thread>
#include <csignal>
#include <time.h>
#include <atomic>
#include <limits>
#include <optional>
#include <chrono>
#include <cassert>
#include <queue>
#include <mutex>

//## From vcruntime
// [[nodiscard]] attributes on STL functions
#ifndef _HAS_NODISCARD
#ifndef __has_cpp_attribute
#define _HAS_NODISCARD 0
#elif __has_cpp_attribute(nodiscard) >= 201603L // TRANSITION, VSO#939899 (need toolset update)
#define _HAS_NODISCARD 1
#else
#define _HAS_NODISCARD 0
#endif
#endif // _HAS_NODISCARD

#ifndef _NODISCARD
#if _HAS_NODISCARD
#define _NODISCARD [[nodiscard]]
#else // ^^^ CAN HAZ [[nodiscard]] / NO CAN HAZ [[nodiscard]] vvv
#define _NODISCARD
#endif // _HAS_NODISCARD
#endif // _HAS_NODISCARD

#ifndef FORCEINLINE
#define FORCEINLINE __forceinline
#endif

#ifndef ALIGNMENT
#define ALIGNMENT alignof(size_t)
#endif

namespace CommonEx {
	/*------------------------------------------------------------
		Buffer interface
	  ------------------------------------------------------------*/
	struct IBuffer
	{
		uint32_t Length{ 0 };
		uint8_t* Buffer{ nullptr };
	};

	using TClock = std::chrono::high_resolution_clock;
	#define TCLOCK_MILLIS(x)  std::chrono::milliseconds(x)

	// Global allocate block of memory
	extern void* GAllocate(size_t BlockSize, size_t BlockAlignment) noexcept;

	// Global deallocate block of memory
	extern void GFree(void* BlockPtr) noexcept;

	template<typename T>
	FORCEINLINE void GDestructNothrow(T* Ptr)noexcept
	{
		if constexpr (std::is_nothrow_destructible_v<T>)
		{
			Ptr->~T();
		}
		else if constexpr (std::is_destructible_v<T>)
		{
			static_assert(false, "GDestructNothrow<T>() T must be nothrow destructible");
		}
	}

	template<typename T, typename ...TArgs>
	FORCEINLINE constexpr void GConstructNothrow(void* Ptr, TArgs... Args)noexcept
	{
		if constexpr (sizeof...(TArgs) == 0)
		{
			new (Ptr) T();

			//if constexpr (std::is_nothrow_default_constructible_v<T>)
			//{
			//	new (Ptr) T();
			//}
			//else if constexpr (std::is_default_constructible_v<T>)
			//{
			//	static_assert(false, "GConstructNothrow<T,TArgs>() T must be nothrow default constructible");
			//}
		}
		else
		{ 
			//@TODO check if specific nothrow constructor exists on type T

			new (Ptr) T(std::forward<TArgs>(Args)...);
		}
	}

	template<typename T>
	FORCEINLINE void GFreeCpp(T* Ptr)noexcept
	{
		GDestructNothrow<T>(Ptr);
		GFree(Ptr);
	}

	template<typename T, typename ...TArgs>
	FORCEINLINE T* GAllocateCpp(TArgs... Args)noexcept
	{
		auto* Block = GAllocate(sizeof(T), ALIGNMENT);

		GConstructNothrow<T, TArgs...>(Block, std::forward<TArgs>(Args)...);

		return (T*)Block;
	}
}

#include "TypeTraits.h"
#include "Tunning.h"
#include "Misc.h"
#include "RStatus.h"
#include "Diag.h"
#include "Platform.h"
#include "Core.h"
#include "EntityId.h"
#include "Stream.h"
#include "Task.h"
#include "TransportLayer.h"
#include "Memory.h"
#include "TObjectPool.h"
#include "Ptr.h"
#include "MemoryManager.h"
#include "TStructures.h"
#include "Packet.h"
#include "Opcode.h"
#include "Work.h"
#include "Buffer.h"
#include "PacketBuilder.h"
#include "Worker.h"
#include "Async.h"
#include "System.h"
#include "ServerController.h"
#include "ConnectionEndpoint.h"

namespace CommonEx {
	//Initialize the CommonEx library
	RStatus InitializeCommonEx(int32_t argc, const char** argv)noexcept;
	RStatus ShutdownCommonEx()noexcept;
}