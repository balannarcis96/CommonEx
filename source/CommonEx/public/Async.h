#pragma once

namespace CommonEx{
	//Forward declarations
	class TAsync;
	class TAsyncTimer;
	using TExecuterList = TDQueue<TAsync*>;

	//Global (per thread) state of the Async subsystem
	class AsyncSystem final
	{
	public:
		//This must be called by every thread that uses the Async subsystem
		static RStatus InitializeThread() noexcept;

		static inline thread_local TAsyncTimer* PTR		GTimer{ nullptr };
		static inline thread_local int64_t				GTickCount{ 0 };
		static inline thread_local TExecuterList* PTR	GExecuterList{ nullptr };
		static inline thread_local TAsync* PTR			GCurrentExecuterOccupyingThisThread{ nullptr };
	};
}

#include "AsyncDispatcher/AsyncTask.h"
#include "AsyncDispatcher/AsyncDispatcher.h"