#pragma once
/**
 * @file MemoryManager.h
 *
 * @brief CommonEx MemoryManager implementation
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

namespace CommonEx {
	/*------------------------------------------------------------
		Memory Manager
	  ------------------------------------------------------------*/
	class MemoryManager {
	public:
		template<typename T, size_t PoolSize>
		using TObjectStore = TObjectPool<T, PoolSize>;

		struct SmallBlock : MemoryBlock<SmallMemBlockSize>, TObjectStore<SmallBlock, SmallMemBlockCount> {
			SmallBlock(ulong_t ElementSize) noexcept
				: MemoryBlock(ElementSize)
			{}

			SmallBlock(ulong_t ElementSize, ulong_t ElementsCount) noexcept
				: MemoryBlock(ElementSize, ElementsCount)
			{}
		};
		struct MediumBlock : MemoryBlock<MediumMemBlockSize>, TObjectStore<MediumBlock, MediumMemBlockCount> {
			MediumBlock(ulong_t ElementSize) noexcept
				: MemoryBlock(ElementSize)
			{}

			MediumBlock(ulong_t ElementSize, ulong_t ElementsCount) noexcept
				: MemoryBlock(ElementSize, ElementsCount)
			{}
		};
		struct LargeBlock : MemoryBlock<LargeMemBlockSize>, TObjectStore<LargeBlock, LargeMemBlockCount> {
			LargeBlock(ulong_t ElementSize) noexcept
				: MemoryBlock(ElementSize)
			{}

			LargeBlock(ulong_t ElementSize, ulong_t ElementsCount) noexcept
				: MemoryBlock(ElementSize, ElementsCount)
			{}
		};
		struct ExtraLargeBlock : MemoryBlock<ExtraLargeMemBlockSize>, TObjectStore<ExtraLargeBlock, ExtraLargeMemBlockCount> {
			ExtraLargeBlock(ulong_t ElementSize) noexcept
				: MemoryBlock(ElementSize)
			{}

			ExtraLargeBlock(ulong_t ElementSize, ulong_t ElementsCount) noexcept
				: MemoryBlock(ElementSize, ElementsCount)
			{}
		};

		_NODISCARD static RStatus Initialize() noexcept {
			R_TRY_L(SmallBlock::Preallocate(), "MemoryManager::SmallBlock Failed to preallocate!") {}
			R_TRY_L(MediumBlock::Preallocate(), "MemoryManager::SmallBlock Failed to preallocate!") {}
			R_TRY_L(LargeBlock::Preallocate(), "MemoryManager::SmallBlock Failed to preallocate!") {}
			R_TRY_L(ExtraLargeBlock::Preallocate(), "MemoryManager::SmallBlock Failed to preallocate!") {}

			return RSuccess;
		}
		_NODISCARD static RStatus Shutdown() noexcept {
			return RSuccess;
		}

#ifdef MEMEX_STATISTICS
		static std::atomic<size_t> CustomSizeAllocations;
		static std::atomic<size_t> CustomSizeDeallocations;

		static void PrintStatistics() {
			LogInfo("MemoryManager ###############################################################\n");
			LogInfo("\n\tSmallBlock:\n\t\tAllocations:{}\n\t\tDeallocations:{}\n\t\tOSAllocations:{}\n\t\tOSDeallocations:{}",
				SmallBlock::GetTotalAllocations(),
				SmallBlock::GetTotalDeallocations(),
				SmallBlock::GetTotalOSAllocations(),
				SmallBlock::GetTotalOSDeallocations()
			);
			LogInfo("\n\tMediumBlock:\n\t\tAllocations:{}\n\t\tDeallocations:{}\n\t\tOSAllocations:{}\n\t\tOSDeallocations:{}",
				MediumBlock::GetTotalAllocations(),
				MediumBlock::GetTotalDeallocations(),
				MediumBlock::GetTotalOSAllocations(),
				MediumBlock::GetTotalOSDeallocations()
			);
			LogInfo("\n\tLargeBlock:\n\t\tAllocations:{}\n\t\tDeallocations:{}\n\t\tOSAllocations:{}\n\t\tOSDeallocations:{}",
				LargeBlock::GetTotalAllocations(),
				LargeBlock::GetTotalDeallocations(),
				LargeBlock::GetTotalOSAllocations(),
				LargeBlock::GetTotalOSDeallocations()
			);
			LogInfo("\n\tExtraLargeBlock:\n\t\tAllocations:{}\n\t\tDeallocations:{}\n\t\tOSAllocations:{}\n\t\tOSDeallocations:{}",
				ExtraLargeBlock::GetTotalAllocations(),
				ExtraLargeBlock::GetTotalDeallocations(),
				ExtraLargeBlock::GetTotalOSAllocations(),
				ExtraLargeBlock::GetTotalOSDeallocations()
			);
			LogInfo("\n\tCustomSize(OS Blocks):\n\t\tAllocations:{}\n\t\tDeallocations:{}",
				CustomSizeAllocations.load(),
				CustomSizeDeallocations.load()
			);
			LogInfo("\n\tTotal Allocation:{}\n\tTotal Deallocations:{}\n\tTotal OSAllocations:{}\n\tTotal OSDeallocations:{}",
				SmallBlock::GetTotalAllocations() + MediumBlock::GetTotalAllocations() + LargeBlock::GetTotalAllocations() + ExtraLargeBlock::GetTotalAllocations() + CustomSizeAllocations.load(),
				SmallBlock::GetTotalDeallocations() + MediumBlock::GetTotalDeallocations() + LargeBlock::GetTotalDeallocations() + ExtraLargeBlock::GetTotalDeallocations() + CustomSizeDeallocations.load(),
				SmallBlock::GetTotalOSAllocations() + MediumBlock::GetTotalOSAllocations() + LargeBlock::GetTotalOSAllocations() + ExtraLargeBlock::GetTotalOSAllocations(),
				SmallBlock::GetTotalOSDeallocations() + MediumBlock::GetTotalOSDeallocations() + LargeBlock::GetTotalOSDeallocations() + ExtraLargeBlock::GetTotalOSDeallocations()
			);
			LogInfo("MemoryManager ###############################################################\n");
		}
#endif

#undef MEMORY_MANAGER_CALL_DESTRUCTOR
#define MEMORY_MANAGER_CALL_DESTRUCTOR																		\
			IMemoryBlock* NewBlockObject = (IMemoryBlock*)Object; 											\
			if constexpr (std::is_destructible_v<T>) {														\
				if(bCallDestructor && !NewBlockObject->bDontDestruct)	{									\
					auto Ptr = reinterpret_cast<ptr_t>(NewBlockObject->Block);								\
																											\
					size_t Space = sizeof(T) + alignof(T);													\
					std::align(alignof(T), sizeof(T), Ptr, Space);											\
																											\
					/*call destructor*/																		\
					reinterpret_cast<T*>(Ptr)->~T();														\
				}																							\
			}

#undef MEMORY_MANAGER_CALL_DESTRUCTOR_BUFFER
#define MEMORY_MANAGER_CALL_DESTRUCTOR_BUFFER																\
			IMemoryBlock* NewBlockObject = (IMemoryBlock*)Object; 											\
			if constexpr (std::is_destructible_v<T>) {														\
				if(bCallDestructor && !NewBlockObject->bDontDestruct)	{									\
					auto Ptr = reinterpret_cast<ptr_t>(NewBlockObject->Block);								\
																											\
					size_t Space = NewBlockObject->BlockSize;												\
					std::align(alignof(T), sizeof(T), Ptr, Space);											\
																											\
					for(size_t i = 0; i < NewBlockObject->ElementsCount; i ++) {							\
						/*call destructor*/																	\
						T* IPtr = reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(Ptr) + (sizeof(T) * i));	\
						/*call destructor*/																	\
						IPtr->~T();																			\
					}																						\
				}																							\
			}

#pragma region Compiletime

		template<typename T, typename ...Types>
		_NODISCARD static MSharedPtr<T> AllocShared(Types... Args) noexcept {
			if constexpr (std::is_reference_v<T>) {
				static_assert(false, "Alloc<T> Cant allocate T reference!");
			}

			if constexpr (std::is_array_v<T>) {
				static_assert(false, "Use AllocBuffer(Count) to allocate arrays!");
			}

			constexpr size_t Size = sizeof(T) + alignof(T);

			MemoryBlockBase* NewBlockObject = MemoryManager::AllocBlock<T>();
			if (!NewBlockObject) {
				return { nullptr , nullptr };
			}

			ptr_t Ptr = NewBlockObject->Block;

			//Align pointer
			size_t Space = Size;
			if (!std::align(alignof(T), sizeof(T), Ptr, Space)) {
				NewBlockObject->Destroy((ptr_t)NewBlockObject, false);
				LogFatal("MemoryManager::Alloc(...) Failed to std::align({}, {}, ptr, {})!", alignof(T), sizeof(T), Size);
				return { nullptr , nullptr };
			}

			if constexpr (sizeof...(Types) == 0) {
				if constexpr (std::is_default_constructible_v<std::decay<T>::type>) {

					//Call default constructor manually
					new (Ptr) T();
				}
			}
			else {
				//Call constructor manually
				new (Ptr) T(std::forward<Types...>(Args)...);
			}

			return { NewBlockObject, reinterpret_cast<T*>(Ptr) };
		}

		template<typename T, typename ...Types>
		_NODISCARD static MPtr<T> Alloc(Types... Args) noexcept {
			if constexpr (std::is_reference_v<T>) {
				static_assert(false, "Alloc<T> Cant allocate T reference!");
			}

			if constexpr (std::is_array_v<T>) {
				static_assert(false, "Use AllocBuffer(Count) to allocate arrays!");
			}

			constexpr size_t Size = sizeof(T) + alignof(T);

			MemoryBlockBase* NewBlockObject = AllocBlock<T>();
			if (!NewBlockObject) {
				return { nullptr , nullptr };
			}

			ptr_t Ptr = NewBlockObject->Block;

			//Align pointer
			size_t Space = Size;
			if (!std::align(alignof(T), sizeof(T), Ptr, Space)) {
				NewBlockObject->Destroy((ptr_t)NewBlockObject, false);
				LogFatal("MemoryManager::Alloc(...) Failed to std::align({}, {}, ptr, {})!", alignof(T), sizeof(T), Size);
				return { nullptr , nullptr };
			}

			if constexpr (sizeof...(Types) == 0) {
				if constexpr (std::is_default_constructible_v<std::decay<T>::type>) {

					//Call default constructor manually
					new (Ptr) T();
				}
			}
			else {
				//Call constructor manually
				new (Ptr) T(std::forward<Types...>(Args)...);
			}

			return { NewBlockObject, reinterpret_cast<T*>(Ptr) };
		}

		template<typename T>
		_NODISCARD static IMemoryBlock* AllocBlock() noexcept {
			constexpr size_t Size = sizeof(T) + alignof(T);

			IMemoryBlock* NewBlockObject = nullptr;

			if constexpr (Size <= SmallMemBlockSize)
			{
				NewBlockObject = SmallBlock::NewRaw((ulong_t)Size);
				if (!NewBlockObject) {
					LogFatal("MemoryManager::Alloc() SmallBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [](ptr_t Object, bool bCallDestructor = true)  -> void {
					MEMORY_MANAGER_CALL_DESTRUCTOR;
					SmallBlock::Deallocate(reinterpret_cast<SmallBlock*>(NewBlockObject));
				};
			}
			else if constexpr (Size <= MediumMemBlockSize)
			{
				NewBlockObject = MediumBlock::NewRaw((ulong_t)Size);
				if (!NewBlockObject) {
					LogFatal("MemoryManager::Alloc() MediumBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [](ptr_t Object, bool bCallDestructor = true) {
					MEMORY_MANAGER_CALL_DESTRUCTOR;
					MediumBlock::Deallocate(reinterpret_cast<MediumBlock*>(NewBlockObject));
				};
			}
			else if constexpr (Size <= LargeMemBlockSize)
			{
				NewBlockObject = LargeBlock::NewRaw((ulong_t)Size);
				if (!NewBlockObject) {
					LogFatal("MemoryManager::Alloc() LargeBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [](ptr_t Object, bool bCallDestructor = true) {
					MEMORY_MANAGER_CALL_DESTRUCTOR;
					LargeBlock::Deallocate(reinterpret_cast<LargeBlock*>(NewBlockObject));
				};
			}
			else if constexpr (Size <= ExtraLargeMemBlockSize)
			{
				NewBlockObject = ExtraLargeBlock::NewRaw((ulong_t)Size);
				if (!NewBlockObject) {
					LogFatal("MemoryManager::Alloc() ExtraLargeBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [](ptr_t Object, bool bCallDestructor = true) {
					MEMORY_MANAGER_CALL_DESTRUCTOR;
					ExtraLargeBlock::Deallocate(reinterpret_cast<ExtraLargeBlock*>(NewBlockObject));
				};
			}
			else {
				NewBlockObject = (IMemoryBlock*)GAllocate(sizeof(CustomBlockHeader) + Size, ALIGNMENT);
				if (NewBlockObject)
				{
					//Construct the CustomBlockHeader at the begining of the block
					new (reinterpret_cast<CustomBlockHeader*>(NewBlockObject)) CustomBlockHeader(Size, Size);

					//Set the destruction handler
					NewBlockObject->Destroy = [](ptr_t Object, bool bCallDestructor = true) {
						MEMORY_MANAGER_CALL_DESTRUCTOR;

#ifdef MEMEX_STATISTICS
						CustomSizeDeallocations++;
#endif
						GFree(NewBlockObject);
					};

#ifdef MEMEX_STATISTICS
					CustomSizeAllocations++;
#endif
				}
				else {
					LogFatal("MemoryManager::Alloc() Failed to get memory from OS!");
					return nullptr;
				}
			}

			return NewBlockObject;
		}

#pragma endregion

#pragma region Runtime

		template<typename T>
		_NODISCARD static MSharedPtr<T> AllocSharedBuffer(size_t Count) noexcept {
			MPtr<T> Unique = AllocBuffer<T>(Count);
			if (Unique.IsNull()) {
				return { nullptr, nullptr };
			}

			T* Ptr = Unique.Get();
			return { Unique.BlockObject.Release(), Ptr };
		}

		// Allocate T[Size] buffer
		// bDontConstructElements - if true the call to T default constructor (for each element) wont be made
		template<typename T, bool bDontConstructElements = false>
		_NODISCARD static MPtr<T> AllocBuffer(const size_t Count) noexcept {
			if constexpr (std::is_array_v<T>) {
				static_assert(false, "Dont use AllocBuffer<T[]>(size) but use AllocBuffer<T>(size)!");
			}

			IMemoryBlock* NewBlockObject = AllocBlock<T>(Count);
			if (!NewBlockObject) {
				return { nullptr , nullptr };
			}

			//if we dont construct, we dont destruct 
			if constexpr (bDontConstructElements && std::is_destructible_v<T>) {
				NewBlockObject->bDontDestruct = true;
			}

			//Change to size in bytes
			const size_t Size = (sizeof(T) * Count) + alignof(T);

			ptr_t Ptr = reinterpret_cast<ptr_t>(NewBlockObject->Block);

			//Align pointer
			size_t Space = Size;
			if (!std::align(alignof(T), sizeof(T), Ptr, Space)) {
				if constexpr (!bDontConstructElements && std::is_destructible_v<T>) {
					NewBlockObject->Destroy((ptr_t)NewBlockObject, false);
				}

				LogFatal("MemoryManager::AllocBuffer({}) Failed to std::align({}, {}, ptr, {})!", Count, alignof(T), sizeof(T), Size);
				return { nullptr , nullptr };
			}

			if constexpr (std::is_default_constructible_v<T> && !bDontConstructElements) {
				//Call default constructor manually for each object of the array
				for (size_t i = 0; i < Count; i++)
				{
					new (reinterpret_cast<uint8_t*>(Ptr) + (sizeof(T) * i)) T();
				}
			}

			return { NewBlockObject, reinterpret_cast<T*>(Ptr) };
		}

		template<typename T>
		_NODISCARD static IMemoryBlock* AllocBlock(size_t Count) noexcept {
			const size_t Size = (sizeof(T) * Count) + alignof(T);

			IMemoryBlock* NewBlockObject = nullptr;

			if (Size <= SmallMemBlockSize)
			{
				NewBlockObject = SmallBlock::NewRaw((ulong_t)sizeof(T), (ulong_t)Count);
				if (!NewBlockObject) {
					LogFatal("MemoryManager::Alloc() SmallBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [](ptr_t Object, bool bCallDestructor = true) {
					MEMORY_MANAGER_CALL_DESTRUCTOR_BUFFER;
					SmallBlock::Deallocate(reinterpret_cast<SmallBlock*>(NewBlockObject));
				};
			}
			else if (Size <= MediumMemBlockSize)
			{
				NewBlockObject = MediumBlock::NewRaw((ulong_t)sizeof(T), (ulong_t)Count);
				if (!NewBlockObject) {
					LogFatal("MemoryManager::Alloc() MediumBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [](ptr_t Object, bool bCallDestructor = true) {
					MEMORY_MANAGER_CALL_DESTRUCTOR_BUFFER;
					MediumBlock::Deallocate(reinterpret_cast<MediumBlock*>(NewBlockObject));
				};
			}
			else if (Size <= LargeMemBlockSize)
			{
				NewBlockObject = LargeBlock::NewRaw((ulong_t)sizeof(T), (ulong_t)Count);
				if (!NewBlockObject) {
					LogFatal("MemoryManager::Alloc() LargeBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [](ptr_t Object, bool bCallDestructor = true) {
					MEMORY_MANAGER_CALL_DESTRUCTOR_BUFFER;
					LargeBlock::Deallocate(reinterpret_cast<LargeBlock*>(NewBlockObject));
				};
			}
			else if (Size <= ExtraLargeMemBlockSize)
			{
				NewBlockObject = ExtraLargeBlock::NewRaw((ulong_t)sizeof(T), (ulong_t)Count);
				if (!NewBlockObject) {
					LogFatal("MemoryManager::Alloc() ExtraLargeBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [](ptr_t Object, bool bCallDestructor = true) {
					MEMORY_MANAGER_CALL_DESTRUCTOR_BUFFER;
					ExtraLargeBlock::Deallocate(reinterpret_cast<ExtraLargeBlock*>(NewBlockObject));
				};
			}
			else {
				NewBlockObject = (IMemoryBlock*)GAllocate(sizeof(CustomBlockHeader) + Size, ALIGNMENT);
				if (NewBlockObject)
				{
					//Construct the CustomBlockHeader at the begining of the block
					new (reinterpret_cast<CustomBlockHeader*>(NewBlockObject)) CustomBlockHeader((ulong_t)Size, (ulong_t)sizeof(T), (ulong_t)Count);

					NewBlockObject->Destroy = [](ptr_t Object, bool bCallDestructor = true) {
						MEMORY_MANAGER_CALL_DESTRUCTOR_BUFFER;

#ifdef MEMEX_STATISTICS
						CustomSizeDeallocations++;
#endif
						GFree(NewBlockObject);
					};

#ifdef MEMEX_STATISTICS
					CustomSizeAllocations++;
#endif
				}
				else {
					LogFatal("MemoryManager::Alloc() Failed to get memory from OS!");
					return nullptr;
				}
			}

			return NewBlockObject;
		}

#pragma endregion
	};

	template<typename TUpper>
	class IResource {
	public:
		template<typename ...Types>
		_NODISCARD FORCEINLINE static MPtr<TUpper> New(Types... Args) noexcept {
			return MemoryManager::Alloc<TUpper>(std::forward<Types...>(Args)...);
		}

		template<typename ...Types>
		_NODISCARD FORCEINLINE static MSharedPtr<TUpper> NewShared(Types... Args) noexcept {
			return MemoryManager::AllocShared<TUpper>(std::forward<Types...>(Args)...);
		}

		_NODISCARD FORCEINLINE static MPtr<TUpper> NewArray(size_t Count) noexcept {
			return MemoryManager::AllocBuffer<TUpper>(Count);
		}

		_NODISCARD FORCEINLINE static MSharedPtr<TUpper> NewSharedArray(size_t Count) noexcept {
			return MemoryManager::AllocSharedBuffer<TUpper>(Count);
		}
	};

	//Creates an instance of T(args...) by allocating memory from the MemoryManager and wrapps it in a unique pointer object
	template<typename T, typename ...Args>
	_NODISCARD FORCEINLINE MPtr<T> MakeUniqueManaged(Args... args) noexcept {
		return std::move(MemoryManager::Alloc<T>(std::forward<Args...>(args)...));
	}

	//Creates an instance of T(args...) by allocating memory from the MemoryManager and wrapps it in a shared pointer object
	template<typename T, typename ...Args>
	_NODISCARD FORCEINLINE MSharedPtr<T> MakeSharedManaged(Args... args) noexcept {
		return std::move(MemoryManager::AllocShared<T>(std::forward<Args...>(args)...));
	}
}