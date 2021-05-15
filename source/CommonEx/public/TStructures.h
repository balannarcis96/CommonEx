#pragma once
/**
 * @file TStructures.h
 *
 * @brief CommonEx Common structures abstractions:
 *			-TVector<T>
 *			-TArray<T, Size> -- if Size = 0 the size must be passed to the constructor
 *			-TStack<T>
 *			-TQueue<T>
 *			-TDQueue<T>
 *			-TMap<KeyType, T>
 *		* All these structures allocate memory through the MemoryManager *
 *
 *		Views:
 *			-TView<T>
 *			-TBlockView
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

namespace CommonEx
{
	template <typename T, bool bIsFixed = false>
	class TStructureBase
	{
		static_assert(std::is_nothrow_destructible_v<T> || !std::is_destructible_v<T>, "TStructBase<T> T must be nothrow destructible");
		static_assert(std::is_nothrow_constructible_v<T> || !std::is_constructible_v<T>, "TStructBase<T> T must be nothrow constructible");

	public:
		using MyType = TStructureBase<T, bIsFixed>;

		//STD traits
		using iterator_category		PTR = std::contiguous_iterator_tag;
		using difference_type		PTR = std::ptrdiff_t;
		using value_type			PTR = std::decay_t<T>;
		using pointer				PTR = value_type*;
		using reference				PTR = value_type&;
		using const_reference		PTR = const value_type&;
		using size_type				PTR = uint32_t;

		//STD iterator
		struct Iterator
		{
			//iterator_traits<T>
			using iterator_concept = std::contiguous_iterator_tag;
			using iterator_category = std::random_access_iterator_tag;
			using value_type = std::remove_cv_t<T>;
			using difference_type = std::ptrdiff_t;
			using pointer = T*;
			using reference = T&;

			Iterator(pointer ptr) noexcept : m_ptr(ptr) {}

			Iterator(const Iterator& Other)noexcept : m_ptr(Other.m_ptr) {}
			Iterator(Iterator&& Other)noexcept : m_ptr(Other.m_ptr) {}

			Iterator& operator=(const Iterator& Other) noexcept { m_ptr = Other.m_ptr; }
			Iterator& operator=(Iterator&& Other) noexcept { m_ptr = Other.m_ptr; }

			reference operator*() const noexcept { return *m_ptr; }
			pointer operator->() const noexcept { return m_ptr; }

			Iterator& operator++() noexcept { m_ptr++; return *this; }
			Iterator& operator--() noexcept { m_ptr--; return *this; }

			Iterator operator++(int) noexcept { Iterator Temp = *this; ++(*this); return Temp; }
			Iterator operator--(int) noexcept { Iterator Temp = *this; --(*this); return Temp; }

			difference_type operator-(const Iterator& Other) noexcept { return (difference_type)((((difference_type)m_ptr) - ((difference_type)Other.m_ptr)) / sizeof(T)); }

			Iterator operator+(difference_type Value) noexcept { return Iterator(m_ptr + Value); }
			Iterator operator-(difference_type Value) noexcept { return Iterator(m_ptr - Value); }

			friend bool operator== (const Iterator& Left, const Iterator& Right)noexcept { return Left.m_ptr == Right.m_ptr; };
			friend bool operator!= (const Iterator& Left, const Iterator& Right)noexcept { return Left.m_ptr != Right.m_ptr; };

		private:
			pointer m_ptr;
		};

		_NODISCARD FORCEINLINE reference front() noexcept { return (*this)[0]; }

		_NODISCARD FORCEINLINE const_reference front() const noexcept { return (*this)[0]; }

		_NODISCARD FORCEINLINE reference back() noexcept { return (*this)[Count - 1]; }

		_NODISCARD FORCEINLINE const_reference back() const noexcept { return (*this)[Count - 1]; }

		_NODISCARD FORCEINLINE Iterator begin() noexcept { return Iterator(Front()); }

		_NODISCARD FORCEINLINE Iterator end() noexcept { return Iterator(Back() + 1); }

		_NODISCARD FORCEINLINE bool empty() const noexcept { return Count == 0; }

		_NODISCARD FORCEINLINE size_type size() const noexcept { return Count; }

		FORCEINLINE void push_back(const value_type& _Val)noexcept
		{
			Push(_Val);
		}

		FORCEINLINE void push_back(value_type&& _Val)noexcept
		{
			Push(std::move(_Val));
		}

		FORCEINLINE void pop_front() noexcept
		{
			//Warning! it doesnt destruct!
			PopFront();
		}

		FORCEINLINE void pop_back() noexcept
		{
			//Warning! it doesnt destruct!
			Pop();
		}

	public:
		using MyT = T;
		using MyTPtr = T*;

		static const uint32_t GrowStep = 32;

		TStructureBase() noexcept { Grow(GrowStep); }
		TStructureBase(const bool bStartEmpty) noexcept { if (!bStartEmpty) { Grow(GrowStep); } }
		TStructureBase(uint32_t Size) noexcept { Grow(Size); }

		TStructureBase(const TStructureBase& Other) noexcept
		{
			(*this) = Other;
		}
		TStructureBase(TStructureBase&& Other) noexcept
		{
			(*this) = std::move(Other);
		}

		~TStructureBase() noexcept
		{
			Destroy();
		}

		inline TStructureBase& operator=(const TStructureBase& Other) noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			if (Other.GetCount() > GetCapacity())
			{
				Grow(Other.GetCount() - GetCapacity());
			}

			if constexpr (std::is_copy_constructible_v<MyT>)
			{
				for (uint32_t i = 0; i < Other.GetCount(); i++)
				{
					(*this)[i] = Other[i];
				}
			}
			else
			{
				RTRY_S_L((RStatus)memcpy_s(
					Front(),
					GetCapacity() * sizeof(T),
					Other.Front(),
					Other.GetCount() * sizeof(T)), *this, "TStructureBase::operator=(const TStructureBase& Other) failed to memcpy_s()")
				{}
			}

			Count = Other.GetCount();

			return *this;
		}

		inline TStructureBase& operator=(TStructureBase&& Other) noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			Count = Other.GetCount();
			Capacity = Other.GetCapacity();
			Memory = std::move(Other.Memory);

			Other.Release();

			return *this;
		}

		inline MyTPtr Front() noexcept { return Memory.Ptr; }

		inline const MyTPtr Front() const noexcept { return Memory.Ptr; }

		inline MyTPtr Back() noexcept { return (Front() + (Count - 1)); }

		inline const MyTPtr Back() const noexcept { return (Front() + (Count - 1)); }

		inline bool IsValid() const noexcept { return Front() != nullptr; }

		inline uint32_t GetCapacity() const noexcept { return Capacity; }

		inline uint32_t GetCount() const noexcept { return Count; }

		inline uint32_t GetSlack() const noexcept { return GetCapacity() - GetCount(); }

		inline bool CanFit(uint32_t FitElementsCount) const noexcept
		{
			return (GetCount() + FitElementsCount) <= GetCapacity();
		}

		template<bool bCallDestructors = true>
		inline void Clear() noexcept
		{
			if constexpr (std::is_nothrow_destructible_v<T>)
			{
				if constexpr (bCallDestructors)
				{
					//Call destructor of each element
					for (size_t i = 0; i < GetCount(); i++)
					{
						Front()[i].~T();
					}
				}
			}
			else if constexpr (std::is_destructible_v<T>)
			{
				static_assert(false, "TStructure<T> T must be nothrow destructible");
			}

			Count = 0;
		}

		inline void Release() noexcept
		{
			Capacity = 0;
			Count = 0;
			Memory.Release();
		}

		template<bool bCallDestructors = true>
		inline void Destroy() noexcept
		{
			Clear<bCallDestructors>();
			Capacity = 0;
			Memory.Reset<false>();
		}

		inline bool Push(const MyT& Item) noexcept
		{
			if (!CanFit(1))
			{
				if constexpr (!bIsFixed)
				{
					RTRY_S_L(
						Grow(GrowStep)
						, false, "TStructureBase::Push(const MyT& Item) call to Grow(GrowStep) failed!"
					)
					{}
				}
				else
				{
					return false;
				}
			}

			Front()[Count++] = Item;

			return true;
		}

		inline bool Push(MyT&& Item) noexcept
		{
			if (!CanFit(1))
			{
				if constexpr (!bIsFixed)
				{
					RTRY_S_L(
						Grow(GrowStep)
						, false, "TStructureBase::Push(MyT&& Item) call to Grow(GrowStep) failed!"
					)
					{}
				}
				else
				{
					return false;
				}
			}

			Front()[Count++] = std::move(Item);

			return true;
		}

		/**
		 * \brief Pop back item. Destructs the item if possible.
		 */
		inline void Pop() noexcept
		{
			if (!GetCount())
			{
				return;
			}

			if constexpr (std::is_nothrow_destructible_v<T>)
			{
				Back()->~T();
			}
			else if constexpr (std::is_destructible_v<T>)
			{
				static_assert(false, "TStructure<T> T must be nothrow destructible");
			}

			Count--;
		}

		/**
		 * \brief Pop front item. Destructs the item if possible. Shifts items to the left by one position.
		 */
		inline void PopFront() noexcept
		{
			if (!GetCount())
			{
				return;
			}

			if constexpr (std::is_nothrow_destructible_v<T>)
			{
				//Destruct poped item
				Front()->~T();
			}

			if (Count > 1)
			{
				ShiftLeft(1);
			}

			Count--;
		}

		inline MyT& operator[](uint32_t Index) noexcept
		{
			return Front()[Index];
		}

		inline const MyT& operator[](uint32_t Index) const noexcept
		{
			return Front()[Index];
		}

	protected:
		void ShiftLeft(size_type SkippFrontAmmount) noexcept
		{
#ifdef _DEBUG
			assert(SkippFrontAmmount < Count);
#endif
			memmove_s(
				Front(),
				Count * sizeof(T),
				(Front() + SkippFrontAmmount),
				(Count - SkippFrontAmmount) * sizeof(T)
			);
		}

		RStatus Grow(size_type AddCapacity) noexcept
		{
			MPtr<T> NewBlock = MemoryManager::AllocBuffer<T, true>((size_t)Capacity + AddCapacity);
			if (NewBlock.IsNull())
			{
				return RFail;
			}

			//! Can be optimized to zero just the new capacity
			NewBlock.GetMemoryBlock()->ZeroMemoryBlock();

			const size_t BlockCapacity = NewBlock.GetCapacity();
			if (IsValid())
			{
				MyTPtr TempFront = NewBlock.Ptr;

				RTRY_L_FMT((RStatus)memcpy_s(
					TempFront,
					BlockCapacity,
					Front(),
					Count * sizeof(MyT)), "_TStructureBase:: call to Grow({}) failed to memcpy_s()", AddCapacity)
				{}
			}

			Memory.Reset<false>();
			Memory = std::move(NewBlock);
			Capacity = (uint32_t)(BlockCapacity / sizeof(MyT));

			return RSuccess;
		}

		MPtr<MyT>	Memory{ nullptr, nullptr };
		size_type	Capacity{ 0 };
		size_type	Count{ 0 };

		template <typename T, uint32_t GrowStep, uint32_t FrontSlackMax>
		friend class TQueue;
	};

	/**
	 * Vector structure (dynamic sized array).
	 * * Memory management is done through the MemoryManager *
	 */
	template <typename T>
	using TVector = TStructureBase<T, false>;

	/**
	 * Stack structure (dynamic sized stack).
	 * * Memory management is done through the MemoryManager *
	 */
	template <typename T>
	using TStack = TStructureBase<T, false>;

	/**
	 * Fixed size heap Array
	 * * Memory management is done through the MemoryManager *
	 */
	template <typename T, uint32_t MaxCapacity = 0>
	struct TArray : TStructureBase<T, true>
	{
		using MyBase = TStructureBase<T, true>;

		TArray() noexcept : MyBase(MaxCapacity) {}
	};

	/**
	 * Fixed size heap Array
	 * * Memory management is done through the MemoryManager *
	 */
	template <typename T>
	struct TArray<T, 0> : TStructureBase<T, true>
	{
		using MyBase = TStructureBase<T, true>;

		TArray(uint32_t Size) noexcept : MyBase(Size) {}
	};

	/*------------------------------------------------------------
		TView simple array view
	  ------------------------------------------------------------*/
	template<typename T>
	struct TView
	{
		TView() :Size(0), Count(0), Elements(nullptr) {}
		TView(const uint32_t Size, T* View) :Size(Size), Count(0), Elements(View) {}
		TView(const uint32_t Size, const T* View) :Size(Size), Count(0), Elements((T*)View) {}
		TView(const uint32_t Size, const uint32_t Count, T* View) :Size(Size), Count(Count), Elements(View) {}

		TView(TView<T>& Other) : Size(Other.Size), Count(Other.Count), Elements(Other.Elements) {}
		TView(TStructureBase<T>& Other) : Size(Other.GetCapacity()), Count(Other.GetCount()), Elements(Other.GetFront()) {}
		TView(std::vector<T>& Other) : Size(Other.size()), Count(Other.size()), Elements((T*)Other.data()) {}

		inline TView<T>& operator=(const TView<T>& Other)noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			Size = Other.Size;
			Count = Other.Count;
			Elements = Other.Elements;

			return *this;
		}

		inline TView<T>& operator=(TStructureBase<T>& Other)noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			Size = Other.GetCapacity();
			Count = Other.GetCount();
			Elements = Other.GetFront();

			return *this;
		}

		//Pushees the item into a new slot.
		inline bool Push(T Item) noexcept
		{
			if (Count + 1 > Size)
			{
				return false;
			}

			Elements[Count++] = Item;
		}

		//Gets Element ptr at index.
		inline T& operator[](uint32_t Index) noexcept
		{
			if (Index >= Count)
			{
				return T();
			}

			return Elements[Index];
		}

		inline const T& operator[](uint32_t Index) const noexcept
		{
			if (Index >= Count)
			{
				return T();
			}

			return Elements[Index];
		}

		//For T is primitive type
		inline int32_t Find(T Value) noexcept
		{
			for (uint32_t i = 0; i < Count; i++)
			{
				if (Elements[i] == Value)
				{
					return (int32_t)i;
				}
			}

			return -1;
		}

		//Pops an item at index.
		inline T& Pop(uint32_t Index) noexcept
		{
			if (Index >= Count)
			{
				return T();
			}

			T Value = Elements[Index];
			Elements[Index] = Elements[Count - 1];

			Count--;

			return Value;
		}

		inline T Front()noexcept
		{
			if (!Count)
			{
				return T();
			}

			return Elements[0];
		}

		inline T Back()noexcept
		{
			if (!Count)
			{
				return T();
			}

			return Elements[Count - 1];
		}

		inline const T Front() const noexcept
		{
			if (!Count)
			{
				return T();
			}

			return Elements[0];
		}

		inline const T Back()const noexcept
		{
			if (!Count)
			{
				return T();
			}

			return Elements[Count - 1];
		}

		inline bool IsEmpty() const noexcept
		{
			return !Count;
		}

		//Pops top item
		//Returns T() if empty
		inline std::optional<T> PopTop() noexcept
		{
			if (!Count)
			{
				return {};
			}

			Count--;
			return Elements[Count];
		}

		inline uint32_t GetCount() const noexcept
		{
			return Count;
		}

		inline uint32_t GetSize() const noexcept
		{
			return Size;
		}

		inline void Clear() noexcept
		{
			Count = 0;
		}

		inline T* GetRaw() noexcept
		{
			return Elements;
		}

		inline T* GetRawRemaining() noexcept
		{
			return Elements + Count;
		}

		//Cehcks if it fits Count elements.
		inline bool Fits(int32_t Count) const noexcept
		{
			return (Size - Count - this->Count) >= 0;
		}

		//How many elements can be added till it is full.
		inline int32_t GetEmptyCount() const noexcept
		{
			return Size - Count;
		}

		//Returns IsFull()
		inline bool Occupy(int32_t Count) noexcept
		{
			this->Count += Count;

			return IsFull();
		}

		//How many elements can be added till it is full.
		inline bool IsFull() const noexcept
		{
			return (Size == Count);
		}

		static TView<T> Empty;

	private:
		uint32_t			Size;
		uint32_t			Count{ 0 };
		T* PTR				Elements{ nullptr };
	};

	template<typename T>
	inline TView<T> TView<T>::Empty{};

	/*------------------------------------------------------------
		BlockView
	  ------------------------------------------------------------*/
	struct TBlockView
	{
		uint8_t* PTR	Data{ nullptr };
		size_t			Size{ 0 };

		TBlockView() {}
		TBlockView(uint8_t* Data, size_t Size) : Data(Data), Size(Size) {}
		TBlockView(const TBlockView& Other) : Data(Other.Data), Size(Other.Size) {}
		TBlockView(TBlockView&& Other) noexcept : Data(Other.Data), Size(Other.Size)
		{
			Other.Release();
		}

		~TBlockView() { Release(); }

		inline TBlockView& operator=(const TBlockView& Other)noexcept
		{
			Data = Other.Data;
			Size = Other.Size;

			return *this;
		}
		inline bool operator==(const TBlockView& Other) const noexcept
		{
			return Data == Other.Data && Size == Other.Size;
		}
		inline bool IsValid() const noexcept
		{
			return Data && Size;
		}
		inline void Release() noexcept
		{
			Data = nullptr;
			Size = 0;
		}

		const static TBlockView Empty;
	};
}

#include "Map.h"
#include "Queue.h"
#include "DQueue.h"
#include "PriorityQueue.h"
