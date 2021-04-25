#pragma once
/**
 * @file Stream.h
 *
 * @brief CommonEx stream abstractions
 *			RStream - reading capabilities
 *			IStream - reading and writing capabilities
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */
namespace CommonEx {

#if COMMONEX_USE_DEFAULTS
	using TStreamOffsetType = uint16_t;
#endif


	//Used primarely for reading, base of all streams
	struct RStream {
		using MyPtrType = uint8_t*;

		RStream() {}
		RStream(MyPtrType Data, int64_t Size) : Buffer(Data), Size(Size) {}
		RStream(MyPtrType Data, int64_t Size, int64_t Position) : Buffer(Data), Size(Size), Position(Position) {}

		//Cant copy
		RStream(const RStream& Other) = delete;
		RStream& operator=(const RStream& Other) = delete;

		//Move
		RStream(RStream&& Other) noexcept : Buffer(Other.Buffer), Size(Other.Size), Position(Other.Position) {
			Other.Release();
		}
		RStream& operator=(RStream&& Other) noexcept {
			if (this == &Other) {
				return *this;
			}

			Buffer = Other.Buffer;
			Size = Other.Size;
			Position = Other.Position;

			Other.Release();

			return *this;
		}

		inline MyPtrType		GetBuffer() const noexcept {
			return Buffer;
		}
		inline MyPtrType		GetFront() const noexcept {
			return Buffer + Position;
		}
		inline int64_t			GetSize() const noexcept {
			return Size;
		}
		inline int64_t			GetPosition() const noexcept {
			return Position;
		}
		inline void				Forward(int64_t Ammount)noexcept {
			Position += Ammount;
		}
		inline int64_t			GetRemainingSize() const noexcept {
			return GetSize() - GetPosition();
		}

		inline bool				Read(uint8_t* out, uint64_t size) noexcept {
#if DEBUG_STREAMS
			assert(size <= GetRemainingSize());
#endif

			if (memcpy_s((void*)out, size, (const void*)GetFront(), size)) {
				return false;
			}

			Forward(size);
		}
		inline int8_t			ReadInt8() noexcept {
			return Read<int8_t>();
		}
		inline uint8_t			ReadUInt8() noexcept {
			return Read<uint8_t>();
		}
		inline int16_t			ReadInt16() noexcept {
			return Read<int16_t>();
		}
		inline uint16_t			ReadUInt16() noexcept {
			return Read<uint16_t>();
		}
		inline int32_t			ReadInt32() noexcept {
			return Read<int32_t>();
		}
		inline uint32_t			ReadUInt32() noexcept {
			return Read<uint32_t>();
		}
		inline int64_t			ReadInt64() noexcept {
			return Read<int64_t>();
		}
		inline uint64_t			ReadUInt64() noexcept {
			return Read<uint64_t>();
		}
		inline float			ReadFloat() noexcept {
			return Read<float>();
		}
		inline double			ReadDouble() noexcept {
			return Read<double>();
		}

		template<typename T>
		inline T& Read() noexcept {
			Forward(sizeof(T));
			return *reinterpret_cast<T*>(GetFront() - sizeof(T));
		}

		inline void				Release() noexcept {
			Buffer = nullptr;
			Size = 0;
			Position = 0;
		}
		//Is End of Stream
		inline bool				IsEOS() const noexcept {
			return Position == Size;
		}

		inline bool				Allocate(size_t Size) noexcept {
			if (Buffer) {
				return false;
			}

			Buffer = new uint8_t[Size];
			if (!Buffer) {
				return false;
			}

			this->Size = Size;
			Position = 0;

			return true;
		}

		//Mocs
		constexpr bool IsLoading() const noexcept { return true; }

	protected:
		MyPtrType				Buffer = nullptr;
		int64_t					Size = 0;
		int64_t					Position = 0;
	};

	//Used for reading and writing
	struct IStream : public RStream {
		IStream() : RStream() {}
		IStream(uint8_t* Data, int64_t Size) : RStream(Data, Size) {}
		IStream(uint8_t* Data, int64_t Size, int64_t Position) : RStream(Data, Size, Position) {}

		inline bool		CanFit(int64_t Size) const noexcept {
			return (GetSize() - GetPosition() - Size) >= 0;
		}

		inline bool		Write(const  uint8_t* Data, int64_t Size)  noexcept {
#if DEBUG_STREAMS
			assert(CanFit(Size) == true);
#endif

			if (memcpy_s(GetFront(), GetRemainingSize(), Data, Size))
			{
				return false;
			}

			Forward(Size);

			return true;
		}
		inline void		WriteUInt8(const uint8_t data) {
			Write(data);
		}
		inline void		WriteInt16(const int16_t data) noexcept {
			Write(data);
		}
		inline void		WriteUInt16(const uint16_t data) noexcept {
			Write(data);
		}
		inline void		WriteInt32(const int32_t data) noexcept {
			Write(data);
		}
		inline void		WriteUInt32(const uint32_t data) noexcept {
			Write(data);
		}
		inline void		WriteInt64(const int64_t data) noexcept {
			Write(data);
		}
		inline void		WriteUInt64(const uint64_t data) noexcept {
			Write(data);
		}
		inline void		WriteFloat(const float data) noexcept {
			Write(data);
		}
		inline void		WriteDouble(const double data) noexcept {
			Write(data);
		}
		inline void		WriteString(const char* str) noexcept {
			const auto size = strlen(str);
			uint8_t* ptr = GetFront();

			for (size_t i = 0; i < size; i++)
			{
				*ptr = *str;
				ptr++;
				str++;
			}

			*ptr = 0x00;
			Forward(static_cast<int64_t>((int64_t)size + 1));
		}
		inline bool		WriteWString(const wchar_t* str, const uint32_t len = 0, bool writeEmptyIfNull = true) noexcept {
			if (!str) {
				if (!writeEmptyIfNull) {
					return false;
				}

				Write((wchar_t)0);
			}
			else {
				if (!len)
				{
					while (str[0])
					{
						w_u16(GetFront(), str[0]);
						Forward(2);
						str++;
					}

					w_u16(GetFront(), 0);
					Forward(2);

					return true;
				}

				if (memcpy_s(GetFront(), ((size_t)len * 2) + 2, str, ((size_t)len * 2) + 2))
				{
					return false;
				}

				Forward((size_t)len + 2);
			}

			return true;
		}
		inline bool		WriteWString(const std::wstring_view& view, const uint32_t len = 0, bool writeEmptyIfNull = true) noexcept {
			return WriteWString(view.data(), len, writeEmptyIfNull);
		}

		inline void		WritePosAt(const TStreamOffsetType at) noexcept {
			WriteAt<TStreamOffsetType>((TStreamOffsetType)Position, at);
		}

		//Will advance the position by [BytesCount] and return the offset before the advancment
		//	Use this to allocate space inside the buffer 
		//	eg. const auto ElementsPacketOffset = stream.Skip(); 
		//		... Write more data here and right before you start to write "Elements" you write back the offset 
		//		stream.WritePosAt(ElementsPacketOffset);
		inline int64_t	Skip(TStreamOffsetType BytesCount = sizeof(TStreamOffsetType)) noexcept {
			Forward(BytesCount);
			return GetPosition() - BytesCount;
		}

		inline void		WriteSGUID(const SGUID data) noexcept {
			w_u32(GetFront(), data.GetRaw());
			Forward(sizeof(SGUID));
		}

		inline void		ZeroOut() noexcept {
#if DEBUG_STREAMS
			assert(Size > 0);
#endif

			memset(Buffer, 0, Size);
		}
		inline void		Clear() noexcept {
			Size = 0;
			Position = 0;
		}

		template<typename T>
		inline void		WriteAt(T Value, int64_t Position) noexcept {
			*reinterpret_cast<T*>(GetBuffer() + Position) = Value;
		}

		template<typename T>
		inline void		Write(T Value) noexcept {
			*reinterpret_cast<T*>(GetBuffer() + Position) = Value;
			Forward(sizeof(T));
		}
	};
}