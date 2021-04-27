#include "../public/Example.h"

std::atomic_bool LogOSAllocs = false;

namespace CommonEx {
	ptr_t GAllocate(size_t BlockSize, size_t BlockAlignment) noexcept {
		if (LogOSAllocs.load()) {
			LogInfo("GAllocate({},{})", BlockSize, BlockAlignment);
		}

		return _aligned_malloc(BlockSize, BlockAlignment);
	}

	void GFree(ptr_t BlockPtr) noexcept {
		if (LogOSAllocs.load()) {
			LogInfo("GFree()");
		}

		_aligned_free(BlockPtr);
	}
}

using namespace CommonEx;

struct TypeA {
	int32_t a{ 4 };
	int32_t b{ 0 };

	TypeA() {
		//LogInfo("TypeA()");
	}
	TypeA(int32_t a) :a(a) {
		//LogInfo("TypeA({})", a);
	}
	~TypeA() {
		//LogInfo("~TypeA()");
	}
};

int32_t main(int32_t argc, const char** argv) noexcept {

	RTRY_S_L(InitializeCommonEx(argc, argv), 1, "Failed to InitializeCommonEx()") {
		//on success
	}

	LogOSAllocs.store(true);

	const auto ThreadRoutine = []() {

		int32_t i = 100000;

		while (i--) {
			{
				auto obj1 = MakeUniqueManaged<TypeA>(22);
				auto obj2 = MakeUniqueManaged<TypeA>();
				auto obj3 = MakeUniqueManaged<TypeA>();
			}

			{
				auto obj1 = MakeSharedManaged<TypeA>(22);
				auto obj2 = MakeSharedManaged<TypeA>();
				auto obj3 = MakeSharedManaged<TypeA>();
			}
		}
	};

	std::thread t1, t2, t3;

	t1 = std::thread(ThreadRoutine);
	t2 = std::thread(ThreadRoutine);
	t3 = std::thread(ThreadRoutine);

	t1.join();
	t2.join();
	t3.join();

	if (MemoryManager::SmallBlock::GetTotalOSAllocations() != 0 ||
		MemoryManager::SmallBlock::GetTotalOSDeallocations() != 0) {
		LogFatal("Something is wrong!");
	}

	MemoryManager::PrintStatistics();

	RTRY_S_L(ShutdownCommonEx(), 2, "Failed to ShutdownCommonEx()") {
		//on success
	}

	return 0;
}
