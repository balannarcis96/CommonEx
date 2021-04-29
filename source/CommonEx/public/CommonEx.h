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

  /*------------------------------------------------------------
	  The Impossibly Fast C++ Delegates
		  Author:		Sergey Alexandrovich Kryukov
		  Url:		https://www.codeproject.com/Articles/1170503/The-Impossibly-Fast-Cplusplus-Delegates-Fixed
		  Licence:	https://opensource.org/licenses/mit-license.php (MIT)
  ------------------------------------------------------------*/
#include <Delegate.h>
#include <MultiCastDelegate.h>

namespace CommonEx {
	template<typename ReturnType, typename ...Params>
	using Delegate = SA::delegate<ReturnType(Params...)>;

	template<typename ReturnType, typename ...Params>
	using MulticastDelegate = SA::multicast_delegate<ReturnType(Params...)>;
}

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
}

#include "Tunning.h"
#include "RStatus.h"
#include "Diag.h"
#include "Platform.h"
#include "Core.h"
#include "EntityId.h"
#include "Stream.h"
#include "Worker.h"
#include "TransportLayer.h"
#include "Task.h"
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
#include "ConnectionEndpoint.h"

namespace CommonEx {
	//Initialize the CommonEx library
	RStatus InitializeCommonEx(int32_t argc, const char** argv)noexcept;
	RStatus ShutdownCommonEx()noexcept;
}