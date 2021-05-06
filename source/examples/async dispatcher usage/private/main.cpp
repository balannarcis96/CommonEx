#include "../public/Example.h"

namespace CommonEx {
	ptr_t GAllocate(size_t BlockSize, size_t BlockAlignment) noexcept {
		return _aligned_malloc(BlockSize, BlockAlignment);
	}

	void GFree(ptr_t BlockPtr) noexcept {
		_aligned_free(BlockPtr);
	}
}
using namespace CommonEx;

struct TypeA
{
	void DoA() noexcept
	{
		LogInfo("From Task 2!!");
	}
};


int32_t main(int32_t argc, const char** argv) noexcept {

	RTRY_S_L(InitializeCommonEx(argc, argv), 1, "Failed to InitializeCommonEx()") {
		//on success
	}

	long long a = argc;

	TypeA A;

	{
		Async::AsyncTask<16> T1 = [a, argc, argv]() noexcept
		{
			LogInfo("From Task!!");
		};

		auto T2 = Async::AsyncMemberFunctionTask(&A , &TypeA::DoA);

		constexpr auto T1Size = sizeof(T1);

		int i = 1000;
		while (i-- > 0)
		{
			auto T3 = Async::MakeTask([a, argc, argv]() noexcept
				{
					LogInfo("From Task 3!!");
				});

			T1.ExecuteTask();
			T2.ExecuteTask();

			if (T3)
			{
				T3->ExecuteTask();
			}
		}
	}

	MemoryManager::PrintStatistics();
	
	RTRY_S_L(ShutdownCommonEx(), 2, "Failed to ShutdownCommonEx()") {
		//on success
	}

	return 0;
}
