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
namespace CommonEx
{

#if COMMONEX_USE_DEFAULTS
	using TStreamOffsetType = uint16_t;
#endif

	//Used primarely for reading, base of all streams
	class RStream
	{
	public:
		using MyPtrType = uint8_t*;

		FORCEINLINE RStream()noexcept {}
		FORCEINLINE RStream(MyPtrType Data, int64_t Size) noexcept : Buffer(Data), Size(Size) {}
		FORCEINLINE RStream(MyPtrType Data, int64_t Size, int64_t Position) noexcept : Buffer(Data), Size(Size), Position(Position) {}

		//Cant copy
		RStream(const RStream& Other) = delete;
		RStream& operator=(const RStream& Other) = delete;

		//Move
		FORCEINLINE RStream(RStream&& Other) noexcept : Buffer(Other.Buffer), Size(Other.Size), Position(Other.Position)
		{
			Other.Release();
		}
		FORCEINLINE RStream& operator=(RStream&& Other) noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			Buffer = Other.Buffer;
			Size = Other.Size;
			Position = Other.Position;

			Other.Release();

			return *this;
		}

		FORCEINLINE MyPtrType		GetBuffer() const noexcept
		{
			return Buffer;
		}
		FORCEINLINE MyPtrType		GetFront() const noexcept
		{
			return Buffer + Position;
		}
		FORCEINLINE int64_t			GetSize() const noexcept
		{
			return Size;
		}
		FORCEINLINE int64_t			GetPosition() const noexcept
		{
			return Position;
		}
		FORCEINLINE void			Forward(int64_t Ammount)noexcept
		{
			Position += Ammount;
		}
		FORCEINLINE int64_t			GetRemainingSize() const noexcept
		{
			return GetSize() - GetPosition();
		}

		FORCEINLINE bool			Read(uint8_t* out, uint64_t size) noexcept
		{
#if DEBUG_STREAMS
			assert(size <= GetRemainingSize());
#endif

			if (memcpy_s((void*)out, size, (const void*)GetFront(), size))
			{
				return false;
			}

			Forward(size);
		}
		FORCEINLINE int8_t			ReadInt8() noexcept
		{
			return Read<int8_t>();
		}
		FORCEINLINE uint8_t			ReadUInt8() noexcept
		{
			return Read<uint8_t>();
		}
		FORCEINLINE int16_t			ReadInt16() noexcept
		{
			return Read<int16_t>();
		}
		FORCEINLINE uint16_t		ReadUInt16() noexcept
		{
			return Read<uint16_t>();
		}
		FORCEINLINE int32_t			ReadInt32() noexcept
		{
			return Read<int32_t>();
		}
		FORCEINLINE uint32_t		ReadUInt32() noexcept
		{
			return Read<uint32_t>();
		}
		FORCEINLINE int64_t			ReadInt64() noexcept
		{
			return Read<int64_t>();
		}
		FORCEINLINE uint64_t		XReadUInt64() noexcept
		{
			return Read<uint64_t>();
		}
		FORCEINLINE float			ReadFloat() noexcept
		{
			return Read<float>();
		}
		FORCEINLINE double			ReadDouble() noexcept
		{
			return Read<double>();
		}

		template<typename T>
		FORCEINLINE T& Read() noexcept
		{
			Forward(sizeof(T));
			return *reinterpret_cast<T*>(GetFront() - sizeof(T));
		}

		FORCEINLINE void			Release() noexcept
		{
			Buffer = nullptr;
			Size = 0;
			Position = 0;
		}
		//Is End of Stream
		FORCEINLINE bool			IsEOS() const noexcept
		{
			return Position == Size;
		}

		FORCEINLINE bool			Allocate(size_t Size) noexcept
		{
			if (Buffer)
			{
				return false;
			}

			Buffer = new uint8_t[Size];
			if (!Buffer)
			{
				return false;
			}

			this->Size = Size;
			Position = 0;

			return true;
		}

	protected:
		MyPtrType					Buffer = nullptr;
		int64_t						Size = 0;
		int64_t						Position = 0;
	};

	//Used for reading and writing
	class IStream : public RStream
	{
	public:
		FORCEINLINE IStream() noexcept : RStream() {}
		FORCEINLINE IStream(uint8_t* Data, int64_t Size) noexcept : RStream(Data, Size) {}
		FORCEINLINE IStream(uint8_t* Data, int64_t Size, int64_t Position) noexcept : RStream(Data, Size, Position) {}

		FORCEINLINE bool		CanFit(int64_t Size) const noexcept
		{
			return (GetSize() - GetPosition() - Size) >= 0;
		}

		FORCEINLINE bool		Write(const  uint8_t* Data, int64_t Size)  noexcept
		{
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
		FORCEINLINE void		WriteUInt8(const uint8_t data)
		{
			Write(data);
		}
		FORCEINLINE void		WriteInt16(const int16_t data) noexcept
		{
			Write(data);
		}
		FORCEINLINE void		WriteUInt16(const uint16_t data) noexcept
		{
			Write(data);
		}
		FORCEINLINE void		WriteInt32(const int32_t data) noexcept
		{
			Write(data);
		}
		FORCEINLINE void		WriteUInt32(const uint32_t data) noexcept
		{
			Write(data);
		}
		FORCEINLINE void		WriteInt64(const int64_t data) noexcept
		{
			Write(data);
		}
		FORCEINLINE void		WriteUInt64(const uint64_t data) noexcept
		{
			Write(data);
		}
		FORCEINLINE void		WriteFloat(const float data) noexcept
		{
			Write(data);
		}
		FORCEINLINE void		WriteDouble(const double data) noexcept
		{
			Write(data);
		}

		FORCEINLINE RStatus		WriteString(const char* String, bool bWriteEmptyIfNull = true) noexcept
		{
			if (String == nullptr)
			{
				if (bWriteEmptyIfNull)
				{
					WriteUInt8(0);
					return RSuccess;
				}

				return RInvalidParameters;
			}

			const auto size = strlen(String);
			if (!CanFit(size + 1))
			{
				return ROperationOverflows;
			}

			strcpy_s((char*)GetFront(), GetRemainingSize(), String);

			Forward(static_cast<int64_t>((int64_t)size + 1));

			return RSuccess;
		}
		FORCEINLINE RStatus		WriteString(const char* String, int32_t MaxLength, bool bWriteEmptyIfNull = true) noexcept
		{
			if (String == nullptr)
			{
				if (bWriteEmptyIfNull)
				{
					WriteUInt8(0);
					return RSuccess;
				}

				return RInvalidParameters;
			}

			const auto size = (int64_t)strnlen_s(String, MaxLength);
			if (!CanFit(size + 1))
			{
				return ROperationOverflows;
			}

			if (strcpy_s((char*)GetFront(), MaxLength, String))
			{
				return RFail;
			}

			Forward(size + 1);

			return RSuccess;
		}
		FORCEINLINE RStatus		WriteString(int32_t StringLength, const char* String, bool bWriteEmptyIfNull = true) noexcept
		{
			if (String == nullptr)
			{
				if (bWriteEmptyIfNull)
				{
					WriteUInt8(0);
					return RSuccess;
				}

				return RInvalidParameters;
			}

			strcpy_s((char*)GetFront(), StringLength + 1, String);

			Forward(StringLength + 1);

			return RSuccess;
		}

		FORCEINLINE RStatus		WriteWString(const wchar_t* String, bool bWriteEmptyIfNull = true) noexcept
		{
			if (!String)
			{
				if (!bWriteEmptyIfNull)
				{
					return RInvalidParameters;
				}

				WriteUInt16(0); //sizeof(whcar_t) == 2
				return RSuccess;
			}

			const int64_t Length = wcslen(String);
			if (!CanFit(Length))
			{
				return ROperationOverflows;
			}

			if (wcscpy_s((wchar_t*)GetFront(), GetRemainingSize(), String))
			{
				return RFail;
			}

			Forward((size_t)Length + 2);

			return RSuccess;
		}
		FORCEINLINE RStatus		WriteWString(const wchar_t* String, int32_t MaxLengthInWchar_t, bool bWriteEmptyIfNull = true) noexcept
		{
			if (!String)
			{
				if (!bWriteEmptyIfNull)
				{
					return RInvalidParameters;
				}

				WriteUInt16(0); //sizeof(whcar_t) == 2
				return RSuccess;
			}

			const int64_t Length = wcsnlen_s(String, MaxLengthInWchar_t);
			if (!CanFit(Length))
			{
				return ROperationOverflows;
			}

			wcscpy_s((wchar_t*)GetFront(), MaxLengthInWchar_t, String);

			Forward((size_t)Length + 2);

			return RSuccess;
		}
		FORCEINLINE RStatus		WriteWString(int32_t StringLengthInWchar_t, const wchar_t* String, bool bWriteEmptyIfNull = true) noexcept
		{
			if (!String)
			{
				if (!bWriteEmptyIfNull)
				{
					return RInvalidParameters;
				}

				WriteUInt16(0); //sizeof(whcar_t) == 2
				return RSuccess;
			}

			wcscpy_s((wchar_t*)GetFront(), StringLengthInWchar_t + 1, String);

			Forward((size_t)StringLengthInWchar_t + 2);

			return RSuccess;
		}

		FORCEINLINE void		WritePosAt(const TStreamOffsetType at) noexcept
		{
			WriteAt<TStreamOffsetType>((TStreamOffsetType)Position, at);
		}

		//Will advance the position by [BytesCount] and return the offset before the advancment
		//	Use this to allocate space inside the buffer 
		//	eg. const auto ElementsPacketOffset = stream.Skip(); 
		//		... Write more data here and right before you start to write "Elements" you write back the offset 
		//		stream.WritePosAt(ElementsPacketOffset);
		_NODISCARD FORCEINLINE int64_t	Skip(TStreamOffsetType BytesCount = sizeof(TStreamOffsetType)) noexcept
		{
			Forward(BytesCount);
			return GetPosition() - BytesCount;
		}

		FORCEINLINE void		WriteSGUID(const SGUID data) noexcept
		{
			w_u32(GetFront(), data.GetRaw());
			Forward(sizeof(SGUID));
		}

		FORCEINLINE void		ZeroOut() noexcept
		{
#if DEBUG_STREAMS
			assert(Size > 0);
#endif

			memset(Buffer, 0, Size);
		}

		FORCEINLINE void		Clear() noexcept
		{
			Size = 0;
			Position = 0;
		}

		template<typename T>
		FORCEINLINE void		WriteAt(T Value, int64_t Position) noexcept
		{
			*reinterpret_cast<T*>(GetBuffer() + Position) = Value;
		}

		template<typename T>
		FORCEINLINE void		Write(T Value) noexcept
		{
			*reinterpret_cast<T*>(GetBuffer() + Position) = Value;
			Forward(sizeof(T));
		}
	};
}