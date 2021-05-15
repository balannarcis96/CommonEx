#pragma once

/*
	Example of KEY and IV code
	41 C7 43 B8 [1C 01 C9 04]
	41 C7 43 BC [FF 76 FF 06]
	41 C7 43 C0 [C2 11 18 7E]
	41 C7 43 C4 [19 7B 57 16]
	41 C7 43 A8 [39 6C 34 2C]
	41 C7 43 AC [52 A0 C1 2D]
	41 C7 43 B0 [51 1D D0 20]
	41 C7 43 B4 [9F 90 CA 7D]

	41 C7 43 ? [? ? ? ?]
	41 C7 43 ? [? ? ? ?]
	41 C7 43 ? [? ? ? ?]
	41 C7 43 ? [? ? ? ?]
	41 C7 43 ? [? ? ? ?]
	41 C7 43 ? [? ? ? ?]
	41 C7 43 ? [? ? ? ?]
	41 C7 43 ? [? ? ? ?]

	Sequence to search by:
	41 C7 43 ? ? ? ? ? 41 C7 43 ? ? ? ? ? 41 C7 43 ? ? ? ? ? 41 C7 43 ? ? ? ? ? 41 C7 43 ? ? ? ? ? 41 C7 43 ? ? ? ? ? 41 C7 43 ? ? ? ? ? 41 C7 43 ? ? ? ? ?
*/

namespace Datacenter {
#define PTR

	namespace Structure {
		template<bool bForServer, bool bFor64>
		using TBlockIndices = typename SerialSizesAndTypes<bForServer, bFor64>::TBlockIndices;

		template<bool bForServer, bool bFor64>
		using BlockIndexType = typename SerialSizesAndTypes<bForServer, bFor64>::BlockIndexType;

		template<bool bForServer, bool bFor64>
		using NameIndexType = typename SerialSizesAndTypes<bForServer, bFor64>::NameIndexType;

		//Constants
		constexpr uint16_t ELEMENT_INDEX_VALUE_MASK = 0xFFF0;
		constexpr uint16_t ELEMENT_INDEX_FLAGS_MASK = 0x000F;
		constexpr uint16_t ELEMENT_INDEX_ENABLED_MASK = 0xFFFE;

		constexpr size_t AdditionalValueBufferSize = 65;

		//Enums
		enum AttributeType : uint16_t {
			AttributeType_Invalid = 0,
			AttributeType_Int = 1,
			AttributeType_Float = 2,
			AttributeType_String = 3,
		};

#pragma region FOR_REBUILDING_FROM_RAW

		enum class RawElementType {
			Element,
			Comment,
		};

		struct AttributeItemRaw {
			std::wstring				Name;
			std::wstring				Value;
			uint64_t					Hash = 0;
			union {
				uint16_t				Type = 0;
				ServerAttributeType		ServerType;
			};

			AttributeItemRaw() {}
			AttributeItemRaw(AttributeItemRaw&& other) noexcept  {
				Name = std::move(other.Name);
				Value = std::move(other.Value);
				Type = other.Type;
				Hash = other.Hash;
			}
			AttributeItemRaw(const AttributeItemRaw&) = delete;
			AttributeItemRaw& operator=(const AttributeItemRaw&) = delete;
			AttributeItemRaw& operator=(AttributeItemRaw&& other)noexcept {
				if (&other == this) {
					return *this;
				}

				Name = std::move(other.Name);
				Value = std::move(other.Value);
				Type = other.Type;
				Hash = other.Hash;

				return *this;
			}

			void BuildHash() {
				Hash = ((uint64_t)Utils::HashUtil::CRC32_CaseSensitive(Name.c_str()) << 32) | (((uint64_t)Utils::HashUtil::CRC32_CaseSensitive(Value.c_str()) << 3) | (Type & 3));
			}
			uint64_t GetHash() const {
				return Hash;
			}
		};

		template<bool bForServer, bool bFor64>
		struct ElementItemRaw {
			std::wstring					Name;

			std::vector<AttributeItemRaw>	Attributes;
			std::vector<ElementItemRaw*>	Children;

			ElementItemRaw* PTR				DuplicateOf = nullptr;
			ElementItemRaw* PTR				Parent = nullptr;

			//For server (elements that dont have children but values)
			std::wstring					Value;
			bool							IsValueElement = false;

			//For building the DC
			uint64_t						Hash = 0;

			//Start with 0 references (not part of a tree)
			int32_t							ReferenceCount = 0;

			RawElementType					Type = RawElementType::Element;

			TBlockIndices<bForServer, bFor64> CachedDCElementIndices = { 0xFFFF,0xFFFF };

			bool							Used = false;

			std::wstring					CachedFileName;

			ElementItemRaw() {}
			ElementItemRaw(ElementItemRaw&& other) = delete;
			ElementItemRaw(const ElementItemRaw&) = delete;
			ElementItemRaw& operator=(const ElementItemRaw&) = delete;
			ElementItemRaw& operator=(ElementItemRaw&& other) = delete;

			~ElementItemRaw() {
				for (size_t i = 0; i < Children.size(); i++)
				{
					if (!Children[i] || Children[i] == this) {
						continue;
					}

					if (Children[i]->RemoveReference()) {
						delete Children[i];
					}

					Children[i] = nullptr;
				}

				Children.clear();
			}

			//reference counting, for shared elements
			void AddReference() { ReferenceCount++; }
			bool RemoveReference() {
				ReferenceCount -= 1;
				if (ReferenceCount == 0) {
					return true;
				}

				return false;
			}

			bool HasValidDCIndices() const {
				return *(int32_t*)(&CachedDCElementIndices) != -1;
			}

			void BuildHash() {
				if (Hash) {
					return;
				}

				Hash = ((uint64_t)Utils::HashUtil::CRC32_CaseInsensitive(Name.c_str()) << 32) | (uint64_t)(0x9e3779b9);
			}

			uint64_t GetHash() const {
				return Hash;
			}

			uint64_t BuildKey() const {
				uint64_t ToReturn;

				ToReturn = Hash;

				if (Attributes.size()) {
					for (const auto& attr : Attributes) {
						Utils::CombineHash(ToReturn, attr.GetHash());
					}
				}

				if (Children.size()) {
					for (const auto* child : Children) {
						Utils::CombineHash(ToReturn, child->GetHash());
					}
				}

				return ToReturn;
			}

			void BuildPath(std::wstring& OutPath) const {
				OutPath = Name;

				auto Cursor = Parent;
				while (Cursor) {
					OutPath.insert(OutPath.begin(), L'.');
					OutPath.insert(0, Cursor->Name.c_str());

					Cursor = Cursor->Parent;
				}
			}
			std::wstring BuildAttributePath(const std::wstring& AttributeName) const {
				std::wstring BasePath;
				BuildPath(BasePath);

				BasePath += L"@";
				BasePath += AttributeName;

				return std::move(BasePath);
			}
		};

#pragma endregion

		//Generic Types
		template<typename T, size_t BasePadd = 0>
		struct DCArray {
			std::vector<T>		Data;
			uint32_t			Count = 0;

			DCArray() {}

			//Just moving
			DCArray(DCArray&& other)noexcept {
				Data = std::move(other.Data);
				Count = other.Count;

				other.Count = 0;
			}
			DCArray& operator =(DCArray&& other) {
				Data = std::move(other.Data);
				Count = other.Count;

				other.UsedCount = 0;
			}

			//No copy
			DCArray(const DCArray&) = delete;
			DCArray& operator=(const DCArray&) = delete;

			bool Serialize(FIStream& Stream) {
				if (Stream.IsLoading()) {
					Count = Stream.ReadUInt32() - BasePadd;

					Data.reserve(Count);

					for (size_t i = 0; i < Count; i++)
					{
						T Object;

						if (!Object.Serialize(Stream)) {
							continue;
						}

						Data.push_back(std::move(Object));
					}
				}
				else {
					Stream.WriteUInt32(Count + BasePadd);

					for (size_t i = 0; i < Data.size(); i++)
					{
						if (!Data[i].Serialize(Stream)) {
							return false;
						}
					}
				}

				return true;
			}

			void Clear() {
				Data.clear();
				Count = 0;
			}
		};

		template<typename T, size_t ElementSize>
		struct DCBlockArray {
			std::vector<T>		Data;
			uint32_t			UsedCount = 0;
			uint32_t			MaxCount = 0;

			DCBlockArray() {}

			//Just moving
			DCBlockArray(DCBlockArray&& other) {
				Data = std::move(other.Data);
				UsedCount = other.UsedCount;
				MaxCount = other.MaxCount;

				other.UsedCount = 0;
				other.MaxCount = 0;
			}
			DCBlockArray& operator =(DCBlockArray&& other) {
				if (this == &other) {
					return *this;
				}

				Data = std::move(other.Data);
				MaxCount = other.MaxCount;
				UsedCount = other.UsedCount;

				other.UsedCount = 0;
				other.MaxCount = 0;

				return *this;
			}

			//No copy
			DCBlockArray(const DCBlockArray& other) = delete;
			DCBlockArray& operator=(const DCBlockArray& other) = delete;

			bool Serialize(FIStream& Stream) {
				if (Stream.IsLoading()) {
					MaxCount = Stream.ReadUInt32();
					UsedCount = Stream.ReadUInt32();

					Data.reserve(UsedCount);

					for (size_t i = 0; i < UsedCount; i++)
					{
						T Object;

						if (!Object.Serialize(Stream)) {
							continue;
						}

						Data.push_back(std::move(Object));
					}
				}
				else {
					Stream.WriteUInt32(MaxCount);
					Stream.WriteUInt32(UsedCount);

					for (size_t i = 0; i < UsedCount; i++)
					{
						if (!Data[i].Serialize(Stream)) {
							return false;
						}
					}
				}

				const size_t Remaining = MaxCount - UsedCount;
				if (Stream.IsLoading())
				{
					if (Remaining) {
						Stream._pos += Remaining * ElementSize;
					}
				}
				else {
					T DefaultObject;

					for (size_t i = 0; i < Remaining; i++) {
						if (!DefaultObject.Serialize(Stream)) {
							return false;
						}
					}
				}

				return true;
			}

			void Clear() {
				Data.clear();
				UsedCount = 0;
				MaxCount = 0;
			}

			bool CanWrite(uint32_t Count = 1)const {
				return MaxCount - UsedCount >= Count;
			}
		};

		template<typename T>
		struct DCBuckets {
			uint32_t									Count = 0;
			std::vector<T>								Buckets;

			bool Serialize(FIStream& Stream) {
				if (!Count) {
					return true;
				}

				if (Stream.IsLoading()) {
					Buckets.reserve(Count);

					for (size_t i = 0; i < Count; i++)
					{
						T Bucket;

						if (!Bucket.Serialize(Stream)) {
							return false;
						}

						Buckets.push_back(std::move(Bucket));
					}
				}
				else {
					for (size_t i = 0; i < Count; i++)
					{
						if (!Buckets[i].Serialize(Stream)) {
							return false;
						}
					}
				}

				return true;
			}

			void Clear() {
				Buckets.clear();
			}
		};

		//Structure types
		struct StringBlock {

			std::unique_ptr<wchar_t>	Block = nullptr;
			int32_t						BlockSize = 0;
			int32_t						UsedSize = 0;

			StringBlock() {}
			StringBlock(StringBlock&& other)noexcept {
				Block.reset(other.Block.release());
				BlockSize = other.BlockSize;
				UsedSize = other.UsedSize;
			}

			bool Serialize(FIStream& Stream) {
				if (Stream.IsLoading()) {
					BlockSize = Stream.ReadInt32();
					UsedSize = Stream.ReadInt32();

					Block.reset(new wchar_t[BlockSize]);

					Stream.Read((uint8_t*)Block.get(), (size_t)BlockSize * 2);
				}
				else {
					Stream.WriteInt32(BlockSize);
					Stream.WriteInt32(UsedSize);

					Stream.Write((uint8_t*)Block.get(), (size_t)BlockSize * 2);
				}

				return true;
			};

			//Returns true and index into block where the inserted string starts
			//Returns false if is full
			template<bool bForServer, bool bFor64>
			bool InsertString(const wchar_t* String, size_t StringSize, BlockIndexType<bForServer, bFor64>& OutIndex) {
				const auto Remaining = BlockSize - UsedSize;

				//Check if it can fit string
				if (Remaining < StringSize + 1) {
					return false;
				}

				wchar_t* ToReturn = Block.get() + UsedSize;

				OutIndex = (BlockIndexType<bForServer, bFor64>)UsedSize;

				if (StringSize) {
					memcpy_s(ToReturn, sizeof(wchar_t) * StringSize, String, sizeof(wchar_t) * StringSize);

					UsedSize += StringSize;
				}

				Block.get()[UsedSize] = L'\0';

				UsedSize++;

				return ToReturn;
			}

			bool AllocateBlock(size_t BlockSize = CStringsBlockSize) {
				Block.reset(new wchar_t[BlockSize]);

				if (Block.get()) {
					this->BlockSize = (int32_t)BlockSize;
					memset(Block.get(), 0, sizeof(wchar_t) * BlockSize);
				}

				return Block.get() != nullptr;
			}

			void Clear() {
				Block.reset();
				BlockSize = 0;
				UsedSize = 0;
			}
		};

		//Using
		using DCStringBlocks = DCArray<StringBlock>;

		template<bool bForServer, bool bFor64>
		struct BucketElement {
			using MyTBlockIndices = TBlockIndices<bForServer, bFor64>;

			uint32_t			Hash{ 0 };
			uint32_t			Length{ 0 };
			uint32_t			Id{ 0 };
			MyTBlockIndices		Indices{ 0 ,0 };

			//Ref
			const wchar_t* StringRef = nullptr;

			BucketElement() {
				Hash = 0;
				Length = 0;
				Id = 0;
				Indices = { 0,0 };
			}
			BucketElement(const BucketElement& Other) {
				Hash = Other.Hash;
				Length = Other.Length;
				Id = Other.Id;
				Indices = Other.Indices;

				StringRef = Other.StringRef;
			}
			BucketElement& operator=(const BucketElement& Other) {
				if (&Other == this) {
					return *this;
				}

				Hash = Other.Hash;
				Length = Other.Length;
				Id = Other.Id;
				Indices = Other.Indices;

				StringRef = Other.StringRef;

				return *this;
			}

			bool Serialize(FIStream& Stream) {
				if (Stream.IsLoading()) {
					Hash = Stream.ReadUInt32();
					Length = Stream.ReadUInt32();
					Id = Stream.ReadUInt32();
					/*	Indices.first = Stream.ReadUInt16();
						Indices.second = Stream.ReadUInt16();*/
					Indices = Stream.ReadT<MyTBlockIndices>();
				}
				else {
					Stream.WriteUInt32(Hash);
					Stream.WriteUInt32(Length);
					Stream.WriteUInt32(Id);

					/*	Stream.WriteUInt16(Indices.first);
						Stream.WriteUInt16(Indices.second);*/

					Stream.WriteT(Indices);
				}

				return true;
			}
		};

		template<bool bForServer, bool bFor64>
		struct StringsBucket {
			using MyBucketElement = BucketElement<bForServer, bFor64>;

			uint32_t									Count = 0;
			std::vector<MyBucketElement>				Elements;

			//Ref
			DCStringBlocks* BlocksRef = nullptr;

			StringsBucket() {}
			StringsBucket(StringsBucket&& other) {
				Count = other.Count;
				Elements = std::move(other.Elements);
				BlocksRef = other.BlocksRef;

				other.Count = 0;
			}
			StringsBucket& operator=(StringsBucket&& other) {
				if (&other == this) {
					return *this;
				}

				Count = other.Count;
				Elements = std::move(other.Elements);
				BlocksRef = other.BlocksRef;

				other.Count = 0;

				return *this;
			}

			//No copy
			StringsBucket(const StringsBucket& other) = delete;
			StringsBucket& operator=(const StringsBucket& other) = delete;

			bool Serialize(FIStream& Stream) {
				if (Stream.IsLoading()) {
					Count = Stream.ReadUInt32();

					Elements.reserve(Count);

					for (size_t i = 0; i < Count; i++)
					{
						MyBucketElement Element;

						if (!Element.Serialize(Stream)) {
							return false;
						}

						Elements.push_back(std::move(Element));
					}
				}
				else {
					Stream.WriteUInt32(Count);

					for (size_t i = 0; i < Count; i++)
					{
						if (!Elements[i].Serialize(Stream)) {
							return false;
						}
					}
				}

				return true;
			}

			void Clear() {
				Count = 0;
				Elements.clear();
			}

			const wchar_t* GetElementByHash(const uint32_t Hash) const {
				if (!Count) {
					return nullptr;
				}

				for (size_t i = 0; i < Count; i++)
				{
					if (Elements[i].Hash == Hash) {
						return Elements[i].StringRef;
					}
				}

				return nullptr;
			}
		};

		template<bool bForServer, bool bFor64>
		struct StringEntry {
			using MyTBlockIndices = TBlockIndices<bForServer, bFor64>;

			MyTBlockIndices					Indices = { 0,0 };

			//Ref
			const wchar_t* PTR				StringRef = nullptr;

			std::vector<MyTBlockIndices>	RefElements;
			std::vector<MyTBlockIndices>	RefAttributes;
			std::vector<MyTBlockIndices>	RefIndices;

			bool Serialize(FIStream& Stream) {
				if (Stream.IsLoading()) {
					/*	Indices.first = Stream.ReadUInt16();
						Indices.second = Stream.ReadUInt16();*/

					Indices = Stream.ReadT<MyTBlockIndices>();
				}
				else {
					/*	Stream.WriteUInt16(Indices.first);
						Stream.WriteUInt16(Indices.second);*/

					Stream.WriteT(Indices);
				}

				return true;
			}
		};

		//Unknown (unused) structure
		struct UnkTree {
			struct Unk3Unk1_Item {
				uint32_t	Unk1 = 0;
				uint32_t	Unk2 = 0;
				uint32_t	Unk3 = 0;

				bool Serialize(FIStream& Stream) {
					if (Stream.IsLoading()) {
						Unk1 = Stream.ReadUInt32();
						Unk2 = Stream.ReadUInt32();
						Unk3 = Stream.ReadUInt32();
					}
					else {
						Stream.WriteUInt32(Unk1);
						Stream.WriteUInt32(Unk2);
						Stream.WriteUInt32(Unk3);
					}

					return true;
				};
				void operator = (const Unk3Unk1_Item& other) {
					Unk1 = other.Unk1;
					Unk2 = other.Unk2;
					Unk3 = other.Unk3;
				}
			};

			struct Unk3Unk3Unk1_Item {
				uint16_t	Unk1 = 0;
				uint16_t	Unk2 = 0;
				uint32_t	Unk3 = 0;

				DCArray<Unk3Unk1_Item>	Unk4;
				DCArray<Unk3Unk3Unk1_Item>	Unk5;

				bool Serialize(FIStream& Stream) {
					if (Stream.IsLoading()) {
						Unk1 = Stream.ReadUInt16();
						Unk2 = Stream.ReadUInt16();
						Unk3 = Stream.ReadUInt32();
					}
					else {
						Stream.WriteUInt16(Unk1);
						Stream.WriteUInt16(Unk2);
						Stream.WriteUInt32(Unk3);
					}

					if (!Unk4.Serialize(Stream)) {
						return false;
					}

					if (!Unk5.Serialize(Stream)) {
						return false;
					}

					return true;
				};
				void operator = (const Unk3Unk1_Item& other) {
					Unk1 = other.Unk1;
					Unk2 = other.Unk2;
					Unk3 = other.Unk3;
				}
			};

			uint16_t							Unk1 = 0;
			uint16_t							Unk2 = 0;
			uint32_t							Unk3 = 0;
			DCArray<Unk3Unk1_Item>				Unk3Unk1;
			DCArray<Unk3Unk3Unk1_Item>			Unk3Unk3;

			void Clear() {
				Unk1 = 0;
				Unk2 = 0;
				Unk3 = 0;
				Unk3Unk1.Clear();
				Unk3Unk3.Clear();
			}

			bool Serialize(FIStream& Stream) {
				if (Stream.IsLoading()) {

					Unk1 = Stream.ReadUInt16();
					Unk2 = Stream.ReadUInt16();
					Unk3 = Stream.ReadUInt32();

				}
				else {
					Stream.WriteUInt16(Unk1);
					Stream.WriteUInt16(Unk2);
					Stream.WriteUInt32(Unk3);
				}

				if (!Unk3Unk1.Serialize(Stream)) {
					return false;
				}

				if (!Unk3Unk3.Serialize(Stream)) {
					return false;
				}

				return true;
			}
		};

		//Datacenter index
		struct DCIndex {
			union {
				struct {
					uint16_t Key1;
					uint16_t Key2;
					uint16_t Key3;
					uint16_t Key4;
				};
				uint64_t Key = 0;
			};

			//Ref
			const wchar_t* Name1 = nullptr;
			const wchar_t* Name2 = nullptr;
			const wchar_t* Name3 = nullptr;
			const wchar_t* Name4 = nullptr;

			//Utf16 cache
			std::wstring			Name1Cached;
			std::wstring			Name2Cached;
			std::wstring			Name3Cached;
			std::wstring			Name4Cached;

			//Utf8 cache
			std::string				Name1Utf8;
			std::string				Name2Utf8;
			std::string				Name3Utf8;
			std::string				Name4Utf8;

			//Info
			int32_t					ElementsCount = 0;

			DCIndex() {}
			DCIndex(const DCIndex& other) {
				Key1 = other.Key1;
				Key2 = other.Key2;
				Key3 = other.Key3;
				Key4 = other.Key4;
			}
			bool Serialize(FIStream& Stream) {
				if (Stream.IsLoading()) {
					Key = Stream.ReadUInt64();
				}
				else {
					Stream.WriteU_int64(Key);
				}

				return true;
			}
			void operator = (const DCIndex& other) {
				Key1 = other.Key1;
				Key2 = other.Key2;
				Key3 = other.Key3;
				Key4 = other.Key4;
			}

			const char* GetName1Utf8() {
				if (Key1 && !Name1Utf8.length()) {
					char buffer[1024];

					if (_WideCharToMultiByte(Name1, buffer, 1024)) {
						Name1Utf8 = buffer;
					}
				}

				return Name1Utf8.c_str();
			}
			const char* GetName2Utf8() {
				if (Key2 && !Name2Utf8.length()) {
					char buffer[1024];

					if (_WideCharToMultiByte(Name2, buffer, 1024)) {
						Name2Utf8 = buffer;
					}
				}

				return Name2Utf8.c_str();
			}
			const char* GetName3Utf8() {
				if (Key3 && !Name3Utf8.length()) {
					char buffer[1024];

					if (_WideCharToMultiByte(Name3, buffer, 1024)) {
						Name3Utf8 = buffer;
					}
				}

				return Name3Utf8.c_str();
			}
			const char* GetName4Utf8() {
				if (Key4 && !Name4Utf8.length()) {
					char buffer[1024];

					if (_WideCharToMultiByte(Name4, buffer, 1024)) {
						Name4Utf8 = buffer;
					}
				}

				return Name4Utf8.c_str();
			}
		};

		struct CachedElementIndex {
			//For saving
			const wchar_t* Name1 = nullptr;
			const wchar_t* Name2 = nullptr;
			const wchar_t* Name3 = nullptr;
			const wchar_t* Name4 = nullptr;
			const wchar_t* ElementNameCacheRef = nullptr;

			//For building
			std::wstring ElementNameCache;
			DCIndex* IndexCache = nullptr;

			//Shared
			uint16_t	Index = 0;
			uint16_t	Flags = 0;

			CachedElementIndex() {}
			CachedElementIndex(CachedElementIndex&& other)noexcept {
				Name1 = other.Name1;
				Name2 = other.Name2;
				Name3 = other.Name3;
				Name4 = other.Name4;

				Index = other.Index;
				Flags = other.Flags;

				ElementNameCache = std::move(other.ElementNameCache);

				other.Name1 = nullptr;
				other.Name2 = nullptr;
				other.Name3 = nullptr;
				other.Name4 = nullptr;
				other.Index = 0;
				other.Flags = 0;
			}
			CachedElementIndex& operator=(CachedElementIndex&& other)noexcept {
				if (&other == this) {
					return *this;
				}

				Name1 = other.Name1;
				Name2 = other.Name2;
				Name3 = other.Name3;
				Name4 = other.Name4;

				Index = other.Index;
				Flags = other.Flags;

				ElementNameCache = std::move(other.ElementNameCache);

				other.Name1 = nullptr;
				other.Name2 = nullptr;
				other.Name3 = nullptr;
				other.Name4 = nullptr;
				other.Index = 0;
				other.Flags = 0;

				return *this;
			}

			//No copy
			CachedElementIndex(const CachedElementIndex&) = delete;
			CachedElementIndex& operator=(const CachedElementIndex&) = delete;
		};

		//Usings
		template<typename T>
		using DCPaddedArray = DCArray<T, 1>;

		template<bool bForServer, bool bFor64>
		using DCStringsBuckets = DCBuckets<StringsBucket<bForServer, bFor64>>;

		//Map
		template<bool bForServer, bool bFor64>
		struct DCMap {
			using MyStringEntry = StringEntry<bForServer, bFor64>;
			using MyDCStringsBuckets = DCStringsBuckets<bForServer, bFor64>;

			using MyBlockIndexType = BlockIndexType<bForServer, bFor64>;
			using MyNameIndexType = NameIndexType<bForServer, bFor64>;
			using MyTBlockIndices = TBlockIndices<bForServer, bFor64>;

			using MyBucketElement = BucketElement<bForServer, bFor64>;

			DCStringBlocks					StringBlocks;
			MyDCStringsBuckets				Buckets;
			DCPaddedArray<MyStringEntry>	AllStrings;

			//For rebuilding
			std::unordered_map<std::wstring, MyNameIndexType> PresentStrings;
			std::unordered_map<std::wstring, MyTBlockIndices> PresentStringsBig;

			DCMap() {}

			bool Serialize(FIStream& Stream) {
				if (!StringBlocks.Serialize(Stream)) {
					return false;
				}

				if constexpr (!bForServer) {
					if (!Buckets.Serialize(Stream)) {
						return false;
					}
				}

				if (!AllStrings.Serialize(Stream)) {
					return false;
				}

				return true;
			}

			const wchar_t* GetString(MyBlockIndexType BlockIndex, MyBlockIndexType StringIndex) {
				const wchar_t* Block = StringBlocks.Data[BlockIndex].Block.get();

				return &Block[StringIndex];
			}
			const wchar_t* GetString(MyTBlockIndices indices) {
				return GetString(indices.first, indices.second);
			}

			bool Prepare() {
				for (auto& Bucket : Buckets.Buckets) {
					for (auto& BucketElement : Bucket.Elements) {
						//BucketElement.StringRef = &(StringBlocks.Data[BucketElement.Indices.first].Block.get())[BucketElement.Indices.second];
						BucketElement.StringRef = GetString(BucketElement.Indices);
					}
				}

				for (auto& StringEntry : AllStrings.Data) {
					if (StringEntry.Indices.first == UINT16_MAX || StringEntry.Indices.second == UINT16_MAX) {
						StringEntry.StringRef = nullptr;
					}
					else {
						//StringEntry.StringRef = &(StringBlocks.Data[StringEntry.Indices.first].Block.get())[StringEntry.Indices.second];
						StringEntry.StringRef = GetString(StringEntry.Indices);
					}
				}

				return true;
			}

			void Clear() {
				StringBlocks.Clear();
				Buckets.Clear();
				AllStrings.Clear();
			}

			bool InsertString(const wchar_t* String, size_t StringSize, MyNameIndexType& StringId) {
				auto Item = PresentStrings.find(String);
				if (Item != PresentStrings.end()) {
					StringId = Item->second;
					return true;
				}

				MyTBlockIndices Indices;
				if (!InsertStringInBlock(String, StringSize, Indices)) {
					return false;
				}

				AllStrings.Data.push_back(MyStringEntry());

				//return 1 based index
				StringId = (MyNameIndexType)(AllStrings.Data.size());

				AllStrings.Count++;
				AllStrings.Data.back().Indices = Indices;
				AllStrings.Data.back().StringRef = GetString(Indices.first, Indices.second);

				PresentStrings.insert({ String, StringId });

				return true;
			}

			bool InsertString(const wchar_t* String, size_t StringSize, MyTBlockIndices& OutIndices) {
				auto Item = PresentStringsBig.find(String);
				if (Item != PresentStringsBig.end()) {
					OutIndices = Item->second;
					return true;
				}

				if (!InsertStringInBlock(String, StringSize, OutIndices)) {
					return false;
				}

				AllStrings.Data.push_back(MyStringEntry());

				AllStrings.Count++;
				AllStrings.Data.back().Indices = OutIndices;
				AllStrings.Data.back().StringRef = GetString(OutIndices.first, OutIndices.second);

				//StringId = (WORD)(AllStrings.Count);

				PresentStringsBig.insert({ String, OutIndices });

				return true;
			}

			MyNameIndexType FindString(const wchar_t* string)const {
				for (size_t i = 0; i < AllStrings.Data.size(); i++) {
					if (!wcscmp(string, AllStrings.Data[i].StringRef)) {
						return (MyNameIndexType)(i + 1);
					}
				}

				return 0;
			}

			/// <summary>
			/// Attempts to find the [string] in the cached [PresentStrings] map
			/// </summary>
			MyNameIndexType FindStringEx(const wchar_t* string) const {
				const auto item = PresentStrings.find(string);
				if (item == PresentStrings.end()) {
					return 0;
				}

				return item->second;
			}

			bool BuildHashTables() {
				const uint32_t Mask = Buckets.Count - 1;
				uint32_t Id = 1;

				for (auto& StringEntry : AllStrings.Data) {
					const uint32_t Hash = Utils::HashUtil::CRC32_CaseSensitive(StringEntry.StringRef);
					const uint32_t Index = (Hash >> 16 ^ Hash) & Mask;

					Buckets.Buckets[Index].Elements.push_back(MyBucketElement());
					Buckets.Buckets[Index].Count++;

					MyBucketElement& HashmapElement = Buckets.Buckets[Index].Elements.back();

					HashmapElement.StringRef = StringEntry.StringRef;
					HashmapElement.Indices = StringEntry.Indices;
					HashmapElement.Id = Id++;
					HashmapElement.Hash = Hash;
					HashmapElement.Length = wcslen(StringEntry.StringRef) + 1;
				}

				return true;
			}

			void CachePresentStrings() {
				for (size_t i = 0; i < AllStrings.Data.size(); i++)
				{
					PresentStrings.insert({ AllStrings.Data[i].StringRef , (MyNameIndexType)(i + 1) });
				}
			}

		private:
			bool InsertStringInBlock(const wchar_t* String, size_t StringSize, MyTBlockIndices& OutIndices) {
				for (size_t i = 0; i < StringBlocks.Data.size(); i++) {
					if (StringBlocks.Data[i].InsertString<bForServer, bFor64>(String, StringSize, OutIndices.second)) {

						OutIndices.first = (MyBlockIndexType)(i);

						return true;
					}
				}

				StringBlocks.Data.push_back(std::move(StringBlock()));
				StringBlocks.Count++;

				if (!StringBlocks.Data.back().AllocateBlock()) {
					Message("StringBlock::Failed to allocate block!");
					return false;
				}

				OutIndices.first = (MyBlockIndexType)(StringBlocks.Data.size() - 1);

				return StringBlocks.Data.back().InsertString<bForServer, bFor64>(String, StringSize, OutIndices.second);
			}
		};

		template<bool bForServer, bool bFor64>
		struct ElementItem;

		template<bool bForServer, bool bFor64>
		struct AttributeItem {
			using MyBlockIndexType = BlockIndexType<bForServer, bFor64>;
			using MyTBlockIndices = TBlockIndices<bForServer, bFor64>;
			using MyNameIndexType = NameIndexType<bForServer, bFor64>;

			MyNameIndexType					NameId = 0;
			union {
				uint16_t					TypeInfo = AttributeType_Invalid;
				ServerAttributeType			ServerTypeInfo;
			};

			union {
				MyTBlockIndices				Indices;
				float						FloatValue;
				int32_t						IntValue;
				int64_t						Int64Value{ 0 };
			};

			//For bFor64
			uint32_t						Padding = 0;

			//Cache
			std::wstring					StringyfiedValue;
			std::wstring					NameCache;
			MyTBlockIndices					LocationCache;

			//Utf8 cache | For local (In Memory) edititing
			std::string						Utf8NameCache;
			std::string						Utf8ValueCacheOriginal;
			std::vector<char>				Utf8ValueCache;

			//Utf16 cache | For local (In Memory) edititing
			std::wstring					Utf16NameCache;
			std::wstring					Utf16ValueCacheOriginal;
			std::vector<wchar_t>			Utf16ValueCache;

			//For Building
			bool							IsCloned = false;
			size_t							Index = 0;

			//Refs
			StringBlock* BlockRef = nullptr;
			const wchar_t* StringRef = nullptr;
			const wchar_t* NameRef = nullptr;

			AttributeItem() {
				Hash = 0;

#if DATACENTER_UI
				NewUIDs();
#endif
			}

			//Copy
			AttributeItem(const AttributeItem& other) {
				(*this) = other;
			}
			virtual AttributeItem& operator=(const AttributeItem& other) {
				if (this == &other) {
					return *this;
				}

				NameId = other.NameId;
				TypeInfo = other.TypeInfo;

				Int64Value = other.Int64Value;
#if DC_64
				Padding = other.Padding;
#endif
				BlockRef = other.BlockRef;
				StringRef = other.StringRef;
				NameRef = other.NameRef;

				NameCache = other.NameCache;
				LocationCache = other.LocationCache;
				Index = other.Index;
				StringyfiedValue = other.StringyfiedValue;
				Utf8ValueCacheOriginal = other.Utf8ValueCacheOriginal;
				Utf8ValueCache = other.Utf8ValueCache;
				IsCloned = other.IsCloned;

#if DATACENTER_UI
				NewUIDs();
#endif

				Hash = 0;

				return *this;
			}

			//Move
			AttributeItem(AttributeItem&& other) : IntValue(0) {
				(*this) = other;
			}
			AttributeItem& operator=(AttributeItem&& other) {
				if (this == &other) {
					return *this;
				}

				NameId = other.NameId;
				TypeInfo = other.TypeInfo;

				Int64Value = other.Int64Value;
#if DC_64
				Padding = other.Padding;
#endif
				BlockRef = other.BlockRef;
				StringRef = other.StringRef;
				NameRef = other.NameRef;

				NameCache = std::move(other.NameCache);
				LocationCache = other.LocationCache;
				Index = other.Index;
				StringyfiedValue = std::move(other.StringyfiedValue);
				Utf8ValueCacheOriginal = std::move(other.Utf8ValueCacheOriginal);
				Utf8ValueCache = std::move(other.Utf8ValueCache);
				IsCloned = other.IsCloned;

#if DATACENTER_UI
				uid1 = other.uid1;
				uid2 = other.uid2;
#endif

				//Invalidate other
				other.NameId = 0;
				other.TypeInfo = AttributeType_Invalid;

				Hash = 0;
				other.Hash = 0;

				return *this;
			}

			bool Serialize(FIStream& Stream) {
				if (Stream.IsLoading()) {
					NameId = Stream.ReadT<MyNameIndexType>();

					TypeInfo = Stream.ReadUInt16();

					if constexpr (!bForServer) {
						Indices = Stream.ReadT<MyTBlockIndices>();
					}
					else {
						Int64Value = Stream.ReadInt64();
					}

					if constexpr (bFor64 && !bForServer) {
						Padding = Stream.ReadUInt32(); //padding bytes
					}
				}
				else {
					Stream.WriteT(NameId);

					Stream.WriteUInt16(TypeInfo);

					if constexpr (!bForServer) {
						Stream.WriteT(Indices);
					}
					else {
						Stream.WriteT(Int64Value);
					}

					if constexpr (bFor64 && !bForServer) {
						Stream.WriteUInt32(Padding);
					}
				}

				return true;
			}

			bool CacheValue() {
				if (IsFloat()) {
					StringyfiedValue = std::move(std::to_wstring(FloatValue));
				}
				else if (IsInt()) {
					StringyfiedValue = std::move(std::to_wstring(IntValue));
				}
				else if (IsString()) {
					StringyfiedValue = std::move(std::wstring(StringRef));
				}
				else {
					Message("XmlNodeFromDCElement::Unknown attribute type %d", (INT)TypeInfo);
					return false;
				}

				return true;
			}

			std::wstring BuildPath(const ElementItem<bForServer, bFor64>* parent)const;

			//Type checks
			bool IsValid() const {
				if constexpr (bForServer) {
					return ServerTypeInfo != ServerAttributeType::Unknown;
				}
				else {
					return (TypeInfo & 3) != AttributeType_Invalid;
				}
			}
			bool IsInt() const {
				if constexpr (bForServer) {
					return ServerTypeInfo == ServerAttributeType::Int || ServerTypeInfo == ServerAttributeType::Int64;
				}
				else {
					return (TypeInfo & 3) == AttributeType_Int;
				}
			}
			bool IsFloat() const {
				if constexpr (bForServer) {
					return ServerTypeInfo == ServerAttributeType::Float;
				}
				else {
					return (TypeInfo & 3) == AttributeType_Float;
				}
			}
			bool IsString() const {
				if constexpr (bForServer) {
					return IsValid() && !IsInt() && !IsFloat();
				}
				else {
					return (TypeInfo & 3) == AttributeType_String;
				}
			}

			float GetFloat() const noexcept {
				if constexpr (bForServer) {
					if (IsFloat()) {
						return FloatValue;
					}
					else if (ServerTypeInfo == ServerAttributeType::Int) {
						return (float)IntValue;
					}
					else if (ServerTypeInfo == ServerAttributeType::Int64) {
						return (float)Int64Value;
					}
				}
				else {
					if (IsFloat()) {
						return FloatValue;
					}
					else if (IsInt()) {
						return (float)IntValue;
					}
				}

				return (float)_wtof(StringRef);
			}
			int32_t GetInt() const noexcept {
				if constexpr (bForServer) {
					if (IsFloat()) {
						return (int32_t)FloatValue;
					}
					else if (ServerTypeInfo == ServerAttributeType::Int) {
						return IntValue;
					}
					else if (ServerTypeInfo == ServerAttributeType::Int64) {
						return (int32_t)Int64Value;
					}
				}
				else {
					if (IsInt()) {
						return IntValue;
					}
					else if (IsFloat()) {
						return (int32_t)FloatValue;
					}
				}

				return (int32_t)_wtol(StringRef);
			}
			uint32_t GetUInt() const noexcept {
				return (uint32_t)GetInt();
			}
			int32_t GetInt64() const noexcept {
				if (IsInt()) {
					return Int64Value;
				}

				return (int64_t)_wtoll(StringRef);
			}
			uint64_t GetUInt64() const noexcept {
				return (uint64_t)GetInt();
			}
			bool GetBool() const noexcept {
				if (IsInt()) {
					return IntValue != 0;
				}
				else if (IsFloat()) {
					return FloatValue == 0.0f;
				}

				if (!StringRef) {
					return false;
				}

				if (!wcsicmp(L"true", StringRef) || !wcsicmp(L"1", StringRef)) {
					return true;
				}

				return false;
			}
			std::wstring GetWString() const noexcept {
				return std::wstring(StringRef);
			}

			void RebuildTypeInfo(AttributeType NewType) {
				TypeInfo = 0;

				if (NewType == AttributeType_String) {
					uint32_t Crc32 = Utils::HashUtil::CRC32_CaseInsensitive(NameCache.c_str());
					TypeInfo = (uint16_t)((Crc32 << 2) & 0xffff);
				}

				//Clear the lower 3 bits and set the type in them
				TypeInfo = (TypeInfo & (~3)) | (NewType & 3);
			}
			void SetTypeInfo(uint16_t TypeInfo) {
				this->TypeInfo = TypeInfo;
			}

			const char* GetUtf8Name() {
				if (Utf8NameCache.size() == 0) {
					Utf8NameCache.resize((wcslen(NameRef) * 4) + 4);
					if (!_WideCharToMultiByte(NameRef, (char*)Utf8NameCache.data(), Utf8NameCache.length() + 1)) {
						Message("Failed to convert name[%ws] from utf16 to utf8", NameRef);
					}
				}

				return Utf8NameCache.c_str();
			}
			const char* GetUtf8Value() {
				if (Utf8ValueCache.size() == 0 || Utf8ValueCache[0] == '\0') {
					if (IsInt()) {
						Utf8ValueCacheOriginal = std::to_string(IntValue);
						Utf8ValueCache.resize(Utf8ValueCacheOriginal.length() + AdditionalValueBufferSize);
						strcpy_s(Utf8ValueCache.data(), Utf8ValueCache.size(), Utf8ValueCacheOriginal.c_str());
					}

					if (IsFloat()) {
						Utf8ValueCacheOriginal = std::to_string(FloatValue);
						Utf8ValueCache.resize(Utf8ValueCacheOriginal.length() + AdditionalValueBufferSize);
						strcpy_s(Utf8ValueCache.data(), Utf8ValueCache.size(), Utf8ValueCacheOriginal.c_str());
					}

					if (IsString()) {
						if (StringRef[0] == L'\0') { //empty string
							Utf8ValueCacheOriginal = "";
							Utf8ValueCache.resize(AdditionalValueBufferSize);
							Utf8ValueCache[0] = '\0';
						}
						else {
							Utf8ValueCacheOriginal.resize((wcslen(StringRef) * 4) + 4);
							if (!_WideCharToMultiByte(StringRef, (char*)Utf8ValueCacheOriginal.data(), Utf8ValueCacheOriginal.length() + 1)) {
								Message("Failed to convert value [%ws] from utf16 to utf8", StringRef);
							}

							Utf8ValueCache.resize(Utf8ValueCacheOriginal.length() + AdditionalValueBufferSize);
							strcpy_s(Utf8ValueCache.data(), Utf8ValueCache.size(), Utf8ValueCacheOriginal.c_str());
						}
					}
				}

				return Utf8ValueCache.data();
			}
			void ClearCachedUtf8() {
				Utf8ValueCache[0] = '\0';
			}

			template<size_t NameSize>
			bool IsNamed(const wchar_t(&Name)[NameSize])const noexcept {
				return !wcsnicmp(Name, NameRef, NameSize);
			}

			template<size_t StrSize>
			bool IsValue(const wchar_t(&Str)[StrSize])const noexcept {
				if (!IsString()) {
					return false;
				}

				return !wcsnicmp(Str, StringRef, StrSize);
			}

#if DATACENTER_UI
			void NewUIDs() {
				uid1 = Utils::SGUID::New();
				uid2 = Utils::SGUID::New();
				uid1.B4 = 0;
				uid2.B4 = 0;
			}

			//UI ids
			Utils::SGUID	uid1;
			Utils::SGUID	uid2;
#endif

			inline const bool HasBeenEdited()
			{
				const char* NewValue = GetUtf8Value();
				const char* OrigianlValue = Utf8ValueCacheOriginal.c_str();

				return strcmp(NewValue, OrigianlValue) != 0;
			}
			inline bool SetEditValue(const char* value) {
				GetUtf8Value();

				return strcpy_s(Utf8ValueCache.data(), Utf8ValueCache.size(), value) == 0;
			}
			void ApplyValueChanges();

			//For deduplication
			uint64_t Hash = 0;

			void BuildHash() {
				Hash = ((uint64_t)Utils::HashUtil::CRC32_CaseSensitive(NameRef)) << 32;

				if (IsString()) {
					Utils::CombineHashV2(Hash, ((uint64_t)Utils::HashUtil::CRC32_CaseSensitive(StringRef)) << 32);
				}
				else {
					Utils::CombineHashV2(Hash, (((uint64_t)IntValue) << 32));
				}

				Utils::CombineHashV2(Hash, ((uint64_t)(TypeInfo & 3)) << 48);
			}
			uint64_t GetHash() {
				if (Hash == 0) {
					BuildHash();
				}

				if (Hash == 0) {
					Message("AttributeItem::Hash is %lld", Hash);
					DebugBreak();
				}

				return Hash;
			}
			};

		template<bool bForServer, bool bFor64>
		struct S1DataCenter;

		template<bool bForServer, bool bFor64>
		struct ElementItem {
			using MyType = ElementItem<bForServer, bFor64>;
			using MyAttributeType = AttributeItem<bForServer, bFor64>;
			using MyBlockIndexType = BlockIndexType<bForServer, bFor64>;
			using MyTBlockIndices = TBlockIndices<bForServer, bFor64>;
			using MyNameIndexType = NameIndexType<bForServer, bFor64>;

			MyNameIndexType						NameId = 0;
			uint16_t							Index = 0;
			uint16_t							AttributesCount = 0;
			uint16_t							ChildCount = 0;
			MyTBlockIndices						AttributesIndices = { UINT16_MAX,UINT16_MAX };
			MyTBlockIndices						ChildrenIndices = { UINT16_MAX,UINT16_MAX };
			MyTBlockIndices						FileNameStrIndices = { UINT16_MAX,UINT16_MAX };
			const wchar_t* PTR					FileNameRef = nullptr;
			bool								IsDuplicated = false;

			//For bFor64 && !bForServer
			uint32_t							Padd1 = 0;
			uint32_t							Padd2 = 0;

			//Ref
			const wchar_t* PTR					NameRef = nullptr;
			std::vector<MyAttributeType*>		Attributes;
			std::vector<MyType*>				Children;

			//Cached
			std::wstring						NameCache;
			std::string							Utf8NameCache;
			MyTBlockIndices						LocationCache = { UINT16_MAX,UINT16_MAX };
			MyTBlockIndices						ParentIndices = { UINT16_MAX,UINT16_MAX };
			MyType* Parent = nullptr;

			//For bForServer
			std::wstring						Value;
			bool								IsValueElement = false;
			bool								IsCommentElement = false;
			MyTBlockIndices						ValueIndices = { UINT16_MAX,UINT16_MAX };
			const wchar_t* PTR					ValueRef = nullptr;
			//For root elements
			std::string							FileName;
			//################################################

			//For rebuilding
			MyTBlockIndices						BuiltLocation{ 0, 0 };

			INT									RefCount = 1;
			BOOL								IsCloned = FALSE;

			ElementItem() {
#if DATACENTER_UI
				NewUIDs();
#endif
			}
			~ElementItem() {
				/*for (auto* attribute : Attributes) {
					if (attribute && attribute->IsCloned) {
						delete attribute;
					}
				}*/
				Attributes.clear();

				/*for (auto* child : Children) {
					if (child && child->IsCloned) {
						delete child;
					}
				}*/

				Children.clear();
			}

			//Move
			ElementItem(MyType&& other) {
				(*this) = std::move(other);
			}
			ElementItem& operator=(MyType&& other) {
				if (this == &other) {
					return *this;
				}

				NameId = other.NameId;
				NameRef = other.NameRef;
				Index = other.Index;
				ChildCount = other.ChildCount;
				AttributesCount = other.AttributesCount;
				ParentIndices = other.ParentIndices;

				AttributesIndices = other.AttributesIndices;
				ChildrenIndices = other.ChildrenIndices;
				IsDuplicated = other.IsDuplicated;
				IsCloned = other.IsCloned;
				RefCount = other.RefCount;
				Parent = other.Parent;

#if DATACENTER_UI
				uid1 = other.uid1;
				uid2 = other.uid2;
#endif

				if constexpr (bFor64 && !bForServer) {
					Padd1 = other.Padd1;
					Padd2 = other.Padd2;
				}

				/*Attributes = std::move(other.Attributes);
				Children = std::move(other.Children);*/
				if (other.Attributes.size()) {
					Attributes.reserve(other.Attributes.size());
				}
				if (other.Children.size()) {
					Children.reserve(other.Children.size());
				}

				Utf8NameCache = std::move(other.Utf8NameCache);
				NameCache = std::move(other.NameCache);
				LocationCache = other.LocationCache;
				FileNameStrIndices = other.FileNameStrIndices;
				ValueRef = other.ValueRef;
				FileNameRef = other.FileNameRef;

				Hash = 0;

				//Invalidate other
				/*other.Name = 0;
				other.RefCount = 0;
				other.Hash = 0;*/

				return *this;
			}

			//Copy
			ElementItem(const MyType& other) {
				(*this) = other;
			}
			ElementItem& operator=(const MyType& other) {
				if (this == &other) {
					return *this;
				}

#if DATACENTER_UI
				NewUIDs();
#endif

				NameId = other.NameId;
				NameRef = other.NameRef;
				Index = other.Index;
				ChildCount = other.ChildCount;
				AttributesCount = other.AttributesCount;
				ParentIndices = other.ParentIndices;

				AttributesIndices = other.AttributesIndices;
				ChildrenIndices = other.ChildrenIndices;
				IsDuplicated = other.IsDuplicated;
				IsCloned = other.IsCloned;
				RefCount = 1;
				Parent = other.Parent;

				if constexpr (bFor64 && !bForServer) {
					Padd1 = other.Padd1;
					Padd2 = other.Padd2;
				}

				Attributes = other.Attributes;
				Children = other.Children;

				Utf8NameCache = other.Utf8NameCache;
				NameCache = other.NameCache;
				LocationCache = other.LocationCache;
				FileNameStrIndices = other.FileNameStrIndices;
				ValueRef = other.ValueRef;
				FileNameRef = other.FileNameRef;

				Hash = 0;

				return *this;
				}

			bool Serialize(FIStream& Stream) {
				if (Stream.IsLoading()) {
					NameId = Stream.ReadT<MyNameIndexType>();

					if constexpr (bForServer) {
						IsValueElement = Stream.ReadUInt8();
						IsCommentElement = Stream.ReadUInt8();
						ValueIndices = Stream.ReadT<MyTBlockIndices>();
					}
					else {
						Index = Stream.ReadUInt16();
					}

					AttributesCount = Stream.ReadUInt16();
					ChildCount = Stream.ReadUInt16();

					AttributesIndices = Stream.ReadT<MyTBlockIndices>();

					if constexpr (bFor64 && !bForServer) {
						Padd1 = Stream.ReadUInt32();
					}

					ChildrenIndices = Stream.ReadT<MyTBlockIndices>();

					if constexpr (bFor64 && !bForServer) {
						Padd2 = Stream.ReadUInt32();
					}

					if constexpr (bForServer) {
						FileNameStrIndices = Stream.ReadT<MyTBlockIndices>();
					}
				}
				else {
					/*if (Name == 0) {
						auto path = BuildPath();
						Message("Element %ws has name 0", path.c_str());
						DebugBreak();
					}*/

					Stream.WriteT(NameId);

					if constexpr (bForServer) {
						Stream.WriteUInt8(IsValueElement);
						Stream.WriteUInt8(IsCommentElement);
						Stream.WriteT(ValueIndices);
					}
					else {
						Stream.WriteUInt16(Index);
					}

					Stream.WriteUInt16(AttributesCount);
					Stream.WriteUInt16(ChildCount);

					Stream.WriteT(AttributesIndices);

					if constexpr (bFor64 && !bForServer)
					{
						Stream.WriteUInt32(Padd1);
					}

					Stream.WriteT(ChildrenIndices);

					if constexpr (bFor64 && !bForServer)
					{
						Stream.WriteUInt32(Padd2);
					}

					if constexpr (bForServer) {
						Stream.WriteT(FileNameStrIndices);
					}
				}

				return true;
			}

			const char* GetNameUtf8() {
				if (Utf8NameCache.length() == 0) {

					Utf8NameCache.resize(wcslen(NameRef) * 2 + 2);
					if (!_WideCharToMultiByte(NameRef, (char*)Utf8NameCache.data(), Utf8NameCache.length() + 1)) {
						Message("ElementItem::Failed to convert name[%ws] from utf16 to utf8", NameRef);
					}

					Utf8NameCache.insert(0, "_");

					if (GConfig.ElementNameComposeFromAttributesList.size()) {
						for (const std::wstring& item : GConfig.ElementNameComposeFromAttributesList) {
							const AttributeItem* idAttr = GetAttribute(item.c_str());
							if (idAttr) {
								if (idAttr->IsString()) {
									std::wstring temp = idAttr->StringRef;
									Utf8NameCache.insert(0, std::string(temp.begin(), temp.end()));
								}
								else if (idAttr->IsFloat()) {
									Utf8NameCache.insert(0, std::to_string(idAttr->IntValue));
								}
								else {
									Utf8NameCache.insert(0, std::to_string(*(UINT*)&idAttr->IntValue));
								}

								Utf8NameCache.insert(0, "_");
							}
						}
					}
					else {
						const AttributeItem* idAttr = GetAttribute(L"id");
						if (idAttr) {
							Utf8NameCache.insert(0, std::to_string(*(UINT*)&idAttr->IntValue));
							Utf8NameCache.insert(0, "_");
						}

						idAttr = GetAttribute(L"templateId");
						if (idAttr) {
							Utf8NameCache.insert(0, std::to_string(*(UINT*)&idAttr->IntValue));
							Utf8NameCache.insert(0, "_");
						}

						idAttr = GetAttribute(L"readableId");
						if (idAttr) {
							std::wstring temp = idAttr->StringRef;

							Utf8NameCache.insert(0, std::string(temp.begin(), temp.end()));
							Utf8NameCache.insert(0, "_");
						}
					}

					Utf8NameCache.insert(0, std::to_string(*(UINT*)&LocationCache));
				}

				return Utf8NameCache.c_str();
			}

			std::vector<const MyType*> GetChildren(const S1DataCenter<bForServer, bFor64>* DC) const;

			bool IsIndexEnabled() const {
				return ((Index & 1) == 0) && (GetIndexValue() != 0);
			}

			INT GetIndexValue() const {
				return (Index & ELEMENT_INDEX_VALUE_MASK) >> 4;
			}

			bool IsValid() const {
				return NameId != 0;
			}

			void AddRef() {
				RefCount++;
			}

			bool DecRef() {
				return (--RefCount) == 0;
			}

			template<size_t NameSize>
			bool IsNamed(const wchar_t(&Name)[NameSize])const noexcept {
				return !wcsnicmp(Name, NameRef, NameSize);
			}

			template<size_t StrSize>
			bool IsValue(const wchar_t(&Str)[StrSize])const noexcept {
				if (!IsValueElement) {
					return false;
				}

				return !wcsnicmp(Str, ValueRef, StrSize);
			}

#if DATACENTER_UI
			void NewUIDs() {
				uid1 = Utils::SGUID::New();
				uid2 = Utils::SGUID::New();
				uid1.B4 = 0;
				uid2.B4 = 0;
			}
#endif

			std::vector<const MyAttributeType*> GetKeys(S1DataCenter<bForServer, bFor64>* DC) const;

			void SetIndex(uint16_t index, uint16_t Flags) {
				Index |= (index | Flags);
			}

			std::wstring BuildPath()const {
				ElementItem* cursor = Parent;

				std::wstring outPath;
				outPath.reserve(512);
				outPath += NameRef;

				while (cursor) {
					outPath.insert(outPath.begin(), '.');
					outPath.insert(0, cursor->NameRef);

					cursor = cursor->Parent;
				}

				return std::move(outPath);
			}
			std::wstring BuildUniquePath(const std::vector<std::wstring>& signatureAttributes)const {
				ElementItem* cursor = Parent;

				std::wstring outPath;
				outPath.reserve(512);
				outPath += NameRef;

				auto attributesSignature = BuildAttributesSignature(nullptr, &signatureAttributes);
				if (attributesSignature)
					outPath += std::to_wstring(attributesSignature);

				while (cursor) {
					outPath.insert(outPath.begin(), '.');

					attributesSignature = cursor->BuildAttributesSignature(nullptr, &signatureAttributes);
					if (attributesSignature)
						outPath.insert(0, std::to_wstring(attributesSignature));
					outPath.insert(0, cursor->NameRef);

					cursor = cursor->Parent;
				}

				return std::move(outPath);
			}
			std::wstring BuildPathByGetRefs(S1DataCenter<bForServer, bFor64>* DataCenter);

			const MyAttributeType* GetAttribute(const wchar_t* name) const;
			const MyAttributeType* GetAttributeEx(const wchar_t* names[], size_t count) const;
			const size_t BuildAttributesSignature(const std::vector<std::wstring>* excludedAttributes = nullptr, const std::vector<std::wstring>* signatureAttributes = nullptr)const;

			bool IsOrphan(S1DataCenter<bForServer, bFor64>* DataCenter);

#if DATACENTER_UI
			//UI
			bool IsSelected = false;
			Utils::SGUID	uid1;
			Utils::SGUID	uid2;
#endif
			//For rebuild
			ElementItem* Clone();

			const bool HasBeenEdited() {
				for (auto* attr : Attributes) {
					if (attr->HasBeenEdited()) {
						return true;
					}
				}
				return false;
			}
			const bool HasBeenEditedWhole() {
				for (auto* attr : Attributes) {
					if (attr->HasBeenEdited()) {
						return true;
					}
				}

				for (auto* child : Children) {
					if (child->HasBeenEditedWhole()) {
						return true;
					}
				}

				return false;
			}
			const bool HasEditedChildren() {
				for (auto* child : Children) {
					if (child->HasBeenEditedWhole()) {
						return true;
					}
				}

				return false;
			}

			bool DestroyChild(MyType* child) {
				if (!Children.size()) {
					return false;
				}

				INT j = -1;
				for (INT i = 0; i < (INT)Children.size(); i++)
				{
					if (Children[i] == child) {
						j = i;
						break;
					}
				}
				if (j != -1) {
					Children.erase(Children.begin() + j);

					if (child->IsCloned) {
						delete child;
					}

					return true;
				}

				return false;
			}

			uint64_t Hash = 0;
			uint64_t FullKey = 0;
			uint64_t ChildrenKey = 0;

			void BuildHash() {
				Hash = ((uint64_t)Utils::HashUtil::CRC32_CaseSensitive(NameRef)) << 32;

				if (Attributes.size()) {
					for (auto* attr : Attributes) {
						Utils::CombineHash(Hash, attr->GetHash());
					}
				}
			}
			uint64_t GetHash() {
				if (Hash == 0) {
					BuildHash();
				}

				if (Hash == 0) {
					const auto path = BuildPath();
					Message("ElementItem::GetHash() [%ws]\n\nHash: %lld", path.c_str(), Hash);
					DebugBreak();
				}

				return Hash;
			}
			uint64_t GetFullKey() {
				if (FullKey == 0) {
					FullKey = GetHash();

					for (auto* child : Children) {
						Utils::CombineHash(FullKey, child->GetFullKey());
					}
				}

				if (FullKey == 0) {
					const auto path = BuildPath();
					Message("ElementItem::GetFullKey() [%ws]\n\nHash: %lld", path.c_str(), FullKey);
					DebugBreak();
				}

				return FullKey;
			}
			uint64_t GetChildrenKey() {
				if (!ChildrenKey) {
					for (auto* child : Children) {
						Utils::CombineHash(ChildrenKey, child->GetFullKey());
					}
				}

				if (ChildrenKey == 0) {
					const auto path = BuildPath();
					Message("ElementItem::GetChildrenKey() [%ws]\n\nHash: %lld", path.c_str(), ChildrenKey);
					DebugBreak();
				}

				return ChildrenKey;
			}
			};

		//Metadata
		template<bool bForServer, bool bFor64>
		struct S1DataCenterMetadata {
			struct ElementIndices {
				uint16_t Key1;
				uint16_t Key2;
				uint16_t Key3;
				uint16_t Key4;

				ElementIndices() {
					Key1 = 0;
					Key2 = 0;
					Key3 = 0;
					Key4 = 0;
				}
				ElementIndices(const ElementIndices& other) {
					Key1 = other.Key1;
					Key2 = other.Key2;
					Key3 = other.Key3;
					Key4 = other.Key4;
				}
				ElementIndices& operator=(const ElementIndices& other) {
					Key1 = other.Key1;
					Key2 = other.Key2;
					Key3 = other.Key3;
					Key4 = other.Key4;

					return *this;
				}
			};

			//Type of all possible attributes (eg. ItemData.Item.id (int))
			std::unordered_map<std::wstring, uint16_t>			AttributesTypes;

			//Names of attributes used as keys (indices) (1 based)
			std::vector<std::wstring>							IndicesNames;

			//Map of indices for elements with active indices, these are indices into IndicesNames
			std::unordered_map<std::wstring, ElementIndices>	ElementsIndices;

			S1DataCenterMetadata() {
				Clear();
			}

			bool Serialize(std::string fileName) const;
			bool Deserialize(std::string fileName);

			bool BuildFromLoadedDC(S1DataCenter<bForServer, bFor64>* DC);

			void Clear() {
				IndicesNames.clear();
				IndicesNames.push_back(L""); //invalid entry

				AttributesTypes.clear();
				ElementsIndices.clear();
			}

			uint16_t HasIndexName(const wchar_t* name) const {
				for (size_t i = 0; i < IndicesNames.size(); i++)
				{
					if (!wcscmp(IndicesNames[i].c_str(), name)) {
						return (uint16_t)i;
					}
				}

				return 0; //Not found
			}

			uint16_t AddIndexName(const wchar_t* name) {
				IndicesNames.push_back(std::move(std::wstring(name, wcslen(name))));
				return (uint16_t)(IndicesNames.size() - 1);
			}
		};

		//Datacenter structure
		template<bool bForServer, bool bFor64>
		struct S1DataCenter {
			static const size_t CAttributeSerialSize = SerialSizesAndTypes<bForServer, bFor64>::GetAttributeSerialSize();
			static const size_t CElementSerialSize = SerialSizesAndTypes<bForServer, bFor64>::GetElementSerialSize();

			using MyElementType = ElementItem<bForServer, bFor64>;
			using MyAttribiteType = AttributeItem<bForServer, bFor64>;
			using MyElementRawType = ElementItemRaw<bForServer, bFor64>;

			using TBlockIndices = typename SerialSizesAndTypes<bForServer, bFor64>::TBlockIndices;
			using BlockIndexType = typename SerialSizesAndTypes<bForServer, bFor64>::BlockIndexType;
			using NameIndexType = typename SerialSizesAndTypes<bForServer, bFor64>::NameIndexType;

			using ElementsBlocks = DCArray<DCBlockArray<MyElementType, CElementSerialSize>>;
			using AttributesBlocks = DCArray<DCBlockArray<MyAttribiteType, CAttributeSerialSize>>;

			using MyDCMap = DCMap<bForServer, bFor64>;
			using MyS1DataCenterMetadata = S1DataCenterMetadata<bForServer, bFor64>;
			using MyStringsBucket = StringsBucket<bForServer, bFor64>;

			//File structure
			int32_t														FormatVersion = 0;
			uint64_t													Unk1_8 = 0;
			int32_t														Version = 0;

			UnkTree														Unk;
			DCArray<DCIndex>											Indices;
			AttributesBlocks											Attributes;
			ElementsBlocks												Elements;
			MyDCMap														ValuesMap;
			MyDCMap														NamesMap;

			uint32_t													EndCount = 0;

			//Metadata
			std::unique_ptr<MyS1DataCenterMetadata>						Metadata;

			//Caching for editing
			std::vector<MyElementType*>									AllItemElements;
			std::vector<MyElementType*>									AllSkillElements;

			//Is Datacenter loaded
			bool														bIsLoaded = false;

			//Datacenter Indices cache
			std::unordered_map<std::wstring, CachedElementIndex>		CachedElementIndices;
			std::unordered_map<std::wstring, int32_t>					CachedNamesIndices;

			S1DataCenter() {
				ValuesMap.Buckets.Count = 1024;
				NamesMap.Buckets.Count = 512;
			}

#pragma region QUERY API
		public:
			MyElementType* GetRootElement() {
				return &Elements.Data[0].Data[0];
			}
			const MyElementType* GetRootElementC() const {
				return &Elements.Data[0].Data[0];
			}

			bool IsLoaded() const {
				return bIsLoaded;
			}
			bool IsNamePartOfIndex(const wchar_t* str)const {
				for (size_t i = 0; i < Indices.Data.size(); i++) {
					if (Indices.Data[i].Key == 0) {
						continue;
					}

					if (Indices.Data[i].Key1) {
						if (!wcscmp(str, NamesMap.AllStrings.Data[Indices.Data[i].Key1 - 1].CachedString)) {
							return true;
						}
					}

					if (Indices.Data[i].Key2) {
						if (!wcscmp(str, NamesMap.AllStrings.Data[Indices.Data[i].Key2 - 1].CachedString)) {
							return true;
						}
					}

					if (Indices.Data[i].Key3) {
						if (!wcscmp(str, NamesMap.AllStrings.Data[Indices.Data[i].Key3 - 1].CachedString)) {
							return true;
						}
					}

					if (Indices.Data[i].Key4) {
						if (!wcscmp(str, NamesMap.AllStrings.Data[Indices.Data[i].Key4 - 1].CachedString)) {
							return true;
						}
					}
				}

				return false;
			}

			void BuildDCExportName(std::wstring& str) const {
				str += L"DC";
				str += L"_";
				str += std::to_wstring(Version);
			}
			void BuildDCExportName(std::string& str) const {
				str += "DC";
				str += "_";
				str += std::to_string(Version);
			}

			MyElementType* GetElement(TBlockIndices indices) {
				if (indices.first == UINT16_MAX &&
					indices.second == UINT16_MAX) {
					return nullptr;
				}

				return &Elements.Data[indices.first].Data[indices.second];
			}
			MyElementType* GetElement(BlockIndexType blockIndex, BlockIndexType itemIndex) {
				return &Elements.Data[blockIndex].Data[itemIndex];
			}
			MyAttribiteType* GetAttribute(TBlockIndices indices) {
				return &Attributes.Data[indices.first].Data[indices.second];
			}
			MyAttribiteType* GetAttribute(BlockIndexType blockIndex, BlockIndexType itemIndex) {
				return &Attributes.Data[blockIndex].Data[itemIndex];
			}

			std::vector<const MyElementType*> GetAllByNameStartsWith(const wchar_t* String)noexcept {
				std::vector<const MyElementType*> Result;
				Result.reserve(1024); //reserve some

				const auto Root = GetRootElement();

				for (const auto Item : Root->Children) {
					if (Utils::CString_StartsWith(Item->NameRef, String)) {
						Result.push_back(Item);
					}
				}

				return std::move(Result);
			}
			std::vector<const MyElementType*> GetAllByName(const wchar_t* Name)noexcept {
				std::vector<const MyElementType*> Result;
				Result.reserve(1024); //reserve some

				const auto Root = GetRootElement();

				for (const auto Item : Root->Children) {
					if (!wcsicmp(Item->NameRef, Name)) {
						Result.push_back(Item);
					}
				}

				return std::move(Result);
			}

#pragma endregion

#pragma region PREPARING
		public:
			bool PrepareForBuild() {
				ValuesMap.Buckets.Clear();
				//NamesMap.Buckets.Clear();

				for (size_t i = 0; i < ValuesMap.Buckets.Count; i++) {
					ValuesMap.Buckets.Buckets.push_back(MyStringsBucket());
				}

				/*	for (size_t i = 0; i < NamesMap.Buckets.Count; i++) {
						NamesMap.Buckets.Buckets.push_back(MyStringsBucket());
					}*/

				return true;
			}

			bool Server_PrepareForBuild() {
				ValuesMap.Buckets.Clear();
				//NamesMap.Buckets.Clear();

				for (size_t i = 0; i < ValuesMap.Buckets.Count; i++) {
					ValuesMap.Buckets.Buckets.push_back(MyStringsBucket());
				}

				for (size_t i = 0; i < NamesMap.Buckets.Count; i++) {
					NamesMap.Buckets.Buckets.push_back(MyStringsBucket());
				}

				return true;
			}

			void AssociateIndices(MyElementType* element);
			void BuildIndicesCache();
			bool BuildIndices();
			bool BuildNamesIndices();

			bool Serialize(FIStream& Stream, bool bDoPostLoad = true);
			bool PostLoad();

			bool PostLoadElements();
			bool PostLoadAttributes();

			void OnAfterLoaded();

			void SetParents(MyElementType* element, MyElementType* parent) {

				if (!element->IsValid()) {
					return;
				}

				element->Parent = parent;

				for (size_t i = 0; i < element->ChildCount; i++)
				{
					SetParents(GetElement({ element->ChildrenIndices.first , element->ChildrenIndices.second + i }), element);
				}
			}

			void Clear() {
				Unk.Clear();
				Indices.Clear();
				Attributes.Clear();
				Elements.Clear();
				ValuesMap.Clear();
				NamesMap.Clear();
				CachedElementIndices.clear();

				bIsLoaded = false;

				Metadata.reset();

				//ImGui
				EditedElements.clear();
			}

		private:
			bool PostLoadElementsRecursive(MyElementType* Element, MyElementType* Parent);
#pragma endregion

#pragma region REBUILDING CLIENT
		public:
			bool InsertElementTree(MyElementRawType* Root) {
				TBlockIndices LocalRootIndices;
				if (!AllocateElement(Root, LocalRootIndices)) {
					return false;
				}

				GetElement(LocalRootIndices)->ChildCount = (uint16_t)(Root->Children.size());

				if (Root->Children.size()) {
					TBlockIndices childrenIndices;

					std::vector<MyElementRawType*> UniqueRootChildren;
					UniqueRootChildren.reserve(Root->Children.size());

					//Reset all
					for (size_t i = 0; i < Root->Children.size(); i++)
					{
						Root->Children[i]->Used = false;
					}

					for (size_t i = 0; i < Root->Children.size(); i++)
					{
						if (!Root->Children[i]->Used) {
							UniqueRootChildren.push_back(Root->Children[i]);
							Root->Children[i]->Used = true;
						}
					}

					Root->Children = std::move(UniqueRootChildren);

					//allocate empty elements for all root children
					if (!AllocateElementsSection((int32_t)Root->Children.size(), childrenIndices)) {
						return false;
					}

					//set root node children indices
					GetElement(LocalRootIndices)->ChildrenIndices = childrenIndices;

					//Parse all sub tree
					for (size_t i = 0; i < Root->Children.size(); i++) {
						//check if already exists
						if (Root->Children[i]->HasValidDCIndices()) {

							//validate that the indices are part of the root children
							if (Root->Children[i]->CachedDCElementIndices.first != childrenIndices.first ||
								Root->Children[i]->CachedDCElementIndices.second < childrenIndices.second ||
								Root->Children[i]->CachedDCElementIndices.second >(childrenIndices.second + (WORD)Root->Children.size())) {
								Message("Found existing node but not inside root children indices\n\n\tNode[%ws]", Root->Children[i]->Name.c_str());
								return false;
							}

							continue;
						}

						//cache element indices for duplication check and skip
						Root->Children[i]->CachedDCElementIndices = childrenIndices;

						NameIndexType NameId = NamesMap.FindStringEx(Root->Children[i]->Name.c_str());
						if (NameId == 0) {
							Message("Could not find raw element name[%ws] in the NamesMap", Root->Children[i]->Name.c_str());
							return false;
						}
						/*if (!NamesMap.InsertString(Root->Children[i]->Name.c_str(), Root->Children[i]->Name.size(), NameId)) {
							return false;
						}*/

						NamesMap.AllStrings.Data[NameId - 1].RefElements.push_back({ childrenIndices.first, childrenIndices.second + i });
						GetElement({ childrenIndices.first, childrenIndices.second + i })->NameId = NameId;

						if (!InsertElementTreeRecursive(Root->Children[i], TBlockIndices<bForServer, bFor64>(childrenIndices.first, childrenIndices.second + i), LocalRootIndices)) {
							return false;
						}
					}
				}

				/*if (!ValuesMap.BuildHashTables()) {
					return false;
				}*/

				/*if (!NamesMap.BuildHashTables()) {
					return false;
				}*/

				//sort all bucket values by Hash for values map
				/*for (size_t i = 0; i < ValuesMap.Buckets.Buckets.size(); i++)
				{
					std::sort(ValuesMap.Buckets.Buckets[i].Elements.begin(), ValuesMap.Buckets.Buckets[i].Elements.end(),
						[](const StringsBucket::BucketElement& a, const StringsBucket::BucketElement& b) -> bool
						{
							return a.Hash > b.Hash;
						});
				}*/

				return true;
			}
		private:
			bool InsertElementTreeRecursive(MyElementRawType* RawElement, TBlockIndices ElementIndices, TBlockIndices parentIndices) {
				if (RawElement->Attributes.size()) {
					if (!AllocateAttributes(RawElement->Attributes, GetElement(ElementIndices)->AttributesIndices)) {
						return false;
					}
				}

				GetElement(ElementIndices)->AttributesCount = (uint16_t)(RawElement->Attributes.size());
				GetElement(ElementIndices)->ChildCount = (uint16_t)(RawElement->Children.size());
				GetElement(ElementIndices)->ParentIndices = parentIndices;

				if (RawElement->Children.size()) {
					TBlockIndices childrenIndices;

					if (!AllocateElements(RawElement->Children, childrenIndices)) {
						return false;
					}

					GetElement(ElementIndices)->ChildrenIndices = childrenIndices;

					for (size_t i = 0; i < RawElement->Children.size(); i++) {
						if (!InsertElementTreeRecursive(RawElement->Children[i], TBlockIndices<bForServer, bFor64>(childrenIndices.first, childrenIndices.second + i), ElementIndices))
						{
							return false;
						}
					}
				}

				return true;
			}
#pragma endregion

#pragma region REBUILDING SERVER
		public:
			bool Server_InsertElementTree(MyElementRawType* Root) {
				TBlockIndices LocalRootIndices;
				if (!Server_AllocateElement(Root, LocalRootIndices)) {
					return false;
				}

				GetElement(LocalRootIndices)->ChildCount = (uint16_t)(Root->Children.size());

				if (Root->Children.size()) {
					TBlockIndices childrenIndices;

					std::vector<MyElementRawType*> UniqueRootChildren;
					UniqueRootChildren.reserve(Root->Children.size());

					//Reset all
					for (size_t i = 0; i < Root->Children.size(); i++)
					{
						Root->Children[i]->Used = false;
					}

					for (size_t i = 0; i < Root->Children.size(); i++)
					{
						if (!Root->Children[i]->Used) {
							UniqueRootChildren.push_back(Root->Children[i]);
							Root->Children[i]->Used = true;
						}
					}

					Root->Children = std::move(UniqueRootChildren);

					//allocate empty elements for all root children
					if (!Server_AllocateElementsSection((int32_t)Root->Children.size(), childrenIndices)) {
						return false;
					}

					//set root node children indices
					GetElement(LocalRootIndices)->ChildrenIndices = childrenIndices;

					//Parse all sub tree
					for (size_t i = 0; i < Root->Children.size(); i++) {

						//check if already exists
						if (Root->Children[i]->HasValidDCIndices()) {

							//validate that the indices are part of the root children
							if (Root->Children[i]->CachedDCElementIndices.first != childrenIndices.first ||
								Root->Children[i]->CachedDCElementIndices.second < childrenIndices.second ||
								Root->Children[i]->CachedDCElementIndices.second >(childrenIndices.second + Root->Children.size())) {
								Message("Found existing node but not inside root children indices\n\n\tNode[%ws]", Root->Children[i]->Name.c_str());
								return false;
							}

							continue;
						}

						//cache element indices for duplication check and skip
						Root->Children[i]->CachedDCElementIndices = childrenIndices;

						NameIndexType NameId;
						if (!NamesMap.InsertString(Root->Children[i]->Name.c_str(), Root->Children[i]->Name.size(), NameId)) {
							return false;
						}

						NamesMap.AllStrings.Data[NameId - 1].RefElements.push_back({ childrenIndices.first, childrenIndices.second + i });
						GetElement({ childrenIndices.first, childrenIndices.second + i })->NameId = NameId;

						if (!Server_InsertElementTreeRecursive(Root->Children[i], TBlockIndices<bForServer, bFor64>(childrenIndices.first, childrenIndices.second + i), LocalRootIndices)) {
							return false;
						}
					}
				}

				return true;
			}
		private:
			bool Server_InsertElementTreeRecursive(MyElementRawType* RawElement, TBlockIndices ElementIndices, TBlockIndices parentIndices) {
				if (RawElement->Attributes.size()) {
					if (!Server_AllocateAttributes(RawElement->Attributes, GetElement(ElementIndices)->AttributesIndices)) {
						return false;
					}
				}

				GetElement(ElementIndices)->AttributesCount = (WORD)(RawElement->Attributes.size());
				GetElement(ElementIndices)->ChildCount = (WORD)(RawElement->Children.size());
				GetElement(ElementIndices)->ParentIndices = parentIndices;

				if constexpr (bForServer) {
					GetElement(ElementIndices)->IsValueElement = RawElement->IsValueElement;

					if (RawElement->IsValueElement) {
						TBlockIndices valueIndices = { UINT16_MAX,UINT16_MAX };
						ValuesMap.InsertString(RawElement->Value.data(), RawElement->Value.size(), valueIndices);
						GetElement(ElementIndices)->ValueIndices = valueIndices;
					}
				}

				if (RawElement->Children.size()) {
					TBlockIndices childrenIndices;

					if (!Server_AllocateElements(RawElement->Children, childrenIndices)) {
						return false;
					}

					GetElement(ElementIndices)->ChildrenIndices = childrenIndices;

					for (size_t i = 0; i < RawElement->Children.size(); i++) {
						if (!Server_InsertElementTreeRecursive(RawElement->Children[i], std::pair<WORD, WORD>(childrenIndices.first, childrenIndices.second + i), ElementIndices))
						{
							return false;
						}
					}
				}

				return true;
			}
#pragma endregion

#pragma region BUILD
		public:
			bool AllocateElement(MyElementRawType* RawElement, TBlockIndices& OutIndices) {
				auto& Block = GetElementsContainerForNewElements(1, OutIndices.first);

				//start of this elements block
				OutIndices.second = (BlockIndexType)(Block.Data.size());

				Block.Data.push_back(MyElementType());
				Block.UsedCount++;

				//Prepare Element
				MyElementType* NewElement = &Block.Data.back();

				NewElement->Index = 0;
				NewElement->LocationCache.first = OutIndices.first;
				NewElement->LocationCache.second = (BlockIndexType)(Block.Data.size() - 1);
				NewElement->NameCache = RawElement->Name;

				/*BlockIndexType NameId;
				if (!NamesMap.InsertString(NewElement->NameCache.data(), NewElement->NameCache.size(), NameId)) {
					Message("AllocateElement::Failed to insert string [%ws]", NewElement->NameCache.size());
					return false;
				}*/

				NameIndexType NameId = NamesMap.FindStringEx(NewElement->NameCache.c_str());
				if (NameId == 0) {
					Message("AllocateElement::Could not find element name[%ws] in the NamesMap", NewElement->NameCache.c_str());
					return false;
				}

				NewElement->NameId = NameId;

				NamesMap.AllStrings.Data[NameId - 1].RefElements.push_back(NewElement->LocationCache);

				return true;
			}
			bool AllocateElements(std::vector<MyElementRawType*>& RawElements, TBlockIndices& OutIndices) {
				auto& Block = GetElementsContainerForNewElements((int32_t)RawElements.size(), OutIndices.first);

				//start of this elements block
				OutIndices.second = (BlockIndexType)(Block.Data.size());

				for (auto* RawElement : RawElements) {
					Block.Data.push_back(MyElementType());
					Block.UsedCount++;

					//Prepare Element
					MyElementType* NewElement = &Block.Data.back();

					NewElement->LocationCache.first = OutIndices.first;
					NewElement->LocationCache.second = (BlockIndexType)(Block.Data.size() - 1);
					NewElement->NameCache = RawElement->Name;

					/*WORD NameId;
					if (!NamesMap.InsertString(RawElement->Name.c_str(), RawElement->Name.size(), NameId)) {
						Message("Failed to insert string [%ws]", RawElement->Name.c_str());
						return false;
					}*/

					NameIndexType NameId = NamesMap.FindStringEx(RawElement->Name.c_str());
					if (NameId == 0) {
						Message("AllocateElements::Could not find raw element name[%ws] in the NamesMap", RawElement->Name.c_str());
						return false;
					}

					NewElement->NameId = NameId;
					NewElement->NameRef = NamesMap.AllStrings.Data[NameId - 1].CachedString;

					//cache ref to element, needed for name relocating after sorting
					NamesMap.AllStrings.Data[NameId - 1].RefElements.push_back(NewElement->LocationCache);
				}

				return true;
			}
			bool AllocateElementsSection(int32_t Count, TBlockIndices& OutStartIndices) {
				auto& Block = GetElementsContainerForNewElements(Count, OutStartIndices.first);

				OutStartIndices.second = (BlockIndexType)(Block.Data.size());

				for (int32_t i = 0; i < Count; i++)
				{
					Block.Data.push_back(MyElementType());
					Block.UsedCount++;

					Block.Data.back().LocationCache.first = OutStartIndices.first;
					Block.Data.back().LocationCache.second = (BlockIndexType)(Block.Data.size() - 1);
				}

				return true;
			}

			bool AllocateAttributes(std::vector<AttributeItemRaw>& RawAttributes, TBlockIndices& OutIndices) {
				auto& Block = GetAttributesContainerForNewAttributes((int32_t)RawAttributes.size(), OutIndices.first);

				//start of this attributes block
				OutIndices.second = (BlockIndexType)(Block.Data.size());

				for (auto& RawAttribute : RawAttributes) {
					Block.Data.push_back(MyAttribiteType());
					Block.UsedCount++;

					//Prepare Attribute
					MyAttribiteType* NewAttribute = &Block.Data.back();

					NewAttribute->Index = 0;
					NewAttribute->LocationCache.first = OutIndices.first;
					NewAttribute->LocationCache.second = (BlockIndexType)(Block.Data.size() - 1);
					NewAttribute->NameCache = RawAttribute.Name;
					NewAttribute->StringyfiedValue = RawAttribute.Value;

					NameIndexType StringId;

					if (RawAttribute.Type == AttributeType_String) {
						if (!ValuesMap.InsertString(NewAttribute->StringyfiedValue.data(), NewAttribute->StringyfiedValue.size(), NewAttribute->Indices)) {
							return false;
						}
					}
					else if (RawAttribute.Type == AttributeType_Int) {
						NewAttribute->IntValue = _wtol(NewAttribute->StringyfiedValue.c_str());
					}
					else if (RawAttribute.Type == AttributeType_Float) {
						NewAttribute->FloatValue = _wtof(NewAttribute->StringyfiedValue.c_str());
					}

					NewAttribute->RebuildTypeInfo(RawAttribute.Type);

					StringId = NamesMap.FindStringEx(NewAttribute->NameCache.c_str());
					if (StringId == 0) {
						Message("Could not find attribute name[%ws] in the NamesMap", NewAttribute->NameCache.c_str());
						return false;
					}

					/*if (!NamesMap.InsertString(NewAttribute->NameCache.data(), NewAttribute->NameCache.size(), StringId)) {
						return false;
					}*/

					NewAttribute->NameId = StringId;

					NamesMap.AllStrings.Data[StringId - 1].RefAttributes.push_back(NewAttribute->LocationCache);
				}

				return true;
			}
			bool AllocateAttributesSection(int32_t Count, TBlockIndices& OutStartIndices) {
				auto& Block = GetAttributesContainerForNewAttributes(Count, OutStartIndices.first);

				OutStartIndices.second = (BlockIndexType)(Block.Data.size());

				for (int32_t i = 0; i < Count; i++)
				{
					Block.Data.push_back(MyAttribiteType());
					Block.UsedCount++;

					Block.Data.back().LocationCache.first = OutStartIndices.first;
					Block.Data.back().LocationCache.second = (BlockIndexType)(Block.Data.size() - 1);
				}

				return true;
			}

			DCBlockArray<MyElementType, CElementSerialSize>& GetElementsContainerForNewElements(int32_t Count, BlockIndexType& Out_Index) {
				//Try to find block that will contain all elements
				for (size_t i = 0; i < Elements.Data.size(); i++)
				{
					if (Elements.Data[i].CanWrite(Count)) {

						Out_Index = (BlockIndexType)(i);

						return Elements.Data[i];
					}
				}

				//Create new block array and reserve max
				Elements.Data.push_back(DCBlockArray<MyElementType, CElementSerialSize>());
				Elements.Data.back().Data.reserve(CElementsBlockSize);
				Elements.Data.back().MaxCount = CElementsBlockSize;

				Elements.Count++;

				Out_Index = (BlockIndexType)(Elements.Data.size() - 1);

				return Elements.Data.back();
			}
			DCBlockArray<MyAttribiteType, CAttributeSerialSize>& GetAttributesContainerForNewAttributes(int32_t Count, BlockIndexType& Out_Index) {
				//Try to find block that will contain all attributes
				for (size_t i = 0; i < Attributes.Data.size(); i++)
				{
					if (Attributes.Data[i].CanWrite(Count)) {

						Out_Index = (BlockIndexType)(i);

						return Attributes.Data[i];
					}
				}

				Out_Index = (BlockIndexType)(Attributes.Data.size());

				//Create new block array and reserve max
				Attributes.Data.push_back(DCBlockArray<MyAttribiteType, CAttributeSerialSize>());

				Attributes.Data.back().Data.reserve(CAttributesBlockSize);
				Attributes.Data.back().MaxCount = CAttributesBlockSize;

				Attributes.Count++;

				return Attributes.Data.back();

			}
#pragma endregion

#pragma region BUILD_SERVER
			bool Server_AllocateElements(std::vector<MyElementRawType*>& RawElements, TBlockIndices& OutIndices) {
				auto& Block = GetElementsContainerForNewElements((int32_t)RawElements.size(), OutIndices.first);

				//start of this elements block
				OutIndices.second = (BlockIndexType)(Block.Data.size());

				for (auto* RawElement : RawElements) {

					Block.Data.push_back(MyElementType());
					Block.UsedCount++;

					//Prepare Element
					MyElementType* NewElement = &Block.Data.back();

					NewElement->LocationCache.first = OutIndices.first;
					NewElement->LocationCache.second = (BlockIndexType)(Block.Data.size() - 1);
					NewElement->NameCache = RawElement->Name;

					NameIndexType NameId;
					if (!NamesMap.InsertString(RawElement->Name.c_str(), RawElement->Name.size(), NameId)) {
						Message("Failed to insert string [%ws]", RawElement->Name.c_str());
						return false;
					}

					NewElement->NameId = NameId;
					NewElement->NameRef = NamesMap.AllStrings.Data[NameId - 1].CachedString;

					//cache ref to element, needed for name relocating after sorting
					NamesMap.AllStrings.Data[NameId - 1].RefElements.push_back(NewElement->LocationCache);
				}

				return true;
			}
			bool Server_AllocateElement(MyElementRawType* RawElement, TBlockIndices& OutIndices) {
				auto& Block = GetElementsContainerForNewElements(1, OutIndices.first);

				//start of this elements block
				OutIndices.second = (BlockIndexType)(Block.Data.size());

				Block.Data.push_back(MyElementType());
				Block.UsedCount++;

				//Prepare Element
				MyElementType* NewElement = &Block.Data.back();

				NewElement->Index = 0;
				NewElement->LocationCache.first = OutIndices.first;
				NewElement->LocationCache.second = (BlockIndexType)(Block.Data.size() - 1);
				NewElement->NameCache = RawElement->Name;

				NameIndexType NameId;
				if (!NamesMap.InsertString(NewElement->NameCache.data(), NewElement->NameCache.size(), NameId)) {
					Message("AllocateElement::Failed to insert string [%ws]", NewElement->NameCache.size());
					return false;
				}

				NewElement->NameId = NameId;

				NamesMap.AllStrings.Data[NameId - 1].RefElements.push_back(NewElement->LocationCache);

				return true;
			}
			bool Server_AllocateElementsSection(int32_t Count, TBlockIndices& OutStartIndices) {
				auto& Block = GetElementsContainerForNewElements(Count, OutStartIndices.first);

				OutStartIndices.second = (BlockIndexType)(Block.Data.size());

				for (int32_t i = 0; i < Count; i++)
				{
					Block.Data.push_back(MyElementType());
					Block.UsedCount++;

					Block.Data.back().LocationCache.first = OutStartIndices.first;
					Block.Data.back().LocationCache.second = (BlockIndexType)(Block.Data.size() - 1);
				}

				return true;
			}

			bool Server_AllocateAttributes(std::vector<AttributeItemRaw>& RawAttributes, TBlockIndices& OutIndices) {
				auto& Block = GetAttributesContainerForNewAttributes((int32_t)RawAttributes.size(), OutIndices.first);

				//start of this attributes block
				OutIndices.second = (BlockIndexType)(Block.Data.size());

				for (auto& RawAttribute : RawAttributes) {
					Block.Data.push_back(MyAttribiteType());
					Block.UsedCount++;

					//Prepare Attribute
					MyAttribiteType* NewAttribute = &Block.Data.back();

					NewAttribute->Index = 0;
					NewAttribute->LocationCache.first = OutIndices.first;
					NewAttribute->LocationCache.second = (BlockIndexType)(Block.Data.size() - 1);
					NewAttribute->NameCache = RawAttribute.Name;
					NewAttribute->StringyfiedValue = RawAttribute.Value;

					NameIndexType StringId;

					if (RawAttribute.Type == AttributeType_String) {
						if (!ValuesMap.InsertString(NewAttribute->StringyfiedValue.data(), NewAttribute->StringyfiedValue.size(), NewAttribute->Indices)) {
							return false;
						}
					}
					else if (RawAttribute.Type == AttributeType_Int) {
						NewAttribute->IntValue = _wtol(NewAttribute->StringyfiedValue.c_str());
					}
					else if (RawAttribute.Type == AttributeType_Float) {
						NewAttribute->FloatValue = _wtof(NewAttribute->StringyfiedValue.c_str());
					}

					NewAttribute->RebuildTypeInfo(RawAttribute.Type);

					if (!NamesMap.InsertString(NewAttribute->NameCache.data(), NewAttribute->NameCache.size(), StringId)) {
						return false;
					}

					NewAttribute->NameId = StringId;

					NamesMap.AllStrings.Data[StringId - 1].RefAttributes.push_back(NewAttribute->LocationCache);
				}

				return true;
			}
#pragma endregion

#pragma region EDITING
		public:
			std::vector<MyElementType*>					EditedElements;
			std::vector<std::unique_ptr<MyElementType>>	AddedElements;

			//Rebuild
			bool RebuildValueStringsBlocksAndMap();
			bool Rebuild();
			bool RebuildElementsAndAttributesList();

			//Value changes
			inline void GenerateChangesList() {
				auto root = GetRootElement();
				EditedElements.clear();

				for (auto* item : root->Children) {
					if (HasBeenEdited_Tree(item)) {
						EditedElements.push_back(item);
					}
				}
			}
			inline void ApplyValueChanges() {
				Message("Apply changes\nClick Ok to start!");
				Message("Phase 1: Apply Numeric value changes\nClick Ok to Start");
				for (auto* item : EditedElements) {
					ApplyValueChangesRecursive(item);
				}

				Message("Phase 2: Rebuilding the strings map\nClick Ok to Start");
				if (!RebuildValueStringsBlocksAndMap()) {
					Message("CRTICAL! Failed to rebuild the strings map!");
				}
				else {
					Message("SUCCESS! All changes have been applied, you can save the Datacenter now!");
				}
			}

		private:
			bool RebuildElementsListRecursive(ElementItem<bForServer, bFor64>* element, TBlockIndices location, bool isRoot);
			void ApplyValueChangesRecursive(MyElementType* Item);

			inline bool HasBeenEdited_Tree(MyElementType* item) {
				for (auto* attr : item->Attributes) {
					if (attr->HasBeenEdited()) {
						return true;
					}
				}

				for (auto* child : item->Children) {
					if (HasBeenEdited_Tree(child)) {
						return true;
					}
				}

				return false;
			}

#pragma endregion

#pragma region REBUILD EX
		public:
			bool InsertElementsTreeEx(MyElementRawType* root) {
				Clear();

				if constexpr (bForServer) {
					Server_PrepareForBuild();
				}
				else {
					PrepareForBuild();
				}

				TBlockIndices RootIndices;
				AllocateElementsSection(1, RootIndices);

				return InsertElementsTreeExRecursive(root, RootIndices);
			}
		private:
			bool InsertElementsTreeExRecursive(MyElementRawType* rawElement, TBlockIndices elementIndex) {
				//insert name

				MyElementType* element = GetElement(elementIndex);

				if (rawElement->Type == RawElementType::Comment) {
					if constexpr (!bForServer) {
						Message("InsertElementsTreeEx:: Client data should not have comment elements, skipp comment elements when parsing the source files(xml)!");
						return false;
					}
					else {
						element->AttributesCount = 0;
						element->ChildCount = 0;

						element->IsValueElement = false;
						element->IsCommentElement = true;
						element->Value = rawElement->Value;

						if (!ValuesMap.InsertString(rawElement->Value.c_str(), rawElement->Value.length(), element->ValueIndices)) {
							Message("InsertElementsTreeEx:: Failed to insert element comment string [%ws]", rawElement->Value.c_str());
							return false;
						}

						return true;
					}
				}
				else {
					element->AttributesCount = (uint16_t)rawElement->Attributes.size();
					element->ChildCount = (uint16_t)rawElement->Children.size();

					if (!NamesMap.InsertString(rawElement->Name.c_str(), rawElement->Name.length(), element->NameId)) {
						Message("InsertElementsTreeEx:: Failed to insert element name string [%ws]", rawElement->Name.c_str());
						return false;
					}

					//Dont cache name ref here as we'll prepare the elements at the end

					if (rawElement->Attributes.size()) {
						//Allocate attributes section
						TBlockIndices AttributeIndices;
						if (!AllocateAttributesSection(rawElement->Attributes.size(), AttributeIndices)) {
							Message("InsertElementsTreeEx:: Failed to allocate attributes section(%lld)", rawElement->Attributes.size());
							return false;
						}

						for (size_t i = 0; i < rawElement->Attributes.size(); i++)
						{
							MyAttribiteType* attr = GetAttribute({ AttributeIndices.first, AttributeIndices.second + i });
							auto& rawAttr = rawElement->Attributes[i];

							//1.Insert attribute name
							if (!NamesMap.InsertString(rawAttr.Name.c_str(), rawAttr.Name.length(), attr->NameId)) {
								Message("InsertElementsTreeEx:: Failed to insert attribute name string [%ws]", rawAttr.Name.c_str());
								return false;
							}

							//2.Set attribute value
							if constexpr (bForServer) {
								attr->SetTypeInfo(rawAttr.Type);
								if (rawAttr.ServerType == ServerAttributeType::Int) {
									attr->IntValue = _wtol(rawAttr.Value.c_str());
								}
								else if (rawAttr.ServerType == ServerAttributeType::Int64) {
									attr->Int64Value = _wtoll(rawAttr.Value.c_str());
								}
								else if (rawAttr.ServerType == ServerAttributeType::Float) {
									attr->FloatValue = _wtof(rawAttr.Value.c_str());
								}
								else {
									if (!ValuesMap.InsertString(rawAttr.Value.c_str(), rawAttr.Value.size(), attr->Indices)) {
										Message("InsertElementsTreeEx:: Failed to insert attribute value string[%ws]", rawElement->Name.c_str());
										return false;
									}
								}
							}
							else {
								//@TODO for client
								return false;
							}
						}

						element->AttributesIndices = AttributeIndices;
					}

					if constexpr (bForServer) {
						//Insert cached file name if any
						if (rawElement->CachedFileName.size()) {
							if (!NamesMap.InsertString(rawElement->CachedFileName.c_str(), rawElement->CachedFileName.length(), element->FileNameStrIndices)) {
								Message("InsertElementsTreeEx:: Failed to insert element file name string [%ws]", rawElement->CachedFileName.c_str());
								return false;
							}
						}

						//Insert element value strng is any
						element->IsValueElement = rawElement->IsValueElement;
						if (rawElement->IsValueElement) {
							element->Value = rawElement->Value;

							if (!ValuesMap.InsertString(rawElement->Value.c_str(), rawElement->Value.length(), element->ValueIndices)) {
								Message("InsertElementsTreeEx:: Failed to insert element value string [%ws]", rawElement->Value.c_str());
								return false;
							}
						}
					}

					if (!rawElement->Children.size()) {
						return true;
					}

					TBlockIndices ChildrenIndices;
					if (!AllocateElementsSection(rawElement->Children.size(), ChildrenIndices)) {
						Message("InsertElementsTreeEx:: Failed to allocate child elements section");
						return false;
					}
					element->ChildrenIndices = ChildrenIndices;

					for (size_t i = 0; i < rawElement->Children.size(); i++)
					{
						if (!InsertElementsTreeExRecursive(rawElement->Children[i], { ChildrenIndices.first, ChildrenIndices.second + i })) {
							return false;
						}
					}

					return true;
				}
			}
#pragma endregion

		private:
			static std::unordered_map<uint64_t, TBlockIndices> GExistingElements;
		};

		//Clone utils
		template<bool bForServer, bool bFor64>
		ElementItem<bForServer, bFor64>* CloneElementItem(ElementItem<bForServer, bFor64>* Item) {
			std::unique_ptr<ElementItem> NewItem(new ElementItem(*Item));

			NewItem->IsCloned = TRUE;
			NewItem->NewUIDs();
			NewItem->Utf8NameCache.clear();

			//Clone attributes
			NewItem->Attributes.clear();
			for (auto* attr : Item->Attributes) {
				auto newAttr = CloneAttributeItem(attr);
				if (newAttr) {
					NewItem->Attributes.push_back(newAttr);
				}
			}

			//This cloned element is not part of the DC structure 
			NewItem->AttributesCount = 0;
			NewItem->AttributesIndices = { 0 , 0 };

			NewItem->ChildCount = 0;
			NewItem->ChildrenIndices = { 0 , 0 };

			//Clone children recursively
			for (auto* child : Item->Children) {
				auto* NewChild = CloneElementItem(child);
				if (!NewChild) {
					return nullptr;
				}

				NewItem->Children.push_back(NewChild);
			}

			return NewItem.release();
		}

		template<bool bForServer, bool bFor64>
		AttributeItem<bForServer, bFor64>* CloneAttributeItem(AttributeItem<bForServer, bFor64>* Item) {
			auto* newAttr = new AttributeItem(*Item);
			if (newAttr) {
				newAttr->IsCloned = TRUE;
				newAttr->LocationCache = { 0,0 };
				newAttr->ClearCachedUtf8();
			}

			return newAttr;
		}
		}

	namespace Structure { //Template implementations

#pragma region AttributeItem
		template<bool bForServer, bool bFor64>
		std::wstring Datacenter::Structure::AttributeItem<bForServer, bFor64>::BuildPath(const ElementItem<bForServer, bFor64>* parent) const
		{
			std::wstring outPath = parent->BuildPath();
			outPath += L'.';
			outPath += NameRef;
			return std::move(outPath);
		}

		template<bool bForServer, bool bFor64>
		void Datacenter::Structure::AttributeItem<bForServer, bFor64>::ApplyValueChanges()
		{
			if (HasBeenEdited()) {
				//Update original
				if (IsInt()) {
					IntValue = (int)atol((const char*)Utf8ValueCache.data());
					ClearCachedUtf8();
					GetUtf8Value();
				}
				else if (IsFloat()) {
					FloatValue = (float)atof((const char*)Utf8ValueCache.data());
					ClearCachedUtf8();
					GetUtf8Value();
				}
				else {
					//this needs reconstruction of the Value string blocks
				}
			}
		}
#pragma  endregion

#pragma region ElementItem

		template<bool bForServer, bool bFor64>
		const size_t Datacenter::Structure::ElementItem<bForServer, bFor64>::BuildAttributesSignature(const std::vector<std::wstring>* excludedAttributes, const std::vector<std::wstring>* signatureAttributes) const
		{
			//@TODO
		}

		template<bool bForServer, bool bFor64>
		const AttributeItem<bForServer, bFor64>* Datacenter::Structure::ElementItem<bForServer, bFor64>::GetAttributeEx(const wchar_t* names[], size_t count) const
		{

		}

		template<bool bForServer, bool bFor64>
		const AttributeItem<bForServer, bFor64>* Datacenter::Structure::ElementItem<bForServer, bFor64>::GetAttribute(const wchar_t* name) const
		{

		}

		template<bool bForServer, bool bFor64>
		std::vector<const AttributeItem<bForServer, bFor64>*> Datacenter::Structure::ElementItem<bForServer, bFor64>::GetKeys(S1DataCenter<bForServer, bFor64>* DC) const
		{
			std::vector<const AttributeItem<bForServer, bFor64>*> result;

			if (!IsIndexEnabled()) {
				return std::move(result);
			}

			const auto Index = DC->Indices.Data[GetIndexValue()];

			if (Index.Key1) {
				result.push_back(GetAttribute(DC->NamesMap.AllStrings.Data[Index.Key1 - 1].StringRef));
			}

			if (Index.Key2) {
				result.push_back(GetAttribute(DC->NamesMap.AllStrings.Data[Index.Key2 - 1].StringRef));
			}

			if (Index.Key3) {
				result.push_back(GetAttribute(DC->NamesMap.AllStrings.Data[Index.Key3 - 1].StringRef));
			}

			if (Index.Key4) {
				result.push_back(GetAttribute(DC->NamesMap.AllStrings.Data[Index.Key4 - 1].StringRef));
			}

			return std::move(result);
		}

#pragma endregion

#pragma region Metadata
		template<bool bForServer, bool bFor64>
		bool BuildAttributeTypesMetadataRecursive(Datacenter::Structure::ElementItem<bForServer, bFor64>* Item, Datacenter::Structure::S1DataCenterMetadata<bForServer, bFor64>* Meta) {
			for (auto* attr : Item->Attributes) {
				auto path = attr->BuildPath(Item);

				auto existing = Meta->AttributesTypes.find(path);
				if (existing == Meta->AttributesTypes.end()) {
					Meta->AttributesTypes.insert({ path, attr->TypeInfo });
				}
			}

			for (auto* child : Item->Children) {
				if (!BuildAttributeTypesMetadataRecursive(child, Meta)) {
					return false;
				}
			}

			return true;
		}

		template<bool bForServer, bool bFor64>
		bool BuildElementsIndicesMetadataRecursive(Datacenter::Structure::ElementItem<bForServer, bFor64>* Item, Datacenter::Structure::S1DataCenterMetadata<bForServer, bFor64>* Meta, Datacenter::Structure::S1DataCenter<bForServer, bFor64>* DC) {
			if (Item->IsIndexEnabled()) {
				const auto& Index = DC->Indices.Data[Item->GetIndexValue()];
				std::wstring path = Item->BuildPath();

				do {
					//find if already processed
					auto existing = Meta->ElementsIndices.find(path);
					if (existing != Meta->ElementsIndices.end()) {
						break;
					}

					//Process keys
					Datacenter::Structure::S1DataCenterMetadata::ElementIndices NewIndex;

					if (Index.Key1) {
						uint16_t i = Meta->HasIndexName(Index.Name1);
						if (i == 0)
						{
							i = Meta->AddIndexName(Index.Name1);
						}
						NewIndex.Key1 = i;
					}

					if (Index.Key2) {
						uint16_t i = Meta->HasIndexName(Index.Name2);
						if (i == 0)
						{
							i = Meta->AddIndexName(Index.Name2);
						}
						NewIndex.Key2 = i;
					}

					if (Index.Key3) {
						uint16_t i = Meta->HasIndexName(Index.Name3);
						if (i == 0)
						{
							i = Meta->AddIndexName(Index.Name3);
						}
						NewIndex.Key3 = i;
					}

					if (Index.Key4) {
						uint16_t i = Meta->HasIndexName(Index.Name4);
						if (i == 0)
						{
							i = Meta->AddIndexName(Index.Name4);
						}
						NewIndex.Key4 = i;
					}

					Meta->ElementsIndices.insert({ path, NewIndex });
				} while (false);
			}

			for (ElementItem* child : Item->Children) {
				if (!BuildElementsIndicesMetadataRecursive(child, Meta, DC)) {
					return false;
				}
			}

			return true;
		}

		template<bool bForServer, bool bFor64>
		bool Datacenter::Structure::S1DataCenterMetadata<bForServer, bFor64>::BuildFromLoadedDC(S1DataCenter<bForServer, bFor64>* DC)
		{
			Clear();

			//Preallocations
			IndicesNames.resize(1024);
			AttributesTypes.reserve(8049);
			ElementsIndices.reserve(8049);

			auto* Root = DC->GetRootElement();

			for (auto* RootChild : Root->Children) {
				//1 Attributes type
				if (!BuildAttributeTypesMetadataRecursive(RootChild, this)) {
					Message("Failed to build attribute types metadata!");
					return false;
				}

				//2 Elements indices
				if (!BuildElementsIndicesMetadataRecursive(RootChild, this, DC)) {
					Message("Failed to build elements indices metadata!");
					return false;
				}
			}

			return true;
		}
#pragma endregion

#pragma region S1DataStructure

		template<bool bForServer, bool bFor64>
		inline bool S1DataCenter<bForServer, bFor64>::Serialize(FIStream& Stream, bool bDoPostLoad)
		{
			if constexpr (!bForServer) {
				if (Stream.IsLoading()) {
					FormatVersion = Stream.ReadInt32();
					Unk1_8 = Stream.ReadInt64();
					Version = Stream.ReadInt32();
				}
				else {
					Stream.WriteInt32(FormatVersion);
					Stream.Write_int64(Unk1_8);
					Stream.WriteInt32(Version);
				}
			}

			if constexpr (!bForServer) {
				if (!Unk.Serialize(Stream)) {
					return false;
				}

				if (!Indices.Serialize(Stream)) {
					return false;
				}
			}

			if (!Attributes.Serialize(Stream)) {
				return false;
			}

			if (!Elements.Serialize(Stream)) {
				return false;
			}

			if (!Stream.IsLoading()) {
				//Values map is not used by client !
				for (auto& buckets : ValuesMap.Buckets.Buckets) {
					buckets.Clear();
				}
			}

			if (!ValuesMap.Serialize(Stream)) {
				return false;
			}

			if (!NamesMap.Serialize(Stream)) {
				return false;
			}

			if constexpr (!bForServer) {
				if (Stream.IsLoading()) {
					EndCount = Stream.ReadUInt32();
				}
				else {
					Stream.WriteUInt32(EndCount);
				}
			}

			if (Stream.IsLoading()) {
				bIsLoaded = true;

				if (bDoPostLoad) {
					return PostLoad();
				}
			}

			return true;
		}

		template<bool bForServer, bool bFor64>
		bool Datacenter::Structure::S1DataCenter<bForServer, bFor64>::PostLoad()
		{
			if (!ValuesMap.Prepare()) {
				return false;
			}

			if (!NamesMap.Prepare()) {
				return false;
			}

			if (!PostLoadAttributes()) {
				return false;
			}

			if (!PostLoadElements()) {
				return false;
			}

			//Remove all invalid elements
			int32_t i = 0;
			while (i < (GetRootElement()->Children.size())) {
				if (!GetRootElement()->Children[i]->IsValid()) {
					GetRootElement()->Children.erase(GetRootElement()->Children.begin() + i);
					i = 0;
					continue;
				}

				i++;
			}

			SetParents(GetRootElement(), nullptr);

			BuildIndicesCache();

			//Build metadata
			/*Metadata.reset(new S1DataCenterMetadata());
			if (!Metadata->BuildFromLoadedDC(this)) {
				return false;
			}*/

			OnAfterLoaded();

			return true;
		}

		template<bool bForServer, bool bFor64>
		bool Datacenter::Structure::S1DataCenter<bForServer, bFor64>::PostLoadAttributes()
		{
			for (auto& AttributesBlock : Attributes.Data) {

				for (auto& Attribute : AttributesBlock.Data) {
					if (Attribute.IsString()) {
						Attribute.StringRef = ValuesMap.GetString(Attribute.Indices.first, Attribute.Indices.second);
					}
					else {
						Attribute.StringRef = nullptr;
					}
					if (Attribute.NameId) {
						Attribute.NameRef = NamesMap.AllStrings.Data[Attribute.NameId - 1].StringRef;
					}
					else {
						Message("PostLoadAttributes::Attribute with name Id 0 ??");
					}
				}
			}

			return true;
		}

		template<bool bForServer, bool bFor64>
		bool Datacenter::Structure::S1DataCenter<bForServer, bFor64>::PostLoadElements()
		{
			for (size_t i = 0; i < Elements.Data.size(); i++) {
				for (size_t j = 0; j < Elements.Data[i].Data.size(); j++)
				{
					if (!Elements.Data[i].Data[j].IsValid()) {
						continue;
					}

					Elements.Data[i].Data[j].NameRef = NamesMap.AllStrings.Data[Elements.Data[i].Data[j].NameId - 1].StringRef;
					Elements.Data[i].Data[j].LocationCache = { i , j };
				}
			}

			if (!PostLoadElementsRecursive(GetRootElement(), nullptr)) {
				return false;
			}

			return true;
		}

		template<bool bForServer, bool bFor64>
		bool Datacenter::Structure::S1DataCenter<bForServer, bFor64>::PostLoadElementsRecursive(MyElementType* element, MyElementType* parent)
		{
			if (parent) {
				//if (element->Parent) { //duplicate if has parent
				//	element = new ElementItem(*element);
				//	element->IsDuplicated = true;
				//	parent->Children.push_back(element);

				//	return true;
				//}

				if (element->Parent) { //shared node, inc ref
					element->AddRef();

					parent->Children.push_back(element);

					return true;
				}

				parent->Children.push_back(element);

				element->ParentIndices = parent->LocationCache;
			}

			element->Parent = parent;
			element->NameRef = NamesMap.AllStrings.Data[element->NameId - 1].StringRef;

			if constexpr (bForServer) {
				//Cache Value string
				if (element->IsCommentElement || element->IsValueElement) {
					element->ValueRef = ValuesMap.GetString(element->ValueIndices);
				}

				//Cache FileName string
				if (element->FileNameStrIndices.first != UINT16_MAX && element->FileNameStrIndices.second != UINT16_MAX) {
					element->FileNameRef = NamesMap.GetString(element->FileNameStrIndices);
				}
			}

			for (size_t i = 0; i < element->AttributesCount; i++) {
				element->Attributes.push_back(GetAttribute({ element->AttributesIndices.first, element->AttributesIndices.second + i }));
			}

			if (element->IsIndexEnabled()) {
				Indices.Data[element->GetIndexValue()].ElementsCount++;

				if (Indices.Data[element->GetIndexValue()].Key1) {
					Indices.Data[element->GetIndexValue()].Name1 = NamesMap.AllStrings.Data[Indices.Data[element->GetIndexValue()].Key1 - 1].StringRef;
				}

				if (Indices.Data[element->GetIndexValue()].Key2) {
					Indices.Data[element->GetIndexValue()].Name2 = NamesMap.AllStrings.Data[Indices.Data[element->GetIndexValue()].Key2 - 1].StringRef;
				}

				if (Indices.Data[element->GetIndexValue()].Key3) {
					Indices.Data[element->GetIndexValue()].Name3 = NamesMap.AllStrings.Data[Indices.Data[element->GetIndexValue()].Key3 - 1].StringRef;
				}

				if (Indices.Data[element->GetIndexValue()].Key4) {
					Indices.Data[element->GetIndexValue()].Name4 = NamesMap.AllStrings.Data[Indices.Data[element->GetIndexValue()].Key4 - 1].StringRef;
				}
			}

			for (size_t i = 0; i < element->ChildCount; i++) {

				auto* child = GetElement({ element->ChildrenIndices.first , element->ChildrenIndices.second + i });
				child->LocationCache = { element->ChildrenIndices.first , element->ChildrenIndices.second + i };

				if (!PostLoadElementsRecursive(child, element)) {
					return false;
				}
			}

			return true;
		}

		template<bool bForServer, bool bFor64>
		void Datacenter::Structure::S1DataCenter<bForServer, bFor64>::OnAfterLoaded()
		{
			//Cache elements for fast edit here
			auto Root = GetRootElement();

			for (auto* item : Root->Children) {
				if (!_wcsicmp(item->NameRef, L"ItemData")) {
					for (auto* i : item->Children) {
						AllItemElements.push_back(i);
					}
				}
				else if (!_wcsicmp(item->NameRef, L"SkillData")) {
					for (auto* i : item->Children) {
						AllSkillElements.push_back(i);
					}
				}
			}
		}

		template<bool bForServer, bool bFor64>
		static void BuildIndicesCacheRecursive(S1DataCenter<bForServer, bFor64>* DC, ElementItem<bForServer, bFor64>* element) {
			if (element->IsIndexEnabled()) {
				auto name = element->BuildPath();

				auto item = DC->CachedElementIndices.find(name);
				if (item == DC->CachedElementIndices.end()) {
					const uint16_t IndexValue = (element->Index & ELEMENT_INDEX_VALUE_MASK) >> 4;

					CachedElementIndex Index;
					Index.Name1 = DC->Indices.Data[IndexValue].Name1;
					Index.Name2 = DC->Indices.Data[IndexValue].Name2;
					Index.Name3 = DC->Indices.Data[IndexValue].Name3;
					Index.Name4 = DC->Indices.Data[IndexValue].Name4;

					Index.ElementNameCacheRef = element->NameRef;

					Index.Index = IndexValue;
					Index.Flags = element->Index & ELEMENT_INDEX_FLAGS_MASK;

					DC->CachedElementIndices.insert({
						std::move(name),
						std::move(Index)
						});
				}
			}

			for (auto* item : element->Children) {
				BuildIndicesCacheRecursive(DC, item);
			}
		}

		template<bool bForServer, bool bFor64>
		void Datacenter::Structure::S1DataCenter<bForServer, bFor64>::BuildIndicesCache()
		{
			for (auto* item : GetRootElement()->Children) {
				BuildIndicesCacheRecursive(this, item);
			}
		}

		template<bool bForServer, bool bFor64>
		void Datacenter::Structure::S1DataCenter<bForServer, bFor64>::AssociateIndices(MyElementType* element)
		{
			const auto path = element->BuildPathByGetRefs(this);

			auto item = CachedElementIndices.find(path);
			if (item != CachedElementIndices.end()) {
				const uint16_t Index = (uint16_t)(item->second.Index << 4);

				element->SetIndex(Index, item->second.Flags);
			}

			for (size_t i = 0; i < element->ChildCount; i++) {
				const auto indices = element->ChildrenIndices;
				AssociateIndices(GetElement({ indices.first , indices.second + i }));
			}
		}

		template<bool bForServer, bool bFor64>
		bool Datacenter::Structure::S1DataCenter<bForServer, bFor64>::BuildNamesIndices()
		{
			for (size_t i = 0; i < NamesMap.AllStrings.Data.size(); i++) {

				CachedNamesIndices.insert({
					NamesMap.AllStrings.Data[i].CachedString, (int32_t)i
					});
			}
		}

		template<bool bForServer, bool bFor64>
		bool Datacenter::Structure::S1DataCenter<bForServer, bFor64>::BuildIndices()
		{

			////1.Order Names by cached order
			//for (auto& item : CachedNamesIndices) {
			//	bool found = false;
			//	for (INT i = 0; i < NamesMap.AllStrings.Data.size(); i++) {
			//		if (item.first == NamesMap.AllStrings.Data[i].CachedString) {
			//			if (i != item.second) {
			//				continue;
			//			}

			//			found = true;

			//			//swap
			//			auto Temp = std::move(NamesMap.AllStrings.Data[i]);
			//			NamesMap.AllStrings.Data[i] = std::move(NamesMap.AllStrings.Data[item.second]);
			//			NamesMap.AllStrings.Data[item.second] = std::move(Temp);

			//			break;
			//		}
			//	}

			//	if (!found) {
			//		Message("Failed to find ordering for Name[%ws]", item.first.c_str());
			//	}
			//}

			//1.Associate indices from cache
			for (size_t i = 0; i < GetRootElement()->ChildCount; i++)
			{
				const auto indices = GetRootElement()->ChildrenIndices;
				AssociateIndices(GetElement({ indices.first , indices.second + i }));
			}

			//2. Search for name attribute indices and add references to string entry
			/*for (size_t i = 0; i < Indices.Data.size(); i++) {
				auto& item = Indices.Data[i];

				if (item.Name1Cached.size()) {
					if ((item.Key1 = NamesMap.FindString(item.Name1Cached.c_str()), !item.Key1)) {
						Message("Failed to find indexed attribute named[%ws].", item.Name1Cached.c_str());
						return false;
					}
				}

				if (item.Name2Cached.size()) {
					if ((item.Key2 = NamesMap.FindString(item.Name2Cached.c_str()), !item.Key2)) {
						Message("Failed to find indexed attribute named[%ws].", item.Name2Cached.c_str());
						return false;
					}
				}

				if (item.Name3Cached.size()) {
					if ((item.Key3 = NamesMap.FindString(item.Name3Cached.c_str()), !item.Key3)) {
						Message("Failed to find indexed attribute named[%ws].", item.Name3Cached.c_str());
						return false;
					}
				}

				if (item.Name4Cached.size()) {
					if ((item.Key4 = NamesMap.FindString(item.Name4Cached.c_str()), !item.Key4)) {
						Message("Failed to find indexed attribute named[%ws].", item.Name4Cached.c_str());
						return false;
					}
				}

				if (item.Key1) {
					NamesMap.AllStrings.Data[item.Key1 - 1].RefIndices.push_back({ (WORD)i , 1 });
				}
				if (item.Key2) {
					NamesMap.AllStrings.Data[item.Key2 - 1].RefIndices.push_back({ (WORD)i , 2 });
				}
				if (item.Key3) {
					NamesMap.AllStrings.Data[item.Key3 - 1].RefIndices.push_back({ (WORD)i , 3 });
				}
				if (item.Key4) {
					NamesMap.AllStrings.Data[item.Key4 - 1].RefIndices.push_back({ (WORD)i , 4 });
				}
			}*/

			//3. Sort strings by (have indices ref) and then by name
			//std::sort(NamesMap.AllStrings.Data.begin(), NamesMap.AllStrings.Data.end(),
			//	[](const DCMap::StringEntry& left, const DCMap::StringEntry& right)
			//	{
			//		const auto lhs_root = wcscmp(left.CachedString, L"__root__") != 0;
			//		const auto rhs_root = wcscmp(right.CachedString, L"__root__") != 0;
			//		const auto lhs_indices = left.RefIndices.size() == 0;
			//		const auto rhs_indices = right.RefIndices.size() == 0;
			//		return std::tie(
			//			lhs_root,
			//			lhs_indices)
			//			<
			//			std::tie(
			//				rhs_root,
			//				rhs_indices
			//			) /*? (std::wstring_view(left.CachedString) < std::wstring_view(right.CachedString)) : false*/;
			//	});

			//4. Restore correct name indices
			/*for (size_t i = 0; i < NamesMap.AllStrings.Data.size(); i++) {

				for (auto& item : NamesMap.AllStrings.Data[i].RefAttributes) {
					GetAttribute(item)->NameId = (NameIndexType)(i + 1);
				}

				for (auto& item : NamesMap.AllStrings.Data[i].RefElements) {
					GetElement(item)->Name = (NameIndexType)(i + 1);
				}

				for (auto& item : NamesMap.AllStrings.Data[i].RefIndices) {
					if (item.second == 1) {
						Indices.Data[item.first].Key1 = (WORD)(i + 1);
					}
					else if (item.second == 2) {
						Indices.Data[item.first].Key2 = (WORD)(i + 1);
					}
					else if (item.second == 3) {
						Indices.Data[item.first].Key3 = (WORD)(i + 1);
					}
					else if (item.second == 4) {
						Indices.Data[item.first].Key4 = (WORD)(i + 1);
					}
				}
			}*/

			//DumpIndicesByRefs();

			return true;
		}

#pragma endregion

#pragma region Editing
		template<bool bForServer, bool bFor64>
		using ToTranslateAttrListType = std::vector<std::pair<const ElementItem<bForServer, bFor64>*, const AttributeItem<bForServer, bFor64>*>>;

		template<bool bForServer, bool bFor64>
		static void BuildStringAttributesListRecursive(ElementItem<bForServer, bFor64>* element, ToTranslateAttrListType<bForServer, bFor64>& outAttributes, bool cacheValue = false) {
			for (auto* attr : element->Attributes) {
				if (attr->IsString()) {
					if (cacheValue) {
						attr->GetUtf8Value();
					}

					outAttributes.push_back({ element, attr });
				}
			}

			for (auto* child : element->Children) {
				BuildStringAttributesListRecursive(child, outAttributes, cacheValue);
			}
		}

		template<bool bForServer, bool bFor64>
		void Datacenter::Structure::S1DataCenter<bForServer, bFor64>::ApplyValueChangesRecursive(MyElementType* Item)
		{
			for (auto* attr : Item->Attributes) {
				attr->ApplyValueChanges();
			}

			for (auto* child : Item->Children) {
				ApplyValueChangesRecursive(child);
			}
		}

		template<bool bForServer, bool bFor64>
		bool Datacenter::Structure::S1DataCenter<bForServer, bFor64>::RebuildElementsListRecursive(ElementItem<bForServer, bFor64>* element, TBlockIndices location, bool isRoot)
		{
			ElementItem<bForServer, bFor64>* localElement = GetElement(location);

			//Copy
			(*localElement) = *element;
			if (localElement->NameId == 0) {
				std::wstring path = element->BuildPath();
				Message("Element %ws has \nname 0", path.c_str());
				DebugBreak();
			}

			localElement->LocationCache = location;

			element->BuiltLocation = location;

			//clear children
			localElement->ChildCount = (uint16_t)element->Children.size();
			localElement->ChildrenIndices = { 0,0 };

			//clear attributes
			localElement->AttributesCount = (uint16_t)element->Attributes.size();
			localElement->AttributesIndices = { 0,0 };

			if (element->IsCloned) {
				std::sort(element->Attributes.begin(), element->Attributes.end(), [](const AttributeItem<bForServer, bFor64>* first, const AttributeItem<bForServer, bFor64>* second) -> bool {
					if (first->NameId > second->NameId) {
						return false;
					}

					return true;
					});
			}

			if (element->Attributes.size()) {
				//Allocate attributes
				if (!AllocateAttributesSection((int32_t)element->Attributes.size(), localElement->AttributesIndices)) {
					Message("Failed to allocate Attributes section %lld", element->Attributes.size());
					return false;
				}

				//Move in the attributes data
				for (size_t i = 0; i < element->Attributes.size(); i++)
				{
					auto* attr = GetAttribute({ localElement->AttributesIndices.first , localElement->AttributesIndices.second + (BlockIndexType)i });

					//Copy
					(*attr) = *element->Attributes[i];
					if (attr->NameId == 0) {
						auto path = attr->BuildPath(element);
						Message("Attr %ws has \nname 0", path.c_str());
						DebugBreak();
					}

					attr->LocationCache = { localElement->AttributesIndices.first , localElement->AttributesIndices.second + (BlockIndexType)i };
					attr->Index = i;
				}
			}

			if (element->Children.size() == 0) {
				return true;
			}

			//Sort by indices if active
			if (!isRoot) {
				std::sort(element->Children.begin(), element->Children.end(), [&](const ElementItem<bForServer, bFor64>* first, const ElementItem<bForServer, bFor64>* second) -> bool {
					if (!first->IsIndexEnabled()) {
						return false;
					}

					std::vector<const AttributeItem<bForServer, bFor64>*> firstKeys = first->GetKeys(this);
					std::vector<const AttributeItem<bForServer, bFor64>*> secondKeys = second->GetKeys(this);

					if (firstKeys.size() != secondKeys.size()) {
						//Message("Compared elements key count missmatch!");
						return false;
					}

					for (size_t i = 0; i < firstKeys.size(); i++)
					{
						if (!firstKeys[i] || !secondKeys[i]) {
							return false;
						}

						if (firstKeys[i]->IsString()) {
							int cmp = wcscmp(firstKeys[i]->StringRef, secondKeys[i]->StringRef);
							if (cmp == 0) {
								continue;
							}

							return cmp < 0;
						}
						else if (firstKeys[i]->IsFloat()) {
							if (std::abs(firstKeys[i]->FloatValue - secondKeys[i]->FloatValue) <= 0.0001f) {
								continue;
							}

							return firstKeys[i]->FloatValue < secondKeys[i]->FloatValue;
						}
						else {
							if (firstKeys[i]->IntValue == secondKeys[i]->IntValue) {
								continue;
							}

							return firstKeys[i]->IntValue < secondKeys[i]->IntValue;
						}
					}

					return false;

					});
			}

			auto existing = GExistingElements.find(element->GetChildrenKey());
			if (existing != GExistingElements.end()) {
				GetElement(location)->ChildrenIndices = existing->second;
			}
			else {
				TBlockIndices childrenLocation;
				if (!AllocateElementsSection((int32_t)element->Children.size(), childrenLocation)) {
					Message("Fatal, failed to allocate %lld elements!", element->Children.size());
					return false;
				}

				for (size_t i = 0; i < element->Children.size(); i++)
				{
					if (!RebuildElementsListRecursive(element->Children[i], { childrenLocation.first , childrenLocation.second + (BlockIndexType)i }, false))
					{
						Message("Failed to build elements recursively [%ws]!", GetElement(location)->NameRef);
						return false;
					}
				}

				element = GetElement(location);

				GExistingElements.insert({
					element->GetChildrenKey(),
					childrenLocation
					});

				element->ChildrenIndices = childrenLocation;
			}

			return true;
		}

		template<bool bForServer, bool bFor64>
		bool Datacenter::Structure::S1DataCenter<bForServer, bFor64>::RebuildElementsAndAttributesList()
		{
			GExistingElements.clear();

			S1DataCenter::AttributesBlocks TempAttributes = std::move(Attributes);
			Attributes.Clear();

			S1DataCenter::ElementsBlocks TempElements = std::move(Elements);
			Elements.Clear();

			auto& root = TempElements.Data[0].Data[0];

			//add in the Root
			TBlockIndices location;
			if (!AllocateElementsSection(1, location)) {
				Message("Fatal, failed to allocate elements section fron __root__!");
				return false;
			}

			auto* __root__ = GetElement(location);

			auto result = RebuildElementsListRecursive(&root, location, true);

			GExistingElements.clear();

			return result;
		}

		template<bool bForServer, bool bFor64>
		bool Datacenter::Structure::S1DataCenter<bForServer, bFor64>::Rebuild()
		{
			//Apply numeric value changes
			for (auto* item : GetRootElement()->Children) {
				ApplyValueChangesRecursive(item);
			}

			if (!RebuildValueStringsBlocksAndMap()) {
				return false;
			}

			if (!RebuildElementsAndAttributesList()) {
				return false;
			}

			EditedElements.clear();

			return true;
		}

		template<bool bForServer, bool bFor64>
		static void BuildStringAttributesList(ElementItem<bForServer, bFor64>* element, ToTranslateAttrListType<bForServer, bFor64>& outAttributes, bool cacheValue = false)noexcept {
			for (auto* attr : element->Attributes) {
				if (attr->IsString()) {
					if (cacheValue) {
						attr->GetUtf8Value();
					}

					outAttributes.push_back({ element, attr });
				}
			}

			for (auto* child : element->Children) {
				BuildStringAttributesList<bForServer, bFor64>(child, outAttributes, cacheValue);
			}
		}

		template<bool bForServer, bool bFor64>
		bool Datacenter::Structure::S1DataCenter<bForServer, bFor64>::RebuildValueStringsBlocksAndMap()
		{
			//Preparation
			ToTranslateAttrListType<bForServer, bFor64> allStringAttributes;
			const auto myRoot = GetRootElementC();
			for (const auto child : myRoot->Children) {
				BuildStringAttributesList<bForServer, bFor64>(child, allStringAttributes, true);
			}

			//Clear all strings
			ValuesMap.Clear();

			//Add empty buckets
			for (size_t i = 0; i < ValuesMap.Buckets.Count; i++)
			{
				ValuesMap.Buckets.Buckets.push_back(StringsBucket<bForServer, bFor64>());
			}

			//Add back the strings into the dc
			for (auto item : allStringAttributes) {
				auto attr = ((AttributeItem<bForServer, bFor64>*)item.second);

				const auto size = strlen(attr->Utf8ValueCache.data());
				if (size) {
					attr->StringyfiedValue.clear();
					attr->StringyfiedValue.resize(size + 1);

					if (!_MultiByteToWideChar(attr->Utf8ValueCache.data(), attr->StringyfiedValue.data(), size + 1)) {
						Message("RebuildValueStringsBlocksAndMap::Failed to convert string[%s] from UTF8 to UTF16", (const char*)attr->Utf8ValueCache.data());
						continue;
					}
				}
				else {
					attr->StringyfiedValue = L"";
				}

				if (!ValuesMap.InsertString(attr->StringyfiedValue.data(), attr->StringyfiedValue.size(), attr->Indices)) {
					Message("RebuildValueStringsBlocksAndMap::Failed to insert string into the DC [%ws]", attr->StringyfiedValue.c_str());
					return false;
				}

				//clear temp
				attr->StringyfiedValue.clear();

				//recache the string ref
				attr->StringRef = ValuesMap.GetString(attr->Indices);

				//recache the values
				attr->ClearCachedUtf8();
				attr->GetUtf8Value();
			}

			return true;
		}
#pragma  endregion

#pragma region Static Members

		template<bool bForServer, bool bFor64>
		std::unordered_map<uint64_t, TBlockIndices<bForServer, bFor64>> S1DataCenter<bForServer, bFor64>::GExistingElements;

#pragma endregion

	}

	template<bool bForServer, bool bFor64>
	using S1DataCenter = Structure::S1DataCenter<bForServer, bFor64>;

	using S1Client32BitDatacenter = S1DataCenter<false, false>;
	using S1Client64BitDatacenter = S1DataCenter<false, true>;

	using SkyalkeServerDatacenter = S1DataCenter<true, true>;
}