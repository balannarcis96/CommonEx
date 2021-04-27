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
	float c{ 0.0f };

	uint8_t buffer[1024 * 3];

	TypeA() {
		b = (std::rand() % 1000) + 100;
		c = (float)(std::rand() % 50000);

		//LogInfo("TypeA()");
	}
	TypeA(int32_t a) :a(a) {
		b = (std::rand() % 1000) + 100;
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

	thread_local TypeA* array[8] = { nullptr };

	const auto ThreadRoutine = [argc, argv]() {
		int32_t i = 5000;

		auto Start = std::chrono::high_resolution_clock::now();

		//std::vector<TypeA*> vec;
		std::vector<MPtr<TypeA>> vec;
		vec.reserve(5000);

		while (i--) {
			//{
			//	auto* a = new TypeA(argc);
			//
			//	for (size_t j = 0; j < 1024 * 3; j++) {
			//		a->buffer[j] = argc + j;
			//	}
			//
			//	vec.push_back(a);
			//}

			{
				auto a = MakeUniqueManaged<TypeA>(argc);
			
				for (size_t j = 0; j < 1024 * 3; j++) {
					a->buffer[j] = argc + j;
				}
			
				vec.push_back(std::move(a));
			}
		}

		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - Start).count();

		LogInfo("T:DURATION[{}ms]", duration);

		vec.clear();

		//for (auto* t : vec) {
		//	delete t;
		//}

		//from my results the MemoryManager strategy is ~30% faster :D
	};

	std::thread t1, t2, t3;

	t1 = std::thread(ThreadRoutine);
	t2 = std::thread(ThreadRoutine);
	t3 = std::thread(ThreadRoutine);

	t1.join();
	t2.join();
	t3.join();

	MemoryManager::PrintStatistics();

	std::cin.get();

	RTRY_S_L(ShutdownCommonEx(), 2, "Failed to ShutdownCommonEx()") {
		//on success
	}

	return 0;
}
