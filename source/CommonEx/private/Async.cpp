#include "../public/CommonEx.h"

using namespace CommonEx;
using namespace Async;

thread_local AsyncTimerTaskDispatcher* PTR	AsyncSystem::GTimer;
thread_local int64_t						AsyncSystem::GTickCount;
thread_local TExecuterList* PTR				AsyncSystem::GExecuterList;
thread_local AsyncDispatcher* PTR			AsyncSystem::GCurrentExecuterOccupyingThisThread;

RStatus AsyncSystem::InitializeThread() noexcept
{
	//Initialie async system sub parts
	GTimer = new AsyncTimerTaskDispatcher();
	GExecuterList = new TExecuterList();
	GTickCount = 0;

	if (!GTimer || !GExecuterList)
	{
		return RFail;
	}

	GTimer->Initialize();

	Async::AsyncSystem::TickThread();

	return RSuccess;
}

void AsyncSystem::TickThread() noexcept
{
	GTimer->Tick();
}

void AsyncSystem::ShutdownThread() noexcept
{
	delete GTimer;
	GTimer = nullptr;

	delete GExecuterList;
	GExecuterList = nullptr;
}

void AsyncTimerTaskDispatcher::PushTask(uint32_t Time, AsyncTaskBase* Task, AsyncDispatcher* Origin) noexcept
{
	int64_t DueTime = static_cast<int64_t>(Time) + AsyncSystem::GTickCount;

	Origin->AddReference();

	TaskQueue.push(std::move(AsyncTimerTask(DueTime, Task, Origin)));
}

void AsyncTimerTaskDispatcher::Tick() noexcept
{
	AsyncSystem::GTickCount = GetUpTicks();

	while (!TaskQueue.empty())
	{
		const AsyncTimerTask& Task = TaskQueue.top();

		if (AsyncSystem::GTickCount < Task.Time)
		{
			break;
		}

		Task.Origin->Dispatch(Task.Task);

		Task.Origin->ReleaseReferenceAndDestroy();

		Task.Release();

		TaskQueue.pop();
	}
}

AsyncTimerTaskDispatcher::~AsyncTimerTaskDispatcher() noexcept
{
	Shutdown();
}

void AsyncTimerTaskDispatcher::Shutdown() noexcept
{
	while (!TaskQueue.empty())
	{
		TaskQueue.top().Origin->ReleaseReferenceAndDestroy();
		TaskQueue.pop();
	}
}

void AsyncDispatcher::Flush() noexcept
{
	while (true)
	{
		if (AsyncTaskBase* Task = Queue.Pop())
		{
			//If the current dispatcher is not pending destroy execute the task else just skip it
			if (!IsPendingDestroy())
			{
				Task->ExecuteTask();
			}
			else
			{
#if DEBUG_ASYNC_SYSTEM
				const auto TId = std::this_thread::get_id();
				LogWarning("TAsync::Flush() skipped Task By Thread[{}]!", *(int32_t*)&TId);
#endif
			}

			Task->Destroy(true);

			if (RemainingTasksCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				break;
			}
		}
	}
}

void AsyncDispatcher::Dispatch(AsyncTaskBase* Task) noexcept
{
	if (RemainingTasksCount.fetch_add(1, std::memory_order_acq_rel) != 0)
	{
		//There is a consumer present just add the task for it to be dispatched
		Queue.Push(Task);
	}
	else
	{
		//We are the new consumer for this AsyncDispatcher instance, do all tasks available

		//Register task for the dispatcher
		Queue.Push(Task);

		//Inc ref count for this object
		AddReference();

		//The following is necessary because any executed task can do DoAsync themselfs and so will cause this function to behave recursively 
		//so we keep the execution stack "flat"

		//Does any dispatcher exist occupying this worker-thread at this moment?
		if (AsyncSystem::GCurrentExecuterOccupyingThisThread != nullptr)
		{
			AsyncSystem::GExecuterList->Push(this);
		}
		else
		{
			//Accquire
			AsyncSystem::GCurrentExecuterOccupyingThisThread = this;

			//Invoke all tasks of this dispatcher
			Flush();

			//Invoke all tasks of other dispatchers registered in this thread
			while (AsyncSystem::GExecuterList->GetSize())
			{
				AsyncDispatcher* Dispatcher = AsyncSystem::GExecuterList->Top();
				AsyncSystem::GExecuterList->Pop();

				Dispatcher->Flush();
				Dispatcher->ReleaseReferenceAndDestroy();
			}

			//Release
			AsyncSystem::GCurrentExecuterOccupyingThisThread = nullptr;
			ReleaseReferenceAndDestroy();
		}
	}
}
