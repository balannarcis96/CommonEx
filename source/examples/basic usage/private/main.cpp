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

struct TypeA {
	int32_t a{ 4 };
	int32_t b{ 0 };

	TypeA() {
		LogInfo("TypeA()");
	}
	TypeA(int32_t a) :a(a) {
		LogInfo("TypeA({})", a);
	}
	~TypeA(){
		LogInfo("~TypeA()");
	}
};


int32_t main(int32_t argc, const char** argv) noexcept {

	RTRY_S_L(InitializeCommonEx(argc, argv), 1, "Failed to InitializeCommonEx()") {
		//on success
	}

	{
		MPtr<TypeA> o1 = MakeUniqueManaged<TypeA>();
		auto o2 = MakeUniqueManaged<TypeA>(55);

		auto o3 = MakeSharedManaged<TypeA>();



	}

	{
		auto o1 = MakeUnique<TypeA>();
		auto o2 = MakeUnique<TypeA>(55);
	}

	MemoryManager::PrintStatistics();
	
	system("pause");

	RTRY_S_L(ShutdownCommonEx(), 2, "Failed to ShutdownCommonEx()") {
		//on success
	}

	return 0;
}
