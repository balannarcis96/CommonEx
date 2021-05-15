#pragma once
/**
 * @file Ptr.h
 *
 * @brief CommonEx Smart Pointers
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

namespace CommonEx
{
#pragma region _TPtr [Base of all TPtr]

	template<typename T>
	class _TPtrBase
	{
		static const bool bIsMemoryResource = std::is_base_of_v<NotSharedMemoryResourceBase, T>;

	public:
		//Dereference the pointer
		_NODISCARD FORCEINLINE T& operator*() noexcept
		{
			return *Ptr;
		}
		//Dereference the pointer [const]
		_NODISCARD FORCEINLINE const T& operator*() const noexcept
		{
			return *Ptr;
		}

		//Access member
		_NODISCARD FORCEINLINE T* operator->() noexcept
		{
			return Ptr;
		}
		//Access member [const]
		_NODISCARD FORCEINLINE const T* operator->() const noexcept
		{
			return Ptr;
		}

		//Array element access
		_NODISCARD FORCEINLINE T& operator[](size_t Index) noexcept
		{
			return Ptr[Index];
		}
		//Array element access [const]
		_NODISCARD FORCEINLINE const T& operator[](size_t Index) const noexcept
		{
			return Ptr[Index];
		}

		//Get the wrapped pointer
		_NODISCARD FORCEINLINE T* Get() noexcept
		{
			return Ptr;
		}
		//Get the wrapped pointer [const]
		_NODISCARD FORCEINLINE const T* Get() const noexcept
		{
			return Ptr;
		}

		//Chec if the wrapped pointer is nullptr
		_NODISCARD FORCEINLINE bool IsNull() const noexcept
		{
			return Ptr == nullptr;
		}

		//Chec if the wrapped pointer is nullptr
		FORCEINLINE explicit operator bool() const noexcept
		{
			return Ptr != nullptr;
		}

	protected:

		FORCEINLINE void DestroyResource()const noexcept
		{
			if constexpr (bIsMemoryResource)
			{
				if (this->Ptr)
				{
					this->Ptr->Destroy(true);
					this->Ptr = nullptr;
				}
			}
			else
			{
				GFreeCpp(this->Ptr);
				this->Ptr = nullptr;
			}
		}

		FORCEINLINE _TPtrBase() noexcept : Ptr(nullptr) {}
		FORCEINLINE _TPtrBase(T* Ptr) noexcept : Ptr(Ptr) {}

		//The wrapped pointer
		mutable T* PTR	Ptr{ nullptr };
	};

	//Unique pointer wrapper
	template<typename T, typename Base = _TPtrBase<T>>
	class _TPtr : public Base
	{
	public:
		//Default constructors
		FORCEINLINE _TPtr() noexcept :Base() {}
		FORCEINLINE _TPtr(T* Ptr) noexcept :Base(Ptr) {}

		//Can move
		FORCEINLINE _TPtr(_TPtr&& Other) noexcept : Base()
		{
			//Use move operator
			(*this) = std::move(Other);
		};
		FORCEINLINE _TPtr& operator=(_TPtr&& Other) noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			Reset(Other.Release());

			return *this;
		};

		//Can't copy
		_TPtr(_TPtr&) = delete;
		_TPtr& operator=(_TPtr&) = delete;

		~_TPtr() noexcept
		{
			this->DestroyResource();
		}

		//Cast to other type. Last minute resort as it creates a copy of itself
		// * Strive to just cast the internal raw pointer ie. (NewType*)TPtr.Get() else the compiler might catch it *
		template<typename TargetType>
		_NODISCARD FORCEINLINE _TPtr<TargetType, Base> Cast() noexcept
		{
			return _TPtr<TargetType, Base>((TargetType*)Release());
		}

		_NODISCARD FORCEINLINE T* Release() noexcept
		{
			T* Temp = this->Ptr;
			this->Ptr = nullptr;
			return Temp;
		}

		FORCEINLINE void Reset(T* Resource = nullptr) noexcept
		{
			this->DestroyResource();
			this->Ptr = Resource;
		}

		friend class MemoryManager;
		template<typename T, typename BasePtr>
		friend class _MPtr;
	};

	//Shared pointer wrapper
	// * Not thread safe, sync before copying or moving if in contention space*
	template<typename T, typename Base = _TPtrBase<T>>
	class _TSharedPtr : public Base
	{
	public:
		FORCEINLINE _TSharedPtr()  noexcept :Base() {}
		FORCEINLINE _TSharedPtr(T* Ptr)  noexcept : Base(Ptr) {}

		//Move
		FORCEINLINE _TSharedPtr(_TSharedPtr&& Other) noexcept
		{
			this->Ptr = Other.Ptr;
			Other.Ptr = nullptr;
		};
		FORCEINLINE _TSharedPtr& operator=(_TSharedPtr&& Other)noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			this->Ptr = Other.Ptr;
			Other.Ptr = nullptr;

			return *this;
		};

		//Copy
		FORCEINLINE _TSharedPtr(const _TSharedPtr& Other) noexcept
		{
			Other.AddReference();
			this->Ptr = Other.Ptr;
		};
		FORCEINLINE _TSharedPtr& operator=(const _TSharedPtr& Other) noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			Other.AddReference();
			this->Ptr = Other.Ptr;

			return *this;
		};

		~_TSharedPtr()  noexcept
		{
			Release();
		}

		FORCEINLINE void Release() noexcept
		{
			ReleaseReference();
			this->Ptr = nullptr;
		}

	private:
		FORCEINLINE void AddReference() const noexcept
		{
			if (this->IsNull()) { return; }

			this->Ptr->AddReference();
		}
		FORCEINLINE void ReleaseReference() const noexcept
		{
			if (this->IsNull()) { return; }

			if (this->Ptr->ReleaseReference())
			{
				this->DestroyResource();
			}

			this->Ptr = nullptr;
		}

		template<typename K, size_t PoolSize, bool bUseSpinLock>
		friend class TObjectPool;
		friend class MemoryManager;
	};

#pragma endregion

#pragma region _MPtr [Base of all MPtr]

	template<typename T>
	class MPtrBase : public _TPtrBase<T>
	{
		using MyType = MPtrBase<T>;
		using MyBase = _TPtrBase<T>;
	public:
		_NODISCARD FORCEINLINE size_t GetCapacity() const noexcept
		{
			if (this->IsNull() || this->BlockObject == nullptr) { return 0; }

			const auto Ptr = reinterpret_cast<uint8_t*>(this->Ptr);

			return static_cast<size_t>(BlockObject->GetEnd() - BlockObject->GetBegin(static_cast<ulong_t>(Ptr - BlockObject->Block)));
		}

		_NODISCARD FORCEINLINE MemoryBlockBaseResource* GetMemoryBlock() noexcept
		{
			return this->BlockObject;
		}

		_NODISCARD FORCEINLINE const MemoryBlockBaseResource* GetMemoryBlock() const noexcept
		{
			return this->BlockObject;
		}

	protected:
		FORCEINLINE MPtrBase() noexcept : MyBase() {}
		FORCEINLINE MPtrBase(MemoryBlockBaseResource* BlockObject, T* Ptr) noexcept : MyBase(Ptr), BlockObject(BlockObject) {}
		FORCEINLINE MPtrBase(MPtrBase&& Other) noexcept : MyBase(Other.Ptr), BlockObject(Other.BlockObject)
		{
			Other.Ptr = nullptr;
			Other.BlockObject = nullptr;
		}

		MemoryBlockBaseResource* BlockObject{ nullptr };
	};

	/*
	* \brief Unique MemoryResource smart pointer.
	* 
	* \code It consits of 2 pointers, one to the MemoryBlock, and the other is an aligned pointer into the MemoryBlock, of type T.
	*/
	template<typename T>
	class MPtr : public MPtrBase<T>
	{
	public:
		using MyBase = MPtrBase<T>;
		using MyType = MPtr<T>;

		FORCEINLINE MPtr()noexcept {}
		FORCEINLINE MPtr(MemoryBlockBaseResource* BlockObject, T* Ptr)noexcept :MyBase(BlockObject, Ptr) {}

		//Can't copy
		MPtr(const MyType&) = delete;
		MPtr& operator=(const MyType&) = delete;

		//Move
		FORCEINLINE MPtr(MyType&& Other) noexcept : MyBase(std::move(Other)) {}
		FORCEINLINE MPtr& operator=(MyType&& Other) noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			this->BlockObject = Other.BlockObject;
			this->Ptr = Other.Ptr;

			Other.BlockObject = nullptr;
			Other.Ptr = nullptr;

			return *this;
		}

		template<bool bCallDestructor = true>
		FORCEINLINE void Reset() noexcept
		{
			if (this->BlockObject)
			{
				if constexpr (std::is_nothrow_destructible_v<T>)
				{
					if constexpr (bCallDestructor)
					{
						this->Ptr->~T();
					}
				}
				else if constexpr (std::is_nothrow_destructible_v<T>)
				{
					static_assert(false, "MPtr<T> T must be nothrow destructible");
				}

				this->BlockObject->Destroy(false);

				this->Ptr = nullptr;
				this->BlockObject = nullptr;
			}
		}

		_NODISCARD FORCEINLINE MemoryBlockBaseResource* Release() noexcept
		{
			MemoryBlockBaseResource* Temp = this->BlockObject;

			this->BlockObject = nullptr;
			this->Ptr = nullptr;

			return Temp;
		}

		~MPtr() noexcept
		{
			Reset();
		}

		friend class MemoryManager;
		template <typename T, bool bIsFixed>
		friend class TStructureBase;
	};

	/*
	* \brief Shared MemoryResource smart pointer.
	* 
	* \code It consits of 2 pointers, one to the MemoryBlock, and the other is an aligned pointer into the MemoryBlock, of type T.
	*/
	template<typename T>
	class MSharedPtr : public MPtrBase<T>
	{
	public:
		using MyBase = MPtrBase<T>;
		using MyType = MSharedPtr<T>;

		FORCEINLINE MSharedPtr()noexcept {}
		FORCEINLINE MSharedPtr(MemoryBlockBaseResource* BlockObject, T* Ptr)noexcept :MyBase(BlockObject, Ptr) {}

		//Copy
		MSharedPtr(const MyType& Other) noexcept
		{
			(*this) = Other;
		}
		MyType& operator=(const MyType& Other) noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			Other.BlockObject->AddReference();

			this->BlockObject = Other.BlockObject;
			this->Ptr = Other.Ptr;

			return *this;
		}

		//Move
		FORCEINLINE MSharedPtr(MyType&& Other) noexcept : MyBase(std::move(Other)) {}
		FORCEINLINE MyType& operator=(MyType&& Other) noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			this->Ptr = Other.Ptr;
			this->BlockObject = Other.BlockObject;

			Other.Ptr = nullptr;
			Other.BlockObject = nullptr;

			return *this;
		}

		void Reset() noexcept
		{
			if (this->BlockObject)
			{
				if (this->BlockObject->ReleaseReference())
				{
					if constexpr (std::is_nothrow_destructible_v<T>)
					{
						this->Ptr->~T();
					}
					else if constexpr (std::is_nothrow_destructible_v<T>)
					{
						static_assert(false, "MPtr<T> T must be nothrow destructible");
					}

					this->BlockObject->Destroy(false);
				}
			}

			this->BlockObject = nullptr;
			this->Ptr = nullptr;
		}

		~MSharedPtr() noexcept
		{
			Reset();
		}
	};

#pragma endregion

#pragma region TPtr [General purpose heap smart pointers]

	template<typename T>
	using TPtr = _TPtr<T>;

	template<typename T>
	using TSharedPtr = _TSharedPtr<T>;

	/**
	* \brief Creates an heap instance of T(args...) and wrapps it in a unique pointer.
	*
	* \return TPtr<T>
	*/
	template<typename T, typename ...TArgs>
	_NODISCARD FORCEINLINE TPtr<T> MakeUnique(TArgs... Args)noexcept
	{
		auto* Ptr = (T*)GAllocate(sizeof(T), ALIGNMENT);
		if (!Ptr)
		{
			return nullptr;
		}

		GConstructNothrow<T, TArgs...>((T*)Ptr, std::forward<TArgs>(Args)...);

		return Ptr;
	}

	/**
	 * \brief Creates an heap instance of T(args...) and wrapps it in a shared pointer. T must inherit from MemoryResource<true>
	 *
	 * \return TSharedPtr<T>
	 */
	template<typename T, typename ...Args>
	_NODISCARD FORCEINLINE TSharedPtr<T> MakeShared(Args... args)noexcept
	{
		static_assert(std::is_base_of_v<MemoryResource<true>, T>, "T must inherit from MemoryResource<true>");

		auto* Ptr = (T*)GAllocate(sizeof(T), ALIGNMENT);
		if (!Ptr)
		{
			return nullptr;
		}

		if constexpr (std::is_nothrow_constructible<T, Args...>)
		{
			//construct
			new (Ptr) T(std::forward<Args>(args)...);
		}
		else if (std::is_constructible_v<T, Args...>)
		{
			static_assert("MakeShared<T> can only call no-throw constructors!");
		}

		return Ptr;
	}

#pragma endregion
}