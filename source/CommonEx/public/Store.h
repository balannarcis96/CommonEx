#pragma  once
/*------------------------------------------------------------
	DataStore.h
		Base class for all stores
  ------------------------------------------------------------*/

namespace CommonEx
{
	template<typename T>
	struct DataStore
	{
		const size_t GetSize() const noexcept { return Store.size(); }

		void SetVerbose(bool Value) noexcept { Verbose = Value; }
		bool IsVerbose() const noexcept { return Verbose; }

	protected:
		bool				Verbose = false;
		std::vector<T*>		Store;
	};

	template<typename T, size_t Size>
	struct FixedLengthDataStoreBase
	{
		constexpr size_t GetSize() const noexcept { return Size; }

		void SetVerbose(bool Value) noexcept { Verbose = Value; }
		bool IsVerbose() const noexcept { return Verbose; }

	protected:
		bool				Verbose = false;
		T* PTR				Store[Size];
	};
}