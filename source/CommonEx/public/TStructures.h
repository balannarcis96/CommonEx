#pragma once
/**
 * @file TStructures.h
 *
 * @brief CommonEx Common structures abstractions:
 *			-TVector<T>
 *			-TArray<T, Size> -- if Size = 0 the size must be passed to the constructor
 *			-TStack<T>
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

namespace CommonEx {
	template <typename T, bool bIsFixed = false>
	class TStructureBase
	{
	public:
		using MyT = T;
		using MyTPtr = T*;

		static const uint32_t GrowStep = 32;

		TStructureBase() noexcept { Grow(GrowStep); }
		TStructureBase(const bool bStartEmpty) noexcept { if (!bStartEmpty) { Grow(GrowStep); } }
		TStructureBase(uint32_t Size) noexcept { Grow(Size); }

		TStructureBase(const TStructureBase& Other) noexcept {
			(*this) = Other;
		}
		TStructureBase(TStructureBase&& Other) noexcept {
			(*this) = std::move(Other);
		}

		~TStructureBase() {
			Destroy();
		}

		inline TStructureBase& operator=(const TStructureBase& Other) noexcept {
			if (this == &Other) {
				return *this;
			}

			if (Other.GetCount() > GetCapacity()) {
				Grow(Other.GetCount() - GetCapacity());
			}

			if constexpr (std::is_copy_constructible_v<MyT>) {
				for (uint32_t i = 0; i < Other.GetCount(); i++) {
					(*this)[i] = Other[i];
				}
			}
			else {
				RTRY_S_L((RStatus)memcpy_s(
					Front(),
					GetCapacity() * sizeof(T),
					Other.Front(),
					Other.GetCount() * sizeof(T)), *this, "TStructureBase::operator=(const TStructureBase& Other) failed to memcpy_s()") {}
			}

			Count = Other.GetCount();

			return *this;
		}

		inline TStructureBase& operator=(TStructureBase&& Other) noexcept {
			if (this == &Other) {
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

		inline bool CanFit(uint32_t FitElementsCount) const noexcept {
			return (GetCount() + FitElementsCount) <= GetCapacity();
		}

		template<bool bCallDestructors = true>
		inline void Clear() noexcept {
			if constexpr (std::is_destructible_v<T> && bCallDestructors) {
				//Call destructor of each element
				for (size_t i = 0; i < GetCount(); i++)
				{
					Front()[i].~T();
				}
			}

			Count = 0;
		}

		inline void Release() noexcept {
			Capacity = 0;
			Count = 0;
			Memory.Release();
		}

		template<bool bCallDestructors = true>
		inline void Destroy() noexcept {
			Clear<bCallDestructors>();
			Capacity = 0;
			Memory.Reset();
		}

		//inline bool Swap(uint32_t IndexFrom, uint32_t IndexWith) noexcept {
		//	if (GetCount() <= IndexFrom || GetCount() <= IndexWith) {
		//		return false;
		//	}

		//	//@TODO
		//}

		inline bool Push(const MyT& Item) noexcept {
			if (!CanFit(1)) {
				if constexpr (!bIsFixed) {
					RTRY_S_L(
						Grow(GrowStep)
						, false, "TStructureBase::Push(const MyT& Item) call to Grow(GrowStep) failed!"
					) {}
				}
				else {
					return false;
				}
			}

			Front()[Count++] = Item;

			return true;
		}

		inline bool Push(MyT&& Item) noexcept {
			if (!CanFit(1)) {
				if constexpr (!bIsFixed) {
					RTRY_S_L(
						Grow(GrowStep)
						, false, "TStructureBase::Push(MyT&& Item) call to Grow(GrowStep) failed!"
					) {}
				}
				else {
					return false;
				}
			}

			Front()[Count++] = std::move(Item);

			return true;
		}

		inline std::optional<MyT> Pop() noexcept {
			if (!GetCount()) {
				return std::nullopt;
			}

			MyTPtr OutItem = Back();
			Count--;

			return std::move(*OutItem);
		}

		inline MyT& operator[](uint32_t Index) noexcept {
			return Front()[Index];
		}

		inline const MyT& operator[](uint32_t Index) const noexcept {
			return Front()[Index];
		}

	protected:
		inline RStatus Grow(uint32_t AddCapacity) noexcept
		{
			MPtr<T> NewBlock = MemoryManager::AllocBuffer<T, true>(Capacity + AddCapacity);
			if (NewBlock.IsNull())
			{
				return RFail;
			}

			NewBlock.GetMemoryBlock()->ZeroMemoryBlock();

			const size_t BlockCapacity = NewBlock.GetCapacity();
			if (IsValid()) {
				MyTPtr TempFront = NewBlock.Ptr;

				RTRY_L_FMT((RStatus)memcpy_s(
					TempFront,
					BlockCapacity,
					Front(),
					Count * sizeof(MyT)), "_TStructureBase:: call to Grow({}) failed to memcpy_s()", AddCapacity) {}
			}

			Memory = std::move(NewBlock);
			Capacity = (uint32_t)(BlockCapacity / sizeof(MyT));

			return RSuccess;
		}

		MPtr<MyT>	Memory{ nullptr, nullptr };
		uint32_t	Capacity{ 0 };
		uint32_t	Count{ 0 };
	};

	template <typename T>
	using TVector = TStructureBase<T, false>;

	template <typename T>
	using TStack = TStructureBase<T, false>;

	template <typename T, uint32_t MaxCapacity = 0>
	struct TArray : TStructureBase<T, true>
	{
		using MyBase = TStructureBase<T, true>;

		TArray() noexcept : MyBase(MaxCapacity) {}
	};

	template <typename T>
	struct TArray<T, 0> : TStructureBase<T, true>
	{
		using MyBase = TStructureBase<T, true>;

		TArray(uint32_t Size) noexcept : MyBase(Size) {}
	};

	template <typename KeyType, typename T>
	struct TMap
	{
		//@TODO
	};

	/*------------------------------------------------------------
		TView simple array view
	  ------------------------------------------------------------*/
	template<typename T>
	struct TView {
		TView() :Size(0), Count(0), Elements(nullptr) {}
		TView(const uint32_t Size, T* View) :Size(Size), Count(0), Elements(View) {}
		TView(const uint32_t Size, const T* View) :Size(Size), Count(0), Elements((T*)View) {}
		TView(const uint32_t Size, const uint32_t Count, T* View) :Size(Size), Count(Count), Elements(View) {}

		TView(TView<T>& Other) : Size(Other.Size), Count(Other.Count), Elements(Other.Elements) {}
		TView(TStructureBase<T>& Other) : Size(Other.GetCapacity()), Count(Other.GetCount()), Elements(Other.GetFront()) {}
		TView(std::vector<T>& Other) : Size(Other.size()), Count(Other.size()), Elements((T*)Other.data()) {}

		inline TView<T>& operator=(const TView<T>& Other)noexcept {
			if (this == &Other) {
				return *this;
			}

			Size = Other.Size;
			Count = Other.Count;
			Elements = Other.Elements;

			return *this;
		}

		inline TView<T>& operator=(TStructureBase<T>& Other)noexcept {
			if (this == &Other) {
				return *this;
			}

			Size = Other.GetCapacity();
			Count = Other.GetCount();
			Elements = Other.GetFront();

			return *this;
		}

		//Pushees the item into a new slot.
		inline bool Push(T Item) noexcept {
			if (Count + 1 > Size)
			{
				return false;
			}

			Elements[Count++] = Item;
		}

		//Gets Element ptr at index.
		inline T& operator[](uint32_t Index) noexcept {
			if (Index >= Count)
			{
				return T();
			}

			return Elements[Index];
		}

		inline const T& operator[](uint32_t Index) const noexcept {
			if (Index >= Count)
			{
				return T();
			}

			return Elements[Index];
		}

		//For T is primitive type
		inline int32_t Find(T Value) noexcept {
			for (uint32_t i = 0; i < Count; i++)
			{
				if (Elements[i] == Value) {
					return (int32_t)i;
				}
			}

			return -1;
		}

		//Pops an item at index.
		inline T& Pop(uint32_t Index) noexcept {
			if (Index >= Count)
			{
				return T();
			}

			T Value = Elements[Index];
			Elements[Index] = Elements[Count - 1];

			Count--;

			return Value;
		}

		inline T Front()noexcept {
			if (!Count) {
				return T();
			}

			return Elements[0];
		}

		inline T Back()noexcept {
			if (!Count) {
				return T();
			}

			return Elements[Count - 1];
		}

		inline const T Front() const noexcept {
			if (!Count) {
				return T();
			}

			return Elements[0];
		}

		inline const T Back()const noexcept {
			if (!Count) {
				return T();
			}

			return Elements[Count - 1];
		}

		inline bool IsEmpty() const noexcept {
			return !Count;
		}

		//Pops top item
		//Returns T() if empty
		inline std::optional<T> PopTop() noexcept {
			if (!Count) {
				return {};
			}

			Count--;
			return Elements[Count];
		}

		inline uint32_t GetCount() const noexcept {
			return Count;
		}

		inline uint32_t GetSize() const noexcept {
			return Size;
		}

		inline void Clear() noexcept {
			Count = 0;
		}

		inline T* GetRaw() noexcept {
			return Elements;
		}

		inline T* GetRawRemaining() noexcept {
			return Elements + Count;
		}

		//Cehcks if it fits Count elements.
		inline bool Fits(int32_t Count) const noexcept {
			return (Size - Count - this->Count) >= 0;
		}

		//How many elements can be added till it is full.
		inline int32_t GetEmptyCount() const noexcept {
			return Size - Count;
		}

		//Returns IsFull()
		inline bool Occupy(int32_t Count) noexcept {
			this->Count += Count;

			return IsFull();
		}

		//How many elements can be added till it is full.
		inline bool IsFull() const noexcept {
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
	struct TBlockView {
		uint8_t* PTR	Data{ nullptr };
		size_t			Size{ 0 };

		TBlockView() {}
		TBlockView(uint8_t* Data, size_t Size) : Data(Data), Size(Size) {}
		TBlockView(const TBlockView& Other) : Data(Other.Data), Size(Other.Size) {}
		TBlockView(TBlockView&& Other) noexcept : Data(Other.Data), Size(Other.Size) {
			Other.Release();
		}

		~TBlockView() { Release(); }

		inline TBlockView& operator=(const TBlockView& Other)noexcept {
			Data = Other.Data;
			Size = Other.Size;

			return *this;
		}
		inline bool operator==(const TBlockView& Other) const noexcept {
			return Data == Other.Data && Size == Other.Size;
		}
		inline bool IsValid() const noexcept {
			return Data && Size;
		}
		inline void Release() noexcept {
			Data = nullptr;
			Size = 0;
		}

		const static TBlockView Empty;
	};
}