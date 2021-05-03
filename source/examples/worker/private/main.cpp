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
	auto work = MakeUniqueManaged<AnyWorkAsync<>>(std::move([](AnyWorkAsync<>* Self, RStatus Result) mutable noexcept
		{
			LogInfo("AsyncWork!!!");
		}));

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
	//		                 <12> - Task body size (lambda capture body max size) = 32bytes
	//WorkerGroupWithMainThread<12> Group;
	WorkerGroup<12>	Group;

	constexpr auto t = sizeof(Group);
	constexpr auto t2 = sizeof(WorkerGroup<12>::TMyWorker);

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

int32_t main(int32_t argc, const char** argv) noexcept
{

	RTRY_S_L(InitializeCommonEx(argc, argv), 1, "Failed to InitializeCommonEx()")
	{
		//on success
	}

	{
		//WorkExample();

		WorkersExample();
	}

	MemoryManager::PrintStatistics();

	RTRY_S_L(ShutdownCommonEx(), 2, "Failed to ShutdownCommonEx()")
	{
		//on success
	}

	return 0;
}
