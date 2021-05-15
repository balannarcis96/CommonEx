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

		class Tiny1Block : public MemoryBlock<Tiny1MemBlockSize, true>, public TObjectStore<Tiny1Block, Tiny1MemBlockCount>
		{
		public:
			Tiny1Block(ulong_t ElementSize) noexcept
				: MemoryBlock(ElementSize)
			{}

			Tiny1Block(ulong_t ElementSize, ulong_t ElementsCount) noexcept
				: MemoryBlock(ElementSize, ElementsCount)
			{}
		};
		class Tiny2Block : public MemoryBlock<Tiny2MemBlockSize, true>, public TObjectStore<Tiny2Block, Tiny2MemBlockCount>
		{
		public:
			Tiny2Block(ulong_t ElementSize) noexcept
				: MemoryBlock(ElementSize)
			{}

			Tiny2Block(ulong_t ElementSize, ulong_t ElementsCount) noexcept
				: MemoryBlock(ElementSize, ElementsCount)
			{}
		};
		class Tiny3Block : public MemoryBlock<Tiny3MemBlockSize, true>, public TObjectStore<Tiny3Block, Tiny3MemBlockCount>
		{
		public:
			Tiny3Block(ulong_t ElementSize) noexcept
				: MemoryBlock(ElementSize)
			{}

			Tiny3Block(ulong_t ElementSize, ulong_t ElementsCount) noexcept
				: MemoryBlock(ElementSize, ElementsCount)
			{}
		};
		class SmallBlock : public MemoryBlock<SmallMemBlockSize, true>, public TObjectStore<SmallBlock, SmallMemBlockCount>
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
			R_TRY_L(Tiny1Block::Preallocate(), "MemoryManager::Tiny1Block Failed to preallocate!") {}
			R_TRY_L(Tiny2Block::Preallocate(), "MemoryManager::Tiny2Block Failed to preallocate!") {}
			R_TRY_L(Tiny3Block::Preallocate(), "MemoryManager::Tiny3Block Failed to preallocate!") {}
			R_TRY_L(SmallBlock::Preallocate(), "MemoryManager::SmallBlock Failed to preallocate!") {}
			R_TRY_L(MediumBlock::Preallocate(), "MemoryManager::SmallBlock Failed to preallocate!") {}
			R_TRY_L(LargeBlock::Preallocate(), "MemoryManager::SmallBlock Failed to preallocate!") {}
			R_TRY_L(ExtraLargeBlock::Preallocate(), "MemoryManager::SmallBlock Failed to preallocate!") {}

			GAllocateCount = 0;

			return RSuccess;
		}
		_NODISCARD static RStatus Shutdown() noexcept
		{
			return RSuccess;
		}

#ifdef MEMEX_STATISTICS
		static std::atomic<size_t> CustomSizeAllocations;
		static std::atomic<size_t> CustomSizeDeallocations;

		static inline std::atomic_uint64_t GAllocateCount;
		static inline std::atomic_uint64_t GFreeCount;

		static void PrintStatistics()
		{
			LogInfo("MemoryManager ###############################################################\n");
			LogInfo("\n\tTiny1Block:\n\t\tAllocations:{}\n\t\tDeallocations:{}\n\t\tOSAllocations:{}\n\t\tOSDeallocations:{}",
				Tiny1Block::GetTotalAllocations(),
				Tiny1Block::GetTotalDeallocations(),
				Tiny1Block::GetTotalOSAllocations(),
				Tiny1Block::GetTotalOSDeallocations()
			);
			LogInfo("\n\tTiny2Block:\n\t\tAllocations:{}\n\t\tDeallocations:{}\n\t\tOSAllocations:{}\n\t\tOSDeallocations:{}",
				Tiny2Block::GetTotalAllocations(),
				Tiny2Block::GetTotalDeallocations(),
				Tiny2Block::GetTotalOSAllocations(),
				Tiny2Block::GetTotalOSDeallocations()
			);
			LogInfo("\n\tTiny3Block:\n\t\tAllocations:{}\n\t\tDeallocations:{}\n\t\tOSAllocations:{}\n\t\tOSDeallocations:{}",
				Tiny3Block::GetTotalAllocations(),
				Tiny3Block::GetTotalDeallocations(),
				Tiny3Block::GetTotalOSAllocations(),
				Tiny3Block::GetTotalOSDeallocations()
			);
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
			LogInfo("\n\tGAllocate:\n\t\tAllocations:{}\n\t\tDeallocations:{}",
				GAllocateCount.load(),
				GFreeCount.load()
			);
			LogInfo("\n\tTotal Allocation:{}\n\tTotal Deallocations:{}\n\tTotal OSAllocations:{}\n\tTotal OSDeallocations:{}",
				Tiny1Block::GetTotalAllocations()
				+ Tiny2Block::GetTotalAllocations()
				+ Tiny3Block::GetTotalAllocations()
				+ SmallBlock::GetTotalAllocations()
				+ MediumBlock::GetTotalAllocations()
				+ LargeBlock::GetTotalAllocations()
				+ ExtraLargeBlock::GetTotalAllocations()
				+ CustomSizeAllocations.load(),

				Tiny1Block::GetTotalDeallocations()
				+ Tiny2Block::GetTotalDeallocations()
				+ Tiny3Block::GetTotalDeallocations()
				+ SmallBlock::GetTotalDeallocations()
				+ MediumBlock::GetTotalDeallocations()
				+ LargeBlock::GetTotalDeallocations()
				+ ExtraLargeBlock::GetTotalDeallocations()
				+ CustomSizeDeallocations.load(),

				Tiny1Block::GetTotalOSAllocations()
				+ Tiny2Block::GetTotalOSAllocations()
				+ Tiny3Block::GetTotalOSAllocations()
				+ SmallBlock::GetTotalOSAllocations()
				+ MediumBlock::GetTotalOSAllocations()
				+ LargeBlock::GetTotalOSAllocations()
				+ ExtraLargeBlock::GetTotalOSAllocations(),

				Tiny1Block::GetTotalOSDeallocations()
				+ Tiny2Block::GetTotalOSDeallocations()
				+ Tiny3Block::GetTotalOSDeallocations()
				+ SmallBlock::GetTotalOSDeallocations()
				+ MediumBlock::GetTotalOSDeallocations()
				+ LargeBlock::GetTotalOSDeallocations()
				+ ExtraLargeBlock::GetTotalOSDeallocations()
			);
			LogInfo("MemoryManager ###############################################################\n");
		}
#endif

#pragma region Compiletime

		template<typename T, typename ...TArgs>
		_NODISCARD static MSharedPtr<T> AllocShared(TArgs... Args) noexcept
		{
			if constexpr (std::is_reference_v<T>)
			{
				static_assert(false, "Alloc<T> Cannot allocate T reference!");
			}
			if constexpr (std::is_array_v<T>)
			{
				static_assert(false, "Use AllocBuffer(Count) to allocate arrays!");
			}

			TBlockType* NewBlockObject = AllocT<T>();
			if (!NewBlockObject)
			{
				return { nullptr , nullptr };
			}

			ptr_t Ptr = NewBlockObject->Block;

			//Construct T
			GConstructNothrow<T>(Ptr, std::forward<TArgs>(Args)...);

			return { NewBlockObject, reinterpret_cast<T*>(Ptr) };
		}

		template<typename T, typename ...TArgs>
		_NODISCARD static MPtr<T> Alloc(TArgs... Args) noexcept
		{
			if constexpr (std::is_reference_v<T>)
			{
				static_assert(false, "Alloc<T> Cannot allocate T reference!");
			}
			if constexpr (std::is_array_v<T>)
			{
				static_assert(false, "Use AllocBuffer(Count) to allocate arrays!");
			}

			constexpr size_t Size = sizeof(T);

			TBlockType* NewBlockObject = AllocT<T>();
			if (!NewBlockObject)
			{
				return { nullptr , nullptr };
			}

			ptr_t Ptr = NewBlockObject->Block;

			//Construct T
			GConstructNothrow<T>(Ptr, std::forward<TArgs>(Args)...);

			return { NewBlockObject, reinterpret_cast<T*>(Ptr) };
		}

		template<typename T>
		_NODISCARD static TBlockType* AllocT() noexcept
		{
			constexpr size_t Size = sizeof(T);

			TBlockType* NewBlockObject = nullptr;

			if constexpr (Size <= Tiny1MemBlockSize)
			{
				NewBlockObject = Tiny1Block::NewRaw((ulong_t)Size);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() Tiny1Block::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					if (bCallDestructor)
					{
						GDestructNothrow<T>((T*)NewBlockObject->Block);
					}

					Tiny1Block::Deallocate(reinterpret_cast<Tiny1Block*>(NewBlockObject));
				};
			}
			else if constexpr (Size <= Tiny2MemBlockSize)
			{
				NewBlockObject = Tiny2Block::NewRaw((ulong_t)Size);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() Tiny2Block::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					if (bCallDestructor)
					{
						GDestructNothrow<T>((T*)NewBlockObject->Block);
					}

					Tiny2Block::Deallocate(reinterpret_cast<Tiny2Block*>(NewBlockObject));
				};
			}
			else if constexpr (Size <= Tiny3MemBlockSize)
			{
				NewBlockObject = Tiny3Block::NewRaw((ulong_t)Size);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() Tiny3Block::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					if (bCallDestructor)
					{
						GDestructNothrow<T>((T*)NewBlockObject->Block);
					}

					Tiny3Block::Deallocate(reinterpret_cast<Tiny3Block*>(NewBlockObject));
				};
			}
			else if constexpr (Size <= SmallMemBlockSize)
			{
				NewBlockObject = SmallBlock::NewRaw((ulong_t)Size);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() SmallBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					if (bCallDestructor)
					{
						GDestructNothrow<T>((T*)NewBlockObject->Block);
					}

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
					if (bCallDestructor)
					{
						GDestructNothrow<T>((T*)NewBlockObject->Block);
					}

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
					if (bCallDestructor)
					{
						GDestructNothrow<T>((T*)NewBlockObject->Block);
					}

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
					if (bCallDestructor)
					{
						GDestructNothrow<T>((T*)NewBlockObject->Block);
					}

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
						if (bCallDestructor)
						{
							GDestructNothrow<T>((T*)NewBlockObject->Block);
						}

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

		template<size_t Size>
		_NODISCARD static TBlockType* AllocBlock() noexcept
		{
			TBlockType* NewBlockObject = nullptr;

			if constexpr (Size <= Tiny1MemBlockSize)
			{
				NewBlockObject = Tiny1Block::NewRaw((ulong_t)Size);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() Tiny1Block::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					Tiny1Block::Deallocate(reinterpret_cast<Tiny1Block*>(NewBlockObject));
				};
			}
			else if constexpr (Size <= Tiny2MemBlockSize)
			{
				NewBlockObject = Tiny2Block::NewRaw((ulong_t)Size);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() Tiny2Block::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					Tiny2Block::Deallocate(reinterpret_cast<Tiny2Block*>(NewBlockObject));
				};
			}
			else if constexpr (Size <= Tiny3MemBlockSize)
			{
				NewBlockObject = Tiny3Block::NewRaw((ulong_t)Size);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() Tiny3Block::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					Tiny3Block::Deallocate(reinterpret_cast<Tiny3Block*>(NewBlockObject));
				};
			}
			else if constexpr (Size <= SmallMemBlockSize)
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

			//Change to size in bytes
			const size_t Size = (sizeof(T) * Count);

			ptr_t Ptr = reinterpret_cast<ptr_t>(NewBlockObject->Block);

			if constexpr (std::is_nothrow_constructible_v<T>)
			{
				if constexpr (!bDontConstructElements)
				{
					//Call default constructor manually for each object of the array
					for (size_t i = 0; i < Count; i++)
					{
						new (reinterpret_cast<uint8_t*>(Ptr) + (sizeof(T) * i)) T();
					}
				}
			}
			else if constexpr (std::is_default_constructible_v<T>)
			{
				static_assert(false, "MemoryManager::AllocBuffer<T, bDontConstructElements>() T must be default nothrow constructible");
			}

			return { NewBlockObject, reinterpret_cast<T*>(Ptr) };
		}

		template<typename T>
		_NODISCARD static TBlockType* AllocBlock(size_t Count) noexcept
		{
			const size_t Size = (sizeof(T) * Count);

			TBlockType* NewBlockObject = nullptr;

			if (Size <= Tiny1MemBlockSize)
			{
				NewBlockObject = Tiny1Block::NewRaw((ulong_t)sizeof(T), (ulong_t)Count);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::AllocBlock() Tiny1Block::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					if (bCallDestructor)
					{
						assert(false); //@TODO destruct buffer
					}

					Tiny1Block::Deallocate(reinterpret_cast<Tiny1Block*>(NewBlockObject));
				};
			}
			else if (Size <= Tiny2MemBlockSize)
			{
				NewBlockObject = Tiny2Block::NewRaw((ulong_t)sizeof(T), (ulong_t)Count);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() Tiny2Block::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					if (bCallDestructor)
					{
						assert(false); //@TODO destruct buffer
					}

					Tiny2Block::Deallocate(reinterpret_cast<Tiny2Block*>(NewBlockObject));
				};
			}
			else if (Size <= Tiny3MemBlockSize)
			{
				NewBlockObject = Tiny3Block::NewRaw((ulong_t)sizeof(T), (ulong_t)Count);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() Tiny3Block::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					if (bCallDestructor)
					{
						assert(false); //@TODO destruct buffer
					}

					Tiny3Block::Deallocate(reinterpret_cast<Tiny3Block*>(NewBlockObject));
				};
			}
			else if (Size <= SmallMemBlockSize)
			{
				NewBlockObject = SmallBlock::NewRaw((ulong_t)sizeof(T), (ulong_t)Count);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::Alloc() SmallBlock::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor) noexcept
				{
					if (bCallDestructor)
					{
						assert(false); //@TODO destruct buffer
					}

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
					if (bCallDestructor)
					{
						assert(false); //@TODO destruct buffer
					}

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
					if (bCallDestructor)
					{
						assert(false); //@TODO destruct buffer
					}

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
					if (bCallDestructor)
					{
						assert(false); //@TODO destruct buffer
					}

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
						if (bCallDestructor)
						{
							assert(false); //@TODO destruct buffer
						}

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

			if (SizeInBytes <= Tiny1MemBlockSize)
			{
				NewBlockObject = Tiny1Block::NewRaw((ulong_t)sizeof(uint8_t), (ulong_t)SizeInBytes);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::AllocBlock() Tiny1Block::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					Tiny1Block::Deallocate(reinterpret_cast<Tiny1Block*>(NewBlockObject));
				};
			}
			else if (SizeInBytes <= Tiny2MemBlockSize)
			{
				NewBlockObject = Tiny2Block::NewRaw((ulong_t)sizeof(uint8_t), (ulong_t)SizeInBytes);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::AllocBlock() Tiny2Block::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					Tiny2Block::Deallocate(reinterpret_cast<Tiny2Block*>(NewBlockObject));
				};
			}
			else if (SizeInBytes <= Tiny3MemBlockSize)
			{
				NewBlockObject = Tiny3Block::NewRaw((ulong_t)sizeof(uint8_t), (ulong_t)SizeInBytes);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::AllocBlock() Tiny3Block::NewRaw() Failed!");
					return nullptr;
				}

				NewBlockObject->Destroy = [NewBlockObject](bool bCallDestructor)
				{
					Tiny3Block::Deallocate(reinterpret_cast<Tiny3Block*>(NewBlockObject));
				};
			}
			else if (SizeInBytes <= SmallMemBlockSize)
			{
				NewBlockObject = SmallBlock::NewRaw((ulong_t)sizeof(uint8_t), (ulong_t)SizeInBytes);
				if (!NewBlockObject)
				{
					LogFatal("MemoryManager::AllocBlock() SmallBlock::NewRaw() Failed!");
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
					LogFatal("MemoryManager::AllocBlock() MediumBlock::NewRaw() Failed!");
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
					LogFatal("MemoryManager::AllocBlock() LargeBlock::NewRaw() Failed!");
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
					LogFatal("MemoryManager::AllocBlock() ExtraLargeBlock::NewRaw() Failed!");
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

	/*
	* \Creates an instance of T(args...) by allocating memory from the MemoryManager and wrapps it in a unique pointer object
	*/
	template<typename T, typename ...Args>
	_NODISCARD FORCEINLINE MPtr<T> MakeUniqueManaged(Args... args) noexcept
	{
		return std::move(MemoryManager::Alloc<T>(std::forward<Args>(args)...));
	}

	/*
	* \brief Creates an instance of T(args...) by allocating memory from the MemoryManager and wrapps it in a shared pointer object
	*/
	template<typename T, typename ...Args>
	_NODISCARD FORCEINLINE MSharedPtr<T> MakeSharedManaged(Args... args) noexcept
	{
		return std::move(MemoryManager::AllocShared<T>(std::forward<Args>(args)...));
	}

	/*
	* \brief Same as MakeSharedManaged but the instance itself will hold the destroy handler
	* \returns Shared ptr to the instance
	*/
	template<typename T, typename ...TArgs>
	_NODISCARD FORCEINLINE TSharedPtr<T> MakeSharedManagedEx(TArgs... Args) noexcept
	{
		static_assert(!std::is_array_v<T>, "MakeSharedManagedEx<T, Args> Can't allcate arrays!");
		static_assert(std::is_base_of_v<MemoryResource<true>, T>, "MakeSharedManagedEx<T, Args> T must inherit MemoryResource<true>!");

		using TBlockType = MemoryManager::TBlockType;

		TBlockType* Block = MemoryManager::AllocT<T>();
		if (!Block)
		{
			return { nullptr };
		}

		//Construct T
		GConstructNothrow<T, TArgs...>(Block->Block, std::forward<TArgs>(Args)...);

		((T*)Block->Block)->Destroy = [Block](bool bCallDestructor) noexcept
		{
			if (bCallDestructor)
			{
				GDestructNothrow<T>((T*)Block->Block);
			}

			Block->Destroy(false);
		};

		return { (T*)Block->Block };
	}

	/*
	* \brief Same as MakeUniqueManaged but the instance itself will hold the destroy handler
	* \returns Unique ptr to the instance
	*/
	template<typename T, typename ...TArgs>
	_NODISCARD FORCEINLINE TPtr<T> MakeUniqueManagedEx(TArgs... Args) noexcept
	{
		static_assert(!std::is_array_v<T>, "MakeSharedManagedEx<T, Args> Can't allcate arrays!");
		static_assert(std::is_base_of_v<NotSharedMemoryResourceBase, T>, "MakeUniqueManagedEx<T, Args> T must inherit NotSharedMemoryResourceBase!");

		using TBlockType = MemoryManager::TBlockType;

		TBlockType* Block = MemoryManager::AllocT<T>();
		if (!Block)
		{
			return { nullptr };
		}

		//Construct T
		GConstructNothrow<T, TArgs...>(Block->Block, std::forward<TArgs>(Args)...);

		((T*)Block->Block)->Destroy = [Block](bool bCallDestructor) noexcept
		{
			if (bCallDestructor)
			{
				GDestructNothrow<T>((T*)Block->Block);
			}

			Block->Destroy(false);
		};

		return { (T*)Block->Block };
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