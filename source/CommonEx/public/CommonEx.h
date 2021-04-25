#pragma once

//Verbose logging
#define RVERBOSE 1

//Verbose Transport Layer(enable logging)
#define VERBOSE_TLAYER 0

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
}

#include "RStatus.h"
#include "Diag.h"
#include "Platform.h"
#include "Core.h"
#include "EntityId.h"
#include "Stream.h"
#include "Worker.h"
#include "TransportLayer.h"
#include "ConnectionEndpoint.h"
