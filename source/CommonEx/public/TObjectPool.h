#pragma once
/**
 * @file TObjectPool.h
 *
 * @brief TObjectPool: Ring based thread safe object pool
			bUseSpinLock:
				[true] : SpinLock is used for synchronization [default]
				[false]: Atomic operations are used for synchronization
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

namespace CommonEx {
	template<typename T, size_t PoolSize, bool bUseSpinLock = true>
	class TObjectPool {
	public:
		struct PoolTraits {
			static const size_t MyPoolSize = PoolSize;
			static const size_t MyPoolMask = PoolSize - 1;

			using MyPoolType = T;
			using MyType = TObjectPool<T, PoolSize>;

			static_assert((MyPoolSize& MyPoolMask) == 0, "TObjectPool size must be a power of 2");
		};

#ifdef MEMEX_STATISTICS
		static std::atomic<size_t> TotalAllocations;
		static std::atomic<size_t> TotalDeallocations;

		static std::atomic<size_t> TotalOSAllocations;
		static std::atomic<size_t> TotalOSDeallocations;
#endif

		//Preallocate and fill the whole Pool with [PoolSize] elements
		static RStatus Preallocate() noexcept {
			// ! Hopefully GAllocate will allocate in a continuous fashion.

			for (size_t i = 0; i < PoolSize; i++)
			{
				Pool[i] = GAllocate(sizeof(T), ALIGNMENT);
				if (Pool[i] == nullptr) {
					return RFail;
				}
			}

			return RSuccess;
		}

		//Allocate raw ptr T
		template<typename ...Types>
		_NODISCARD FORCEINLINE static T* NewRaw(Types... Args) noexcept {
			return Allocate(std::forward<Types...>(Args)...);
		}

		//Allocate raw ptr T
		_NODISCARD FORCEINLINE static T* NewRaw() noexcept {
			return Allocate();
		}

		//Deallocate T
		static void Deallocate(T* Obj) noexcept {

			if constexpr (std::is_destructible_v<T>) {
				//Call destructor manually
				Obj->~T();
			}

			ptr_t PrevVal{ nullptr };

			if constexpr (bUseSpinLock) {
				{ //Critical section
					SpinLockScopeGuard Guard(&SpinLock);

					const uint64_t InsPos = TailPosition++;

					PrevVal = Pool[InsPos & PoolTraits::MyPoolMask];
					Pool[InsPos & PoolTraits::MyPoolMask] = (ptr_t)Obj;
				}
			}
			else {
				uint64_t InsPos = (uint64_t)InterlockedIncrement64((volatile long long*)(&TailPosition));
				InsPos--;

				PrevVal = InterlockedExchangePointer(
					reinterpret_cast<volatile ptr_t*>(&Pool[InsPos & PoolTraits::MyPoolMask]),
					reinterpret_cast<ptr_t>(Obj)
				);
			}

			if (PrevVal)
			{
				GFree(PrevVal);

#ifdef MEMEX_STATISTICS
				TotalOSDeallocations++;

				//LogInfo("TObjectPool:: Freed to os!");
#endif

				return;
			}

#ifdef MEMEX_STATISTICS
			TotalDeallocations++;
#endif
		}

		//Get GUID of this Pool instance
		FORCEINLINE static size_t GetPoolId() {
			return (size_t)(&PoolTraits::MyType::Preallocate);
		}

#ifdef MEMEX_STATISTICS
		FORCEINLINE static size_t GetTotalDeallocations() {
			return TotalDeallocations.load();
		}

		FORCEINLINE static size_t GetTotalAllocations() {
			return TotalAllocations.load();
		}

		FORCEINLINE static size_t GetTotalOSDeallocations() {
			return TotalOSDeallocations.load();
		}

		FORCEINLINE static size_t GetTotalOSAllocations() {
			return TotalOSAllocations.load();
		}
#endif

	private:
		template<typename ...Types>
		_NODISCARD static T* Allocate(Types... Args) noexcept {
			T* Allocated{ nullptr };

			if constexpr (bUseSpinLock) {
				{ //Critical section
					SpinLockScopeGuard Guard(&SpinLock);

					const uint64_t PopPos = HeadPosition++;

					Allocated = reinterpret_cast<T*>(Pool[PopPos & PoolTraits::MyPoolMask]);
					Pool[PopPos & PoolTraits::MyPoolMask] = nullptr;
				}
			}
			else {
				uint64_t PopPos = (uint64_t)InterlockedIncrement64(reinterpret_cast<volatile long long*>(&HeadPosition));
				PopPos--;

				Allocated = reinterpret_cast<T*>(InterlockedExchangePointer(
					reinterpret_cast<volatile ptr_t*>(&Pool[PopPos & PoolTraits::MyPoolMask]),
					nullptr
				));
			}

			if (!Allocated) {
				Allocated = (T*)GAllocate(sizeof(T), ALIGNMENT);
				if (!Allocated) {
					return nullptr;
				}

#ifdef MEMEX_STATISTICS
				TotalOSAllocations++;

				//LogInfo("TObjectPool:: Allocated from os!");
#endif
			}

			if constexpr (sizeof...(Types) == 0) {
				if constexpr (std::is_default_constructible_v<T>) {
					//Call default constructor manually
					new (Allocated) T();
				}
			}
			else {
				//Call constructor manually
				new (Allocated) T(std::forward<Types...>(Args)...);
			}

#ifdef MEMEX_STATISTICS
			TotalAllocations++;
#endif

			return Allocated;
		}

		static	inline ptr_t 				Pool[PoolSize]{ 0 };
		static	inline uint64_t  			HeadPosition{ 0 };
		static	inline uint64_t  			TailPosition{ 0 };
		static	inline SpinLock				SpinLock{ };
	};

#ifdef MEMEX_STATISTICS
	template<typename T, size_t PoolSize, bool bUseSpinLock>
	inline std::atomic<size_t> TObjectPool<T, PoolSize, bUseSpinLock>::TotalAllocations;

	template<typename T, size_t PoolSize, bool bUseSpinLock>
	inline std::atomic<size_t> TObjectPool<T, PoolSize, bUseSpinLock>::TotalDeallocations;

	template<typename T, size_t PoolSize, bool bUseSpinLock>
	inline std::atomic<size_t> TObjectPool<T, PoolSize, bUseSpinLock>::TotalOSAllocations;

	template<typename T, size_t PoolSize, bool bUseSpinLock>
	inline std::atomic<size_t> TObjectPool<T, PoolSize, bUseSpinLock>::TotalOSDeallocations;
#endif
}