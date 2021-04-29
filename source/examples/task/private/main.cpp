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
	int a{ 0 };
	TypeA() { LogInfo("TypeA()"); }
	~TypeA() { LogInfo("~TypeA()"); }

	void operator()()
	{
		LogInfo("Yes2");
	}
};

int32_t main(int32_t argc, const char** argv) noexcept
{

	RTRY_S_L(InitializeCommonEx(argc, argv), 1, "Failed to InitializeCommonEx()")
	{
		//on success
	}

	{
		TPtr<TypeA> a{ nullptr };

		{
			_TaskEx<16, void(int)> T2{ [](int) { LogInfo("T2()"); } };
			_TaskEx<16, void(int)> T1;

			T1 = T2;

			T1(23);

			{
				auto d = MakeUnique<TypeA>();

				//we move the ownershit of d into the task body
				T1 = [d{ std::move(d) }, &a](int i) mutable
				{
					// move the ownership of d into a
					a.reset(d.release());

					//if we dont move ownership of d into a , d will be destroyed with the task and freed (as expected!)

					LogInfo("T1()");
				};

				LogInfo("### Out Of Scope ###");
			}

			T1(23);

			LogInfo("### Out Of Scope 2 ###");
		}

		LogInfo("### Out Of Scope 3 ###");
	}

	MemoryManager::PrintStatistics();

	RTRY_S_L(ShutdownCommonEx(), 2, "Failed to ShutdownCommonEx()")
	{
		//on success
	}

	return 0;
}
