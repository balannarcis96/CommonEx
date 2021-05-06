#include "../public/Example.h"

namespace CommonEx
{
	ptr_t GAllocate(size_t BlockSize, size_t BlockAlignment) noexcept
	{
		MemoryManager::GAllocateCount++;

		return _aligned_malloc(BlockSize, BlockAlignment);
	}

	void GFree(ptr_t BlockPtr) noexcept
	{
		MemoryManager::GFreeCount++;

		_aligned_free(BlockPtr);
	}
}

using namespace CommonEx;

struct TypeA
{
	int32_t a{ 4 };
	int32_t b{ 0 };

	TypeA()
	{
		LogInfo("TypeA()");
	}
	TypeA(int32_t a) :a(a)
	{
		LogInfo("TypeA({})", a);
	}
	~TypeA()
	{
		LogInfo("~TypeA()");
	}
};

class MyWork : public AsyncWork<MyWork, std::string, 32> {};

RStatus WorkExample()
{
	auto work = MakeUniqueManaged<AnyWorkAsync<>>([](AnyWorkAsync<>* Self, RStatus Result) mutable noexcept
		{
			LogInfo("AsyncWork!!!");
		});

	auto work2 = MakeUniqueManaged<MyWork>();
	work2->SetCompletionHandler([](MyWork* self, std::string* Payload, RStatus Result)
		{
			LogInfo("MyAsyncWork!!!");
		});

	//Post work item into the async system to be processed by workers
	//	async->DoAsync(std::move(work));
	//or
	//	R_TRY(async->DoAsync(work)){ work.Release(); }

	{
		//on worker
		work->CompleteWork(RSuccess, 0);
		work2->CompleteWork(RSuccess, 0);
	}

	return RSuccess;
}

RStatus WorkersExample()
{
	//		                 <12> - Task body size (lambda capture body max size) = 12bytes
	//WorkerGroupWithMainThread<12> Group;
	WorkerGroup<12>	Group;

	RStatus Result = Group.Initialize(
		2 //2 workers in this group
		, [/* capture body can be max 12bytes in size */](WorkerBase* Worker, WorkerGroupShared* Group) mutable noexcept
		{

			LogInfo("Worker<>::OnRun");

			return RSuccess;
		});
	if (Result != RSuccess)
	{
		return RFail;
	}

	Result = Group.Start();
	if (Result != RSuccess)
	{
		return RFail;
	}

	return RSuccess;
}

class FrontEndController : public ServerController<true>
{
	using Base = ServerController;
public:
	RStatus Initialize() noexcept
	{
		StartTask = [this]() noexcept
		{
			LogInfo("ForntEnd StartTask");
			return RSuccess;
		};

		StopTask = [this]()noexcept
		{
			LogInfo("ForntEnd StopTask");
		};

		WorkerRoutineTask = [this](WorkerBase* Worker, WorkerGroupShared* Group) noexcept
		{
			LogInfo("ForntEnd WorkerRoutineTask");

			return RSuccess;
		};

		WorkerShutdownTask = [this](WorkerBase* Worker, WorkerGroupShared* Group) noexcept
		{
			LogInfo("ForntEnd WorkerShutdownTask");
			return RSuccess;
		};

		WorkerGroupShutdownTask = [this](WorkerGroupShared* Group) noexcept
		{
			LogInfo("ForntEnd WorkerGroupShutdownTask");
		};

		R_TRY_L(ServerController::Initialize(2, 0, 10001, [this](TSocket Socket, sockaddr_in* Info) noexcept
			{
				return RSuccess;
			}), "")
		{}

		return RSuccess;
	}
};

int32_t main(int32_t argc, const char** argv) noexcept
{
	RTRY_S_L(InitializeCommonEx(argc, argv), 1, "Failed to InitializeCommonEx()")
	{
		//on success
	}

	{
		//WorkExample();

		//WorkersExample();
		{
			FrontEndController c;

			c.Initialize();

			c.Start();
		}

		{
			FrontEndController c;

			c.Initialize();

			c.Start();
		}
	}

	MemoryManager::PrintStatistics();

	RTRY_S_L(ShutdownCommonEx(), 2, "Failed to ShutdownCommonEx()")
	{
		//on success
	}

	return 0;
}
