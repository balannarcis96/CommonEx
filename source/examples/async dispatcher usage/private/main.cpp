#include "../public/Example.h"

namespace CommonEx
{
	ptr_t GAllocate(size_t BlockSize, size_t BlockAlignment) noexcept
	{
		return _aligned_malloc(BlockSize, BlockAlignment);
	}

	void GFree(ptr_t BlockPtr) noexcept
	{
		_aligned_free(BlockPtr);
	}
}

using namespace CommonEx;

class TypeA : public Async::AsyncDispatcher
{
public:
	void DoA(int32_t i) noexcept
	{
		LogInfo("From Task 2 {}!!", i);
	}
};

void SmallTests(int argc, const char** argv) noexcept
{

	{
		Async::AsyncTimerTaskPriorityQueue pq;

		pq.push(Async::AsyncTimerTask(0, nullptr, nullptr));
		pq.push(Async::AsyncTimerTask(6, nullptr, nullptr));
		pq.push(Async::AsyncTimerTask(2, nullptr, nullptr));
		pq.push(Async::AsyncTimerTask(1, nullptr, nullptr));
		pq.push(Async::AsyncTimerTask(3, nullptr, nullptr));

		const Async::AsyncTimerTask& task = pq.top();
	}

	{
		TQueue<uint64_t> q;

		for (uint64_t i = 0; i <= 500000; i++)
		{
			q.Push((uint64_t)500000 - i);
		}

		while (q.GetSize())
		{
			LogInfo("Top: {}", q.Top());
			q.Pop();
		}
	}

	{
		TPriorityQueue<int32_t> pq;

		for (int32_t i = 1; i < 500000; i++)
		{
			pq.push(500000 - i);
		}

		//LogInfo("Min: {}", pq.top());

		while (!pq.empty())
		{
			LogInfo("PQ Min : {}", pq.top());
			pq.pop();
		}
	}

}

void AsyncDispatcherUsage(int argc, const char** argv) noexcept
{
	auto d = MakeSharedManagedEx<TypeA>();

	WorkerGroup<32> wg;
	{
		RStatus Result = wg.Initialize(1,
			[d](WorkerBase* Self, WorkerGroupShared* Group) mutable
			{
				int  i = 8 * 60;

				while (i--)
				{
					std::this_thread::sleep_for(TCLOCK_MILLIS(16)); //~ 60 ticks / s 

					Async::AsyncSystem::TickThread();
				}

				return RSuccess;
			},
			[dPtr = d.Get()](WorkerBase* Self, WorkerGroupShared* Group) mutable
			{
				Async::AsyncSystem::InitializeThread();

				LogInfo("Start!");
				dPtr->DoAfterAsync(1000, &TypeA::DoA, 1);
				dPtr->DoAfterAsync(2000, &TypeA::DoA, 2);
				dPtr->DoAfterAsync(3000, &TypeA::DoA, 3);
				dPtr->DoAfterAsync(4000, &TypeA::DoA, 4);

				dPtr->DoAfterAsync(5000,
					[]()
					{
						LogInfo("L 1()");
					});

				dPtr->DoAfterAsync(6000,
					[]()
					{
						LogInfo("L 2()");
					});

				return RSuccess;
			},
				[](WorkerBase* Self, WorkerGroupShared* Group) noexcept
			{
				Async::AsyncSystem::ShutdownThread();

				return RSuccess;
			});
	}
	RStatus Result = wg.Start();

	wg.JoinGroup();
}

int32_t main(int32_t argc, const char** argv) noexcept
{
	RTRY_S_L(InitializeCommonEx(argc, argv), 1, "Failed to InitializeCommonEx()")
	{
		//on success
	}

	Async::AsyncSystem::InitializeThread();

	AsyncDispatcherUsage(argc, argv);

	Async::AsyncSystem::ShutdownThread();

	MemoryManager::PrintStatistics();

	RTRY_S_L(ShutdownCommonEx(), 2, "Failed to ShutdownCommonEx()")
	{
		//on success
	}

	return 0;
}
