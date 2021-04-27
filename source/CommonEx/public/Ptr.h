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

namespace CommonEx {
#pragma region _TPtr [Base of all TPtr]

	template<typename T>
	class _TPtrBase {
		static const bool bIsMemoryResource = std::is_base_of_v<MemoryResourceBase, T>;

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
		_NODISCARD FORCEINLINE T& operator[](size_t Index) noexcept {
			return Ptr[Index];
		}
		//Array element access [const]
		_NODISCARD FORCEINLINE const T& operator[](size_t Index) const noexcept {
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

		FORCEINLINE void DestroyResource()const noexcept {
			if constexpr (bIsMemoryResource) {
				if (this->Ptr)
				{
					this->Ptr->Destroy(this->Ptr, true);
					this->Ptr = nullptr;
				}
			}
			else {
				static_assert(false, "Implement specific DestroyResource()!");
			}
		}

	protected:
		FORCEINLINE _TPtrBase() noexcept : Ptr(nullptr) {}
		FORCEINLINE _TPtrBase(T* Ptr) noexcept : Ptr(Ptr) {}

		//The wrapped pointer
		mutable T* PTR	Ptr{ nullptr };
	};

	//Unique pointer wrapper
	template<typename T, typename Base = _TPtrBase<T>>
	class _TPtr : public Base {

	public:
		//Default constructors
		FORCEINLINE _TPtr() noexcept :Base() {}
		FORCEINLINE _TPtr(T* Ptr) noexcept :Base(Ptr) {}

		//Can move
		FORCEINLINE _TPtr(_TPtr&& Other) noexcept : Base() {
			//Use move operator
			(*this) = std::move(Other);
		};
		FORCEINLINE _TPtr& operator=(_TPtr&& Other) noexcept {
			if (this == &Other) {
				return *this;
			}

			Reset(Other.Release());

			return *this;
		};

		//Can't copy
		_TPtr(_TPtr&) = delete;
		_TPtr& operator=(_TPtr&) = delete;

		~_TPtr() noexcept {
			this->DestroyResource();
		}

		//Cast to other type. Last minute resort as it creates a copy of itself
		// * Strive to just cast the internal raw pointer ie. (NewType*)TPtr.Get() else the compiler might catch it *
		template<typename TargetType>
		_NODISCARD FORCEINLINE _TPtr<TargetType, Base> Cast() noexcept {
			return _TPtr<TargetType, Base>((TargetType*)Release());
		}

		_NODISCARD FORCEINLINE T* Release() noexcept {
			T* Temp = this->Ptr;
			this->Ptr = nullptr;
			return Temp;
		}

		FORCEINLINE void Reset(T* Resource = nullptr) noexcept {
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
	class _TSharedPtr : public Base {
	public:
		FORCEINLINE _TSharedPtr()  noexcept :Base() {}
		FORCEINLINE _TSharedPtr(T* Ptr)  noexcept : Base(Ptr) {}

		//Move
		FORCEINLINE _TSharedPtr(_TSharedPtr&& Other) noexcept {
			this->Ptr = Other.Ptr;
			Other.Ptr = nullptr;
		};
		FORCEINLINE _TSharedPtr& operator=(_TSharedPtr&& Other)noexcept {
			if (this == &Other) {
				return *this;
			}

			this->Ptr = Other.Ptr;
			Other.Ptr = nullptr;

			return *this;
		};

		//Copy
		FORCEINLINE _TSharedPtr(const _TSharedPtr& Other) noexcept {
			Other.IncRef();
			this->Ptr = Other.Ptr;
		};
		FORCEINLINE _TSharedPtr& operator=(const _TSharedPtr& Other) noexcept {
			if (this == &Other) {
				return *this;
			}

			Other.AddReference();
			this->Ptr = Other.Ptr;

			return *this;
		};

		~_TSharedPtr()  noexcept {
			Release();
		}

		FORCEINLINE void Release() noexcept {
			ReleaseReference();
			this->Ptr = nullptr;
		}

	private:
		FORCEINLINE void AddReference() const noexcept {
			if (this->IsNull()) { return false; }

			this->Ptr->AddReference();
		}
		FORCEINLINE void ReleaseReference() const noexcept {
			if (this->IsNull()) { return; }

			if (this->Ptr->ReleaseReference()) {
				this->DestroyResource();
			}
		}

		template<typename K, size_t PoolSize, bool bUseSpinLock>
		friend class TObjectPool;
		template<typename T, typename BasePtr>
		friend class _MPtr;
		friend class MemoryManager;
	};

#pragma endregion

#pragma region _MPtr [Base of all MPtr]

	//Memory Block pointer abstraction
	//It consits of 2 pointers, one to the MemoryBlock, and the other is an aligned pointer into the MemoryBlock, of type T
	template<typename T, typename MyBlockPtr>
	class _MPtr : public _TPtrBase<T> {
		static_assert(
			std::is_same_v<_TPtr<IMemoryBlock>, MyBlockPtr> ||
			std::is_same_v<_TSharedPtr<IMemoryBlock>, MyBlockPtr>,
			"See _MPtr<T, MyBlockPtr>");
	public:
		using MyType = _MPtr<T, MyBlockPtr>;

		FORCEINLINE _MPtr() {}
		FORCEINLINE _MPtr(IMemoryBlock* BlockObject, T* Ptr) :_TPtrBase<T>(Ptr), BlockObject(BlockObject) {}
		FORCEINLINE _MPtr(MyBlockPtr&& BlockObject, T* Ptr) : _TPtrBase<T>(Ptr), BlockObject(std::move(BlockObject)) {}

		//Cant copy
		_MPtr(const _MPtr&) = delete;
		_MPtr& operator=(const _MPtr&) = delete;

		//Move
		FORCEINLINE _MPtr(_MPtr&& Other) noexcept : BlockObject(std::move(Other.BlockObject)) {
			auto Temp = Other.Ptr;
			Other.Ptr = nullptr;
			this->Ptr = Temp;
		}
		FORCEINLINE _MPtr& operator=(_MPtr&& Other) noexcept {
			if (this == &Other) {
				return *this;
			}

			BlockObject = std::move(Other.BlockObject);

			auto Temp = Other.Ptr;
			Other.Ptr = nullptr;
			this->Ptr = Temp;

			return *this;
		}

		_NODISCARD FORCEINLINE size_t GetCapacity() const noexcept {
			if (this->IsNull() || BlockObject.IsNull()) { return 0; }

			const auto Ptr = reinterpret_cast<uint8_t*>(this->Ptr);

			return static_cast<size_t>(BlockObject->GetEnd() - BlockObject->GetBegin(static_cast<ulong_t>(Ptr - BlockObject->Block)));
		}

		_NODISCARD FORCEINLINE IMemoryBlock* GetMemoryBlock() noexcept {
			return this->BlockObject.Ptr;
		}

		_NODISCARD FORCEINLINE const IMemoryBlock* GetMemoryBlock() const noexcept {
			return this->BlockObject.Ptr;
		}

		FORCEINLINE void Reset() noexcept {
			this->Ptr = nullptr;
			BlockObject.Reset();
		}

	protected:
		MyBlockPtr BlockObject{ nullptr };

		friend class MemoryManager;
		template <typename T, bool bIsFixed>
		friend class TStructureBase;
	};

	using IMemoryBlockPtr = _TPtr<IMemoryBlock>;
	using IMemoryBlockSharedPtr = _TSharedPtr<IMemoryBlock>;

	//MemoryBlock unique pointer
	template<typename T>
	using MPtr = _MPtr<T, IMemoryBlockPtr>;

	//MemoryBlock shared pointer
	template<typename T>
	using MSharedPtr = _MPtr<T, IMemoryBlockSharedPtr>;

#pragma endregion

#pragma region TPtr [General purpose heap smart pointers]

	template<typename T>
	using TPtr = std::unique_ptr<T>;

	template<typename T>
	using TSharedPtr = std::shared_ptr<T>;

	//Creates an heap instance of T(args...) and wrapps it in a unique pointer
	template<typename T, typename ...Args>
	_NODISCARD FORCEINLINE TPtr<T> MakeUnique(Args... args)noexcept {
		return std::move(std::make_unique<T>(std::forward<Args...>(args)...));
	}

	//Creates an heap instance of T(args...) and wrapps it in a shared pointer
	template<typename T, typename ...Args>
	_NODISCARD FORCEINLINE TSharedPtr<T> MakeShared(Args... args)noexcept {
		return std::move(std::make_shared<T>(std::forward<Args...>(args)...));
	}

#pragma endregion
}