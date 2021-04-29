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

namespace CommonEx
{
	/*------------------------------------------------------------
		Memory Manager
	  ------------------------------------------------------------*/
	class MemoryManager
	{
	public:
		template<typename T, size_t PoolSize>
		using TObjectStore = TObjectPool<T, PoolSize>;
		using TBlockType = MemoryBlockBaseResource;
		using TCustomBlockHeader = CustomBlockHeader<true>;

		class SmallBlock: public MemoryBlock<SmallMemBlockSize, true>, public TObjectStore<SmallBlock, SmallMemBlockCount>
		{
		public:
			SmallBlock(ulong_t ElementSize) noexcept
				: MemoryBlock(ElementSize)
			{}

			SmallBlock(ulong_t ElementSize, ulong_t ElementsCount) noexcept
				: MemoryBlock(ElementSize, ElementsCount)
			{}
		};
		class MediumBlock : public MemoryBlock<MediumMemBlockSize, true>, public TObjectStore<MediumBlock, MediumMemBlockCount>
		{
		public:
			MediumBlock(ulong_t ElementSize) noexcept
				: MemoryBlock(ElementSize)
			{}

			MediumBlock(ulong_t ElementSize, ulong_t ElementsCount) noexcept
				: MemoryBlock(ElementSize, ElementsCount)
			{}
		};
		class LargeBlock : public MemoryBlock<LargeMemBlockSize, true>, public TObjectStore<LargeBlock, LargeMemBlockCount>
		{
		public:
			LargeBlock(ulong_t ElementSize) noexcept
				: MemoryBlock(ElementSize)
			{}

			LargeBlock(ulong_t ElementSize, ulong_t ElementsCount) noexcept
				: MemoryBlock(ElementSize, ElementsCount)
			{}
		};
		class ExtraLargeBlock : public MemoryBlock<ExtraLargeMemBlockSize, true>, public TObjectStore<ExtraLargeBlock, ExtraLargeMemBlockCount>
		{
		public:
			ExtraLargeBlock(ulong_t ElementSize) noexcept
				: MemoryBlock(ElementSize)
			{}

			ExtraLargeBlock(ulong_t ElementSize, ulong_t ElementsCount) noexcept
				: MemoryBlock(ElementSize, ElementsCount)
			{}
		};

		_NODISCARD static RStatus Initialize() noexcept
		{
			R_TRY_L(SmallBlock::Preallocate(), "MemoryManager::SmallBlock Failed to preallocate!") {}
			R_TRY_L(MediumBlock::Preallocate(), "MemoryManager::SmallBlock Failed to preallocate!") {}
			R_TRY_L(LargeBlock::Preallocate(), "MemoryManager::SmallBlock Failed to preallocate!") {}
			R_TRY_L(ExtraLargeBlock::Preallocate(), "MemoryManager::SmallBlock Failed to preallocate!") {}

			return RSuccess;
		}
		_NODISCARD static RStatus Shutdown() noexcept
		{
			return RSuccess;
		}

#ifdef MEMEX_STATISTICS
		static std::atomic<size_t> CustomSizeAllocations;
		static std::atomic<size_t> CustomSizeDeallocations;

		static void PrintStatistics()
		{
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
		_NODISCARD static MSharedPtr<T> AllocShared(Types... Args) noexcept
		{
			if constexpr (std::is_reference_v<T>)
			{
				static_assert(false, "Alloc<T> Cant allocate T reference!");
			}

			if constexpr (std::is_array_v<T>)
			{
				static_assert(false, "Use AllocBuffer(Count) to allocate arrays!");
			}

			constexpr size_t Size = sizeof(T) + alignof(T);

			TBlockType* NewBlockObject = MemoryManager::AllocBlock<T>();
			if (!NewBlockObject)
			{
				return { nullptr , nullptr };
			}

			ptr_t Ptr = NewBlockObject->Block;

			//Align pointer
			size_t Space = Size;
			if (!std::align(alignof(T), sizeof(T), Ptr, Space))
			{
				NewBlockObject->Destroy(false);
				LogFatal("MemoryManager::Alloc(...) Failed to std::align({}, {}, ptr, {})!", alignof(T), sizeof(T), Size);
				return { nullptr , nullptr };
			}

			if constexpr (sizeof...(Types) == 0)
			{
				if constexpr (std::is_default_constructible_v<std::decay<T>::type>)
				{

					//Call default constructor manually
					new (Ptr) T();
				}
			}
			else
			{
				//Call constructor manually
				new (Ptr) T(std::forward<Types>(Args)...);
			}

			return { NewBlockObject, reinterpret_cast<T*>(Ptr) };
		}

		template<typename T, typename ...Types>
		_NODISCARD static MPtr<T> Alloc(Types... Args) noexcept
		{
			if constexpr (std::is_reference_v<T>)
			{
				static_assert(false, "Alloc<T> Cant allocate T reference!");
			}

			if constexpr (std::is_array_v<T>)
			{
				static_assert(false, "Use AllocBuffer(Count) to allocate arrays!");
			}

			constexpr size_t Size = sizeof(T) + alignof(T);

			TBlockType* NewBlockObject = AllocBlock<T>();
			if (!NewBlockObject)
			{
				return { nullptr , nullptr };
			}

			ptr_t Ptr = NewBlockObject->Block;

			//Align pointer
			size_t Space = Size;
			if (!std::align(alignof(T), sizeof(T), Ptr, Space))
			{
				NewBlockObject->Destroy(false);
				LogFatal("MemoryManager::Alloc(...) Failed to std::align({}, {}, ptr, {})!", alignof(T), sizeof(T), Size);
				return { nullptr , nullptr };
			}

			if constexpr (sizeof...(Types) == 0)
			{
				if constexpr (std::is_default_constructible_v<std::decay<T>::type>)
				{
					//Call default constructor manually
					new (Ptr) T();
				}
			}
			else
			{
				//Call constructor manually
				new (Ptr) T(std::forward<Types>(Args)...);
			}

			return { NewBlockObject, reinterpret_cast<T*>(Ptr) };
		}

		template<typename T>
		_NODISCARD static TBlockType* AllocBlock() noexcept
		{
			constexpr size_t Size = sizeof(T) + alignof(T);

			return AllocBlock<Size>();
		}

		template<size_t Size>
		_NODISCARD static TBlockType* AllocBlock() noexcept
		{
			TBlockType* NewBlockObject = nullptr;

			if constexpr (Size <= SmallMemBlockSize)
			{
				NewBlockObject = SmallBlock::NewRaw((ulong_t)Size);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() SmallBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					SmallBlock::Deallocate(reinterpret_cast<SmallBlock*>(NewBlockObject));
				};
			}
			else if constexpr (Size <= MediumMemBlockSize)
			{
				NewBlockObject = MediumBlock::NewRaw((ulong_t)Size);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() MediumBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					MediumBlock::Deallocate(reinterpret_cast<MediumBlock*>(NewBlockObject));
				};
			}
			else if constexpr (Size <= LargeMemBlockSize)
			{
				NewBlockObject = LargeBlock::NewRaw((ulong_t)Size);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() LargeBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					LargeBlock::Deallocate(reinterpret_cast<LargeBlock*>(NewBlockObject));
				};
			}
			else if constexpr (Size <= ExtraLargeMemBlockSize)
			{
				NewBlockObject = ExtraLargeBlock::NewRaw((ulong_t)Size);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() ExtraLargeBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					ExtraLargeBlock::Deallocate(reinterpret_cast<ExtraLargeBlock*>(NewBlockObject));
				};
			}
			else
			{
				NewBlockObject = (TBlockType*)GAllocate(sizeof(CustomBlockHeader) + Size, ALIGNMENT);
				if (NewBlockObject)
				{
					//Construct the CustomBlockHeader at the begining of the block
					new (reinterpret_cast<CustomBlockHeader*>(NewBlockObject)) CustomBlockHeader(Size, Size);

					//Set the destruction handler
					NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
					{
#ifdef MEMEX_STATISTICS
						CustomSizeDeallocations++;
#endif
						GFree(NewBlockObject);
					};

#ifdef MEMEX_STATISTICS
					CustomSizeAllocations++;
#endif
				}
				else
				{
					LogFatal("MemoryManager::Alloc() Failed to get memory from OS!");
					return nullptr;
				}
			}

			return NewBlockObject;
		}

#pragma endregion

#pragma region Runtime

		template<typename T>
		_NODISCARD static MSharedPtr<T> AllocSharedBuffer(size_t Count) noexcept
		{
			MPtr<T> Unique = AllocBuffer<T>(Count);
			if (Unique.IsNull())
			{
				return { nullptr, nullptr };
			}

			T* Ptr = Unique.Get();
			return { Unique.BlockObject.Release(), Ptr };
		}

		// Allocate T[Size] buffer
		// bDontConstructElements - if true the call to T default constructor (for each element) wont be made
		template<typename T, bool bDontConstructElements = false>
		_NODISCARD static MPtr<T> AllocBuffer(const size_t Count) noexcept
		{
			if constexpr (std::is_array_v<T>)
			{
				static_assert(false, "Dont use AllocBuffer<T[]>(size) but use AllocBuffer<T>(size)!");
			}

			TBlockType* NewBlockObject = AllocBlock<T>(Count);
			if (!NewBlockObject)
			{
				return { nullptr , nullptr };
			}

			//if we dont construct, we dont destruct 
			if constexpr (bDontConstructElements && std::is_destructible_v<T>)
			{
				NewBlockObject->bDontDestruct = true;
			}

			//Change to size in bytes
			const size_t Size = (sizeof(T) * Count) + alignof(T);

			ptr_t Ptr = reinterpret_cast<ptr_t>(NewBlockObject->Block);

			//Align pointer
			size_t Space = Size;
			if (!std::align(alignof(T), sizeof(T), Ptr, Space))
			{
				if constexpr (!bDontConstructElements && std::is_destructible_v<T>)
				{
					NewBlockObject->Destroy(false);
				}

				LogFatal("MemoryManager::AllocBuffer({}) Failed to std::align({}, {}, ptr, {})!", Count, alignof(T), sizeof(T), Size);
				return { nullptr , nullptr };
			}

			if constexpr (std::is_default_constructible_v<T> && !bDontConstructElements)
			{
				//Call default constructor manually for each object of the array
				for (size_t i = 0; i < Count; i++)
				{
					new (reinterpret_cast<uint8_t*>(Ptr) + (sizeof(T) * i)) T();
				}
			}

			return { NewBlockObject, reinterpret_cast<T*>(Ptr) };
		}

		template<typename T>
		_NODISCARD static TBlockType* AllocBlock(size_t Count) noexcept
		{
			const size_t Size = (sizeof(T) * Count) + alignof(T);

			TBlockType* NewBlockObject = nullptr;

			if (Size <= SmallMemBlockSize)
			{
				NewBlockObject = SmallBlock::NewRaw((ulong_t)sizeof(T), (ulong_t)Count);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() SmallBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor) noexcept
				{
					MEMORY_MANAGER_CALL_DESTRUCTOR_BUFFER;
					SmallBlock::Deallocate(reinterpret_cast<SmallBlock*>(NewBlockObject));
				};
			}
			else if (Size <= MediumMemBlockSize)
			{
				NewBlockObject = MediumBlock::NewRaw((ulong_t)sizeof(T), (ulong_t)Count);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() MediumBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor) noexcept
				{
					MEMORY_MANAGER_CALL_DESTRUCTOR_BUFFER;
					MediumBlock::Deallocate(reinterpret_cast<MediumBlock*>(NewBlockObject));
				};
			}
			else if (Size <= LargeMemBlockSize)
			{
				NewBlockObject = LargeBlock::NewRaw((ulong_t)sizeof(T), (ulong_t)Count);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() LargeBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)  noexcept
				{
					MEMORY_MANAGER_CALL_DESTRUCTOR_BUFFER;
					LargeBlock::Deallocate(reinterpret_cast<LargeBlock*>(NewBlockObject));
				};
			}
			else if (Size <= ExtraLargeMemBlockSize)
			{
				NewBlockObject = ExtraLargeBlock::NewRaw((ulong_t)sizeof(T), (ulong_t)Count);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() ExtraLargeBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)  noexcept
				{
					MEMORY_MANAGER_CALL_DESTRUCTOR_BUFFER;
					ExtraLargeBlock::Deallocate(reinterpret_cast<ExtraLargeBlock*>(NewBlockObject));
				};
			}
			else
			{
				NewBlockObject = (TBlockType*)GAllocate(sizeof(TCustomBlockHeader) + Size, ALIGNMENT);
				if (NewBlockObject)
				{
					//Construct the CustomBlockHeader at the begining of the block
					new (reinterpret_cast<TCustomBlockHeader*>(NewBlockObject)) TCustomBlockHeader((ulong_t)Size, (ulong_t)sizeof(T), (ulong_t)Count);

					NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor) noexcept
					{
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
				else
				{
					LogFatal("MemoryManager::Alloc() Failed to get memory from OS!");
					return nullptr;
				}
			}

			return NewBlockObject;
		}

		_NODISCARD static TBlockType* AllocBlock(ulong_t SizeInBytes) noexcept
		{
			TBlockType* NewBlockObject = nullptr;

			if (SizeInBytes <= SmallMemBlockSize)
			{
				NewBlockObject = SmallBlock::NewRaw((ulong_t)sizeof(uint8_t), (ulong_t)SizeInBytes);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() SmallBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor) noexcept
				{
					SmallBlock::Deallocate(reinterpret_cast<SmallBlock*>(NewBlockObject));
				};
			}
			else if (SizeInBytes <= MediumMemBlockSize)
			{
				NewBlockObject = MediumBlock::NewRaw((ulong_t)sizeof(uint8_t), (ulong_t)SizeInBytes);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() MediumBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor) noexcept
				{
					MediumBlock::Deallocate(reinterpret_cast<MediumBlock*>(NewBlockObject));
				};
			}
			else if (SizeInBytes <= LargeMemBlockSize)
			{
				NewBlockObject = LargeBlock::NewRaw((ulong_t)sizeof(uint8_t), (ulong_t)SizeInBytes);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() LargeBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)  noexcept
				{
					LargeBlock::Deallocate(reinterpret_cast<LargeBlock*>(NewBlockObject));
				};
			}
			else if (SizeInBytes <= ExtraLargeMemBlockSize)
			{
				NewBlockObject = ExtraLargeBlock::NewRaw((ulong_t)sizeof(uint8_t), (ulong_t)SizeInBytes);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() ExtraLargeBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)  noexcept
				{
					ExtraLargeBlock::Deallocate(reinterpret_cast<ExtraLargeBlock*>(NewBlockObject));
				};
			}
			else
			{
				NewBlockObject = (TBlockType*)GAllocate(sizeof(TCustomBlockHeader) + SizeInBytes, ALIGNMENT);
				if (NewBlockObject)
				{
					//Construct the CustomBlockHeader at the begining of the block
					new (reinterpret_cast<TCustomBlockHeader*>(NewBlockObject)) TCustomBlockHeader((ulong_t)SizeInBytes, (ulong_t)sizeof(uint8_t), (ulong_t)SizeInBytes);

					NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor) noexcept
					{
#ifdef MEMEX_STATISTICS
						CustomSizeDeallocations++;
#endif
						GFree(NewBlockObject);
					};

#ifdef MEMEX_STATISTICS
					CustomSizeAllocations++;
#endif
				}
				else
				{
					LogFatal("MemoryManager::Alloc() Failed to get memory from OS!");
					return nullptr;
				}
			}

			return NewBlockObject;
		}

#pragma endregion
	};

	//Creates an instance of T(args...) by allocating memory from the MemoryManager and wrapps it in a unique pointer object
	template<typename T, typename ...Args>
	_NODISCARD FORCEINLINE MPtr<T> MakeUniqueManaged(Args... args) noexcept
	{
		return std::move(MemoryManager::Alloc<T>(std::forward<Args>(args)...));
	}

	//Creates an instance of T(args...) by allocating memory from the MemoryManager and wrapps it in a shared pointer object
	template<typename T, typename ...Args>
	_NODISCARD FORCEINLINE MSharedPtr<T> MakeSharedManaged(Args... args) noexcept
	{
		return std::move(MemoryManager::AllocShared<T>(std::forward<Args>(args)...));
	}

#pragma region Task implementation

	template<typename Lambda, typename ReturnType, typename ...Args>
	inline ReturnType TaskBase::CallStub(ptr_t* Body, Args... args)noexcept
	{
		constexpr auto Size = sizeof(Lambda);
		if constexpr (Size <= sizeof(MemoryBlockBaseResource*))
		{
			return ((Lambda*)Body)->operator()(std::forward<Args>(args)...);
		}
		else
		{
			return ((Lambda*)(*((MemoryBlockBaseResource**)Body))->Block)->operator()(std::forward<Args>(args)...);
		}
	}

	template<typename Lambda, typename ReturnType, typename ...Args>
	inline void TaskBase::BuildHandler(const Lambda& ByConstRef) noexcept
	{
		constexpr auto Size = sizeof(Lambda);

		if constexpr (Size <= sizeof(MemoryBlockBaseResource*))
		{ //we have sizeof(MemoryBlockBaseResource*) bytes that we can use to store the lambda in
			Body = nullptr; //zerout the memory

			new ((void*)(&Body)) Lambda(ByConstRef); //construct the lambda into the pointer's memory space
		}
		else
		{
			Body = MemoryManager::AllocBlock<sizeof(Lambda)>();
			if (!Body)
			{
				LogFatal("Task<> Failed to allocate body size({})!", sizeof(Lambda));
				abort(); //to dramatic?
			}

			Body->ZeroMemoryBlock(); //zerout the memory

			new ((void*)(Body->Block)) Lambda(ByConstRef); //construct the lambda into the block
		}

		if constexpr (std::is_destructible_v<Lambda>)
		{
			DestroyStub = &DestroyStubFunction<Lambda>;
		}
		else
		{
			DestroyStub = nullptr;
		}

		Handler = (ptr_t)&CallStub<Lambda, ReturnType, Args...>;
	}

	FORCEINLINE void TaskBase::Clear() noexcept
	{
		if (DestroyStub)
		{
			using DestroyHandlerType = void(ptr_t)noexcept;

			((DestroyHandlerType*)DestroyStub)(Body);
		}

		DestroyStub = nullptr;
		Handler = nullptr;

		if (Body->Destroy)
		{
			Body->Destroy(false);
		}
	}

	//Duplicate
	FORCEINLINE TaskBase& TaskBase::operator=(const TaskBase& Other)noexcept
	{
		if (this == &Other)
		{
			return *this;
		}

		if (!Other)
		{
			Clear();
			return *this;
		}

		auto* NewBlock = MemoryManager::AllocBlock(Other.Body->BlockSize);
		if (!NewBlock)
		{
			LogFatal("TaskBase::operator=(const TaskBase&) Failed to allocate new block size:{}", Other.Body->BlockSize);
			return *this;
		}

		Clear();

		if (memcpy_s(
			NewBlock->Block,
			NewBlock->BlockSize,
			Other.Body->Block,
			Other.Body->BlockSize
		))
		{
			NewBlock->Destroy(false);
			LogFatal("TaskBase::operator=(const TaskBase&) Failed to copy into new block size:{}", Other.Body->BlockSize);
			return *this;
		}

		Handler = Other.Handler;
		DestroyStub = Other.DestroyStub;

		return *this;
	}

#pragma endregion
}