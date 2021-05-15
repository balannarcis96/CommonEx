#pragma once

namespace CommonEx
{
	namespace Async
	{
		//Forward declarations
		class AsyncDispatcher;
		class AsyncTimerTaskDispatcher;
		using TExecuterList = TQueue<AsyncDispatcher*>;
		using TClock = std::chrono::high_resolution_clock;

#define TCLOCK_MILLIS(x)  std::chrono::milliseconds(x)

		//Global (per thread) state of the Async subsystem
		class AsyncSystem final
		{
		public:
			//This must be called by every thread that uses the Async subsystem
			static RStatus InitializeThread() noexcept;
			static void TickThread() noexcept;
			static void ShutdownThread() noexcept;

			static thread_local AsyncTimerTaskDispatcher* PTR	GTimer;
			static thread_local int64_t							GTickCount;
			static thread_local TExecuterList* PTR				GExecuterList;
			static thread_local AsyncDispatcher* PTR			GCurrentExecuterOccupyingThisThread;
		};
	}
}

#include "AsyncDispatcher/AsyncTask.h"
#include "AsyncDispatcher/AsyncDispatcher.h"