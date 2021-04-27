#include "../public/Example.h"

namespace CommonEx {
	ptr_t GAllocate(size_t BlockSize, size_t BlockAlignment) noexcept {
		return _aligned_malloc(BlockSize, BlockAlignment);
	}

	void GFree(ptr_t BlockPtr) noexcept {
		_aligned_free(BlockPtr);
	}
}


int32_t main(int32_t argc, const char** argv) noexcept {

	RTRY_S_L(InitializeCommonEx(argc, argv), 1, "Failed to InitializeCommonEx()") {
		//on success
	}





	MemoryManager::PrintStatistics();

	system("pause");

	RTRY_S_L(ShutdownCommonEx(), 2, "Failed to ShutdownCommonEx()") {
		//on success
	}

	return 0;
}
