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
	float c{ 0.0f };

	uint8_t buffer[1024 * 3];

	TypeA() noexcept
	{
		b = (std::rand() % 1000) + 100;
		c = (float)(std::rand() % 50000);

		//LogInfo("TypeA()");
	}
	TypeA(int32_t a)noexcept :a(a)
	{
		b = (std::rand() % 1000) + 100;
		//LogInfo("TypeA({})", a);
	}
	~TypeA() noexcept
	{
		LogInfo("~TypeA()");
	}

	friend bool operator>(const TypeA& Left, const TypeA& Right) noexcept
	{
		return Left.a > Right.a;
	}
};

int32_t main(int32_t argc, const char** argv) noexcept
{

	RTRY_S_L(InitializeCommonEx(argc, argv), 1, "Failed to InitializeCommonEx()")
	{
		//on success
	}

	/*thread_local TypeA* array[8] = { nullptr };

	//const auto ThreadRoutine = [argc, argv]() {
	//	int32_t i = 5000;

	//	auto Start = std::chrono::high_resolution_clock::now();

	//	//std::vector<TypeA*> vec;
	//	std::vector<MPtr<TypeA>> vec;
	//	vec.reserve(5000);

	//	while (i--) {
	//		//{
	//		//	auto* a = new TypeA(argc);
	//		//
	//		//	for (size_t j = 0; j < 1024 * 3; j++) {
	//		//		a->buffer[j] = argc + j;
	//		//	}
	//		//
	//		//	vec.push_back(a);
	//		//}

	//		{
	//			auto a = MakeUniqueManaged<TypeA>(argc);
	//
	//			for (size_t j = 0; j < 1024 * 3; j++) {
	//				a->buffer[j] = argc + j;
	//			}
	//
	//			vec.push_back(std::move(a));
	//		}
	//	}

	//	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - Start).count();

	//	LogInfo("T:DURATION[{}ms]", duration);

	//	vec.clear();

	//	//for (auto* t : vec) {
	//	//	delete t;
	//	//}

	//	//from my results the MemoryManager strategy is ~30% faster :D
	//};

	//std::thread t1, t2, t3;

	//t1 = std::thread(ThreadRoutine);
	//t2 = std::thread(ThreadRoutine);
	//t3 = std::thread(ThreadRoutine);

	//t1.join();
	//t2.join();
	//t3.join();*/

	{
		Async::AsyncTimerTaskPriorityQueue q;
		Async::AsyncDispatcher dq;
		dq.RefCount = 0;
		Async::TaskQueue tq;

		for (int32_t i = 0; i < 100; i++)
		{
			Async::AsyncTaskBase* Task = Async::MakeTaskRaw([i]()
				{
					LogInfo("Task {}", i);
				});

			q.push({ i, Task, &dq });
		}



		//for (int32_t i = 0; i < 100; i++)
		//{
		//	Async::AsyncTaskBase* Task = Async::MakeTaskRaw([i]()
		//		{
		//			LogInfo("Task {}", i);
		//		});
		//
		//	tq.Push(Task);
		//}

		//Async::AsyncTaskBase* Task = tq.Pop();
		//while (Task)
		//{
		//	Task->ExecuteTask();
		//	Task->Destroy(true);
		//	Task = tq.Pop();
		//}

		//WorkerGroup<> wg;
		//
		//{
		//	auto d = MakeSharedManaged<TypeA>(10001);
		//	auto d2 = MakeSharedManaged<TypeA>(10002);
		//
		//	wg.Initialize(2,
		//		[d](WorkerBase* Self, WorkerGroupShared* Group) mutable
		//		{
		//			LogInfo("Run");
		//			return RSuccess;
		//		});
		//}
		//
		//RStatus Result = wg.Start();
		//LogInfo("Start");
		//
		//wg.JoinGroup();
	}

	//{
	//	TQueue<MSharedPtr<TypeA>> queue;
	//
	//	{
	//		for (size_t i = 0; i < 100; i++)
	//		{
	//			auto a = MakeSharedManaged<TypeA>((int32_t)i);
	//
	//			queue.Push(a);
	//		}
	//		MemoryManager::PrintStatistics();
	//	}
	//
	//	{
	//		while (queue.GetSize())
	//		{
	//			LogInfo("queue: {}", queue.Top()->a);
	//			queue.Pop();
	//		}
	//	}
	//}
	MemoryManager::PrintStatistics();

	RTRY_S_L(ShutdownCommonEx(), 2, "Failed to ShutdownCommonEx()")
	{
		//on success
	}

	return 0;
}
