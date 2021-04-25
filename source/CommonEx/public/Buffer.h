#pragma once

namespace CommonEx {
	struct TStream;

	enum EReceiveState : int32_t
	{
		EReceiveState_PacketHead,
		EReceiveState_PacketBody
	};

	enum EBroadcastType : int32_t
	{
		EBroadcastType_All,
		EBroadcastType_Visible,
		EBroadcastType_Area,
		EBroadcastType_World,
		EBroadcastType_Party,
		EBroadcastType_Raid,
		EBroadcastType_Enemies,
		EBroadcastType_Allies,
		EBroadcastType_Friends,
		EBroadcastType_Guild,
		EBroadcastType_DespawnPlayer,
		EBroadcastType_MAX,
	};

	/*------------------------------------------------------------
		Buffers Payload
	------------------------------------------------------------*/
	struct SendBufferPayload {
		EntityId	Connection{ 0 };
		ptr_t		ConnectionPtr{ nullptr };
	};

	struct RecvBufferPayload : SendBufferPayload {

	};

	/*------------------------------------------------------------
		SendBuffer (Fixed size Send Buffer)
	  ------------------------------------------------------------*/
	template <uint32_t _BufferSize>
	struct SendBuffer : Work<SendBufferPayload, SendBuffer<_BufferSize>>, MemoryBlockBase
	{
		using Base = Work<SendBufferPayload, SendBuffer<_BufferSize>>;

	private:
		//Interface.
		IBuffer IBuffer{};

		const uint32_t BufferSize{ _BufferSize };

		uint32_t Position{ 0 };

	public:
		union
		{
			struct
			{
				unsigned bIsPreallocated : 1;
			};
			uint32_t Flags{ 0 };
		};

	private:
		uint8_t Buffer[_BufferSize];

		friend TStream;

	public:
		SendBuffer() noexcept
			: Base(EWorkType_SendBuffer)
			, MemoryBlockBase(_BufferSize, Buffer, sizeof(uint8_t), _BufferSize)
		{
			this->bDontDeleteData = true;
			IBuffer.Buffer = Buffer;
		}
		SendBuffer(const SendBuffer& Other) noexcept
			: Base(EWorkType::SendBuffer)
			, MemoryBlockBase(_BufferSize, Buffer, sizeof(uint8_t), _BufferSize)
		{
			this->bDontDeleteData = true;

			(*this) = Other;

			IBuffer.Buffer = Buffer;
		}

		SendBuffer& operator=(const SendBuffer<_BufferSize>& Other) noexcept
		{
			if (!memcpy_s(Buffer, _BufferSize, Other.Buffer, _BufferSize))
			{
				Work::operator=(Other);

				Position = Other.Position;
			}

			return *this;
		}

		//Update the interface
		inline void Prepare() noexcept
		{
			//w_u16(&Buffer, Position);

			IBuffer.Buffer = Buffer;
			IBuffer.Length = Position;
		}

		inline RStatus Write(const uint8_t* Data, size_t Size) noexcept
		{
			if (!CanFit(Size))
			{
				return RFail;
			}

			RTRY_L_FMT(
				(RStatus)memcpy_s(GetFront(), GetRemaining(), Data, Size),
				"SendBuffer::Write(Size:{}) call to memcpy_s() failed!", Size) {}

			Position += Size;

			return RSuccess;
		}

		inline void WriteCount(uint16_t Value, uint16_t Offset) noexcept
		{
			Write(Value, Offset);
		}

		template <typename T>
		inline void Write(const T& Value) noexcept
		{
			*((T*)(GetFront())) = Value;
			Position += sizeof(T);
		}

		template <typename Char, size_t N>
		inline RStatus Write(const Char(&Message)[N]) noexcept
		{
			constexpr size_t StringFullSize = N * sizeof(Char);

			errno_t Result = memcpy_s((void*)GetFront(), StringFullSize, Message, StringFullSize);
			if (Result != 0)
			{
				return RFail;
			}

			Position += StringFullSize;

			return RSuccess;
		}

		template <typename T>
		inline void WriteAt(const T& Value, uint16_t Offset) noexcept
		{
			*((T*)(GetBuffer() + Offset)) = Value;
		}

		inline uint8_t* GetBuffer() noexcept
		{
			return Buffer;
		}

		inline const uint8_t* GetBuffer() const noexcept
		{
			return Buffer;
		}

		//Get the buffer at the position of the next write
		inline uint8_t* GetFront() noexcept
		{
			return Buffer + Position;
		}

		//Get the buffer at the position of the next write
		inline const uint8_t* GetFront() const noexcept
		{
			return Buffer + Position;
		}

		//Get remaning usable space in the buffer
		inline uint32_t GetRemaining() const noexcept
		{
			return BufferSize - Position;
		}

		//Get written ammount
		inline uint32_t GetSize() const noexcept
		{
			return Position;
		}

		inline CommonEx::IBuffer& GetInterface() noexcept
		{
			return IBuffer;
		}

		inline const CommonEx::IBuffer& GetInterface() const noexcept
		{
			return IBuffer;
		}

		//Can the buffer fit [Size] byts
		inline bool CanFit(uint32_t Size) const noexcept
		{
			return GetRemaining() >= Size;
		}

		//Returns TStream(this)
		//  If CurrentPositionAsBase = true , the TStream is based at the sendBuffer current position.
		TStream ToStream(bool CurrentPositionAsBase = true) noexcept;

		friend TStream;
	};

	//Pointer interface for SendBuffer<Size>
	using ISendBuffer = SendBuffer<1>;

	/*------------------------------------------------------------
		Fixed size PacketSendBuffer
	  ------------------------------------------------------------*/
	template <typename Packet>
	using PacketSendBuffer = SendBuffer<sizeof(Packet)>;

	/*------------------------------------------------------------
		RecvBuffer (Fixed size Receive Buffer)
	  ------------------------------------------------------------*/
	struct RecvBuffer : Work<RecvBufferPayload, RecvBuffer>, MemoryBlockBase
	{
		using Base = Work<RecvBufferPayload, RecvBuffer>;

		//Interface.
		IBuffer		IBuffer{};

		int32_t		IOState{ 0 };

		//Actual buffer.
		union {
			TPacketHeader	PacketHead;
			uint8_t			Buffer[CReceiveBufferSizeMax];
		};

		RecvBuffer() noexcept
			: Base(EWorkType_ReceiveBuffer)
			, MemoryBlockBase(CReceiveBufferSizeMax, Buffer, sizeof(uint8_t), CReceiveBufferSizeMax)
		{

		}

		constexpr inline void Reset(TPacketSize RecvLength = sizeof(TPacketHeader)) noexcept {
			IBuffer.Buffer = Buffer;
			IBuffer.Length = RecvLength;
			IOState = 0;

			memset(&this->WorkOverllapped, 0, sizeof(OsOverlappedType));
		}

		constexpr inline void Update(TPacketSize UpdateSize) noexcept {
			IBuffer.Buffer += UpdateSize;
			IBuffer.Length -= UpdateSize;
		}

		inline CommonEx::IBuffer& GetInterface() noexcept
		{
			return IBuffer;
		}

		inline const CommonEx::IBuffer& GetInterface() const noexcept
		{
			return IBuffer;
		}

		inline RStatus Write(const uint8_t* Buffer, TPacketSize Size)noexcept {
			if (memcpy_s(
				this->Buffer
				, CReceiveBufferSizeMax
				, Buffer
				, Size)) {
				return RFail;
			}

			return RSuccess;
		}

		template<typename T>
		const T& To() const noexcept {
			return *(T*)Buffer;
		}

		template<typename T>
		T& To() noexcept {
			return *(T*)Buffer;
		}
	};

	/*------------------------------------------------------------
		TStream
		All Write/Read(s) are temporary until Commit or CommitPacket is called or the TStream instance gets destroyed while holding the valid ref
	  ------------------------------------------------------------*/
	struct TStream : public IStream
	{
		TStream(ISendBuffer* Buffer, bool CurrentPositionAsBase) : IStream(Buffer->GetFront(), Buffer->GetRemaining(), 0)
		{
#if DEBUG_STREAMS
			assert(
				GetBuffer() != nullptr &&
				GetSize() > 0);
#endif

			Stream = Buffer;
		}
		TStream(TStream&& Other) noexcept : IStream(std::move(Other))
		{
			Stream = Other.Stream;
		}

		//Takes ownership of the Other.Stream
		inline TStream& operator=(TStream&& Other) noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			IStream::operator=(std::move(Other));

			Stream = Other.Stream;

			Other.Release();

			return *this;
		}

		//Cant copy
		TStream(const TStream& Other) = delete;
		TStream& operator=(const TStream& Other) = delete;

		//"Commits" changes to the underlaying Buffer
		//	If Rebase is true, after Commiting the
		//	instance, the Buffer(ptr) is pushed forward by the commited ammout
		//	and the Position is reseted to 0
		inline void Commit(bool Rebase = true) noexcept
		{
#if DEBUG_STREAMS
			assert(Stream.Get() != nullptr);
#endif

			//Commit changes by advancing the ref Stream position
			Stream->Position += (uint32_t)GetPosition();

			if (Rebase)
			{
				Buffer += GetPosition();
				Position = 0;
			}
		}

		//"Commits" changes to the underlaying Buffer and writes the total packet size in the first UINT16
		//	If Rebase is true, after Commiting the
		//	instance, the Buffer(ptr) is pushed forward by the commited ammout
		//	and the Position is reseted to 0
		inline void CommitPacket(bool Rebase = true) noexcept
		{
			WriteAt((TPacketSize)GetPosition(), 0); //we write the current position as the size of the packet

			Commit(Rebase);
		}

		//"Rolls back" changes by reseting the Position to 0
		inline void Rollback() noexcept
		{
#if DEBUG_STREAMS
			assert(Stream.Get() != nullptr);
#endif

			Position = 0;
		}

		inline void Release() noexcept
		{
			IStream::Release();
			Stream = nullptr;
		}

		~TStream()
		{
			if (Stream)
			{
				Commit();
				Release();
			}
		}

	private:
		ISendBuffer* Stream = nullptr;
	};

	//SendBuffer::ToStream
	template <uint32_t Size>
	inline TStream SendBuffer<Size>::ToStream(bool CurrentPositionAsBase) noexcept
	{
		return TStream(reinterpret_cast<ISendBuffer*>(this), CurrentPositionAsBase);
	}

	/*------------------------------------------------------------
		TSendBuffer (Merge of TPtr specialization and GlobalObjectStore for SendBuffer)(Source of all SendBuffer<> instances)
	  ------------------------------------------------------------*/
	using TSendBufferPtrBase = MemoryResourcePtrBase<ISendBuffer>;

#define TSEND_BUFFER_DESTROY_CALLBACK(Pool)														\
	NewBuffer->Destroy = [](ptr_t Object, bool bCallDestructor) {								\
		if (bCallDestructor)																	\
		{																						\
			reinterpret_cast<CONCAT(Pool, Buffer) *>(Object)->~CONCAT(Pool, Buffer)();			\
		}																						\
		Pool::Deallocate(reinterpret_cast<CONCAT(Pool, Buffer) *>(Object));						\
	};

	class TSendBuffer : public _TPtr<ISendBuffer, TSendBufferPtrBase>
	{
	public:

		using MyType = TSendBuffer;
		using Base = _TPtr<ISendBuffer, TSendBufferPtrBase>;

		TSendBuffer() : Base() {}
		TSendBuffer(ISendBuffer* Buffer) : Base(Buffer) {}

		static const size_t DefaultBufferSize = CSize4ServerSendBufferSize;
		static const size_t MaxBufferSize = CSize5ServerSendBufferSize;

		/*------------------------------------------------------------
			All send buffer sizes as types
		 ------------------------------------------------------------*/
		using Size1Buffer = SendBuffer<CSize1ServerSendBufferSize>;
		using Size2Buffer = SendBuffer<CSize2ServerSendBufferSize>;
		using Size3Buffer = SendBuffer<CSize3ServerSendBufferSize>;
		using Size4Buffer = SendBuffer<CSize4ServerSendBufferSize>;
		using Size5Buffer = SendBuffer<CSize5ServerSendBufferSize>;

		using Size1 = TObjectPool<Size1Buffer, CSize1ServerSendBufferCount>;
		using Size2 = TObjectPool<Size2Buffer, CSize2ServerSendBufferCount>;
		using Size3 = TObjectPool<Size3Buffer, CSize3ServerSendBufferCount>;
		using Size4 = TObjectPool<Size4Buffer, CSize4ServerSendBufferCount>;
		using Size5 = TObjectPool<Size5Buffer, CSize5ServerSendBufferCount>;

		static RStatus Initialize() noexcept
		{
			RTRY_L(Size1::Preallocate(), "TSendBuffer::Initialize() Failed to Size1::Preallocate()") {}
			RTRY_L(Size2::Preallocate(), "TSendBuffer::Initialize() Failed to Size2::Preallocate()") {}
			RTRY_L(Size3::Preallocate(), "TSendBuffer::Initialize() Failed to Size3::Preallocate()") {}
			RTRY_L(Size4::Preallocate(), "TSendBuffer::Initialize() Failed to Size4::Preallocate()") {}
			RTRY_L(Size5::Preallocate(), "TSendBuffer::Initialize() Failed to Size5::Preallocate()") {}

			return RSuccess;
		}

		template <size_t BufferSize = DefaultBufferSize>
		static inline TSendBuffer New() noexcept
		{
			ISendBuffer* NewBuffer{ nullptr };

			if constexpr (BufferSize <= CSize1ServerSendBufferSize)
			{
				NewBuffer = reinterpret_cast<ISendBuffer*>(Size1::NewRaw());
				TSEND_BUFFER_DESTROY_CALLBACK(Size1)
			}
			else if constexpr (BufferSize <= CSize2ServerSendBufferSize)
			{
				NewBuffer = reinterpret_cast<ISendBuffer*>(Size2::NewRaw());
				TSEND_BUFFER_DESTROY_CALLBACK(Size2)
			}
			else if constexpr (BufferSize <= CSize3ServerSendBufferSize)
			{
				NewBuffer = reinterpret_cast<ISendBuffer*>(Size3::NewRaw());
				TSEND_BUFFER_DESTROY_CALLBACK(Size3)
			}
			else if constexpr (BufferSize <= CSize4ServerSendBufferSize)
			{
				NewBuffer = reinterpret_cast<ISendBuffer*>(Size4::NewRaw());
				TSEND_BUFFER_DESTROY_CALLBACK(Size4)
			}
			else if constexpr (BufferSize <= CSize5ServerSendBufferSize)
			{
				NewBuffer = reinterpret_cast<ISendBuffer*>(Size5::NewRaw());
				TSEND_BUFFER_DESTROY_CALLBACK(Size5)
			}
			else
			{
				static_assert(false, "Buffer size exceeded the MaxBufferSize");
			}

			return NewBuffer;
		}

		//Runtime known size
		//	**Prefere compile time Alloc<Size> if the size is known at compile time
		static inline TSendBuffer New(const uint32_t BufferSize) noexcept
		{
			ISendBuffer* NewBuffer{ nullptr };

			if (BufferSize <= CSize1ServerSendBufferSize)
			{
				NewBuffer = reinterpret_cast<ISendBuffer*>(Size1::NewRaw());
				TSEND_BUFFER_DESTROY_CALLBACK(Size1)
			}
			else if (BufferSize <= CSize2ServerSendBufferSize)
			{
				NewBuffer = reinterpret_cast<ISendBuffer*>(Size2::NewRaw());
				TSEND_BUFFER_DESTROY_CALLBACK(Size2)
			}
			else if (BufferSize <= CSize3ServerSendBufferSize)
			{
				NewBuffer = reinterpret_cast<ISendBuffer*>(Size3::NewRaw());
				TSEND_BUFFER_DESTROY_CALLBACK(Size3)
			}
			else if (BufferSize <= CSize4ServerSendBufferSize)
			{
				NewBuffer = reinterpret_cast<ISendBuffer*>(Size4::NewRaw());
				TSEND_BUFFER_DESTROY_CALLBACK(Size4)
			}
			else if (BufferSize <= CSize5ServerSendBufferSize)
			{
				NewBuffer = reinterpret_cast<ISendBuffer*>(Size5::NewRaw());
				TSEND_BUFFER_DESTROY_CALLBACK(Size5)
			}
			else
			{
				LogFatal("Failed to allocate memory, needed %d,  Max %d", BufferSize, CSize5ServerSendBufferSize);
				return (ISendBuffer*)nullptr;
			}

			return NewBuffer;
		}

		//Send packet to connection through the Arbiter
		template <size_t BufferSize = DefaultBufferSize>
		static inline TSendBuffer NewPacket(EntityId ConnectionId) noexcept
		{
			TSendBuffer Buffer = New<BufferSize>();
			if (!Buffer)
			{
				return nullptr;
			}

			Buffer->Write((TPacketSize)0);							//[ 2]Size	placeholder
			Buffer->Write((TPacketOpcode)Opcode_ROUTED_PACKET);		//[ 2]C_W_ROUTE_PACKET opcode
			Buffer->Write(ConnectionId);							//[ 8]EntityId (connectionId)

			return std::move(Buffer);
		}

		//Relay packet from client/thirdparty to world server
		template <size_t BufferSize = DefaultBufferSize>
		static inline TSendBuffer AllocRelayPacket(EntityId ConnectionId) noexcept
		{
			TSendBuffer Buffer = New<BufferSize>();

			Buffer->Write((TPacketSize)0);							//[ 2]Size	placeholder
			Buffer->Write((TPacketOpcode)Opcode_ROUTED_PACKET);		//[ 2]S_W_PACKET opcode
			Buffer->Write(ConnectionId);							//[ 8]EntityId (connectionId)

			return Buffer;
		}

		template <size_t BufferSize = DefaultBufferSize>
		static inline TSendBuffer AllocBroadcastPacket(EBroadcastType Type = EBroadcastType::Visible) noexcept
		{
			TSendBuffer Buffer = New<BufferSize>();

			WriteBoradcastPacketHeader(Buffer.Get(), Type);

			return Buffer;
		}

#ifdef COMMONEX_BUFFERS_STATISTICS
		static void PrintStatistics()
		{
			LogInfo("TSendBuffer Pools ###############################################################\n");
			LogInfo("\n\tSize1(BlockSize:{} BlockCount:{}):\n\t\tAllocations:{}\n\t\tDeallocations:{}\n\t\tOSAllocations:{}\n\t\tOSDeallocations:{}",
				CSize1ServerSendBufferSize,
				CSize1ServerSendBufferCount,
				Size1::GetTotalAllocations(),
				Size1::GetTotalDeallocations(),
				Size1::GetTotalOSAllocations(),
				Size1::GetTotalOSDeallocations());
			LogInfo("\n\tSize2(BlockSize:{} BlockCount:{}):\n\t\tAllocations:{}\n\t\tDeallocations:{}\n\t\tOSAllocations:{}\n\t\tOSDeallocations:{}",
				CSize2ServerSendBufferSize,
				CSize2ServerSendBufferCount,
				Size2::GetTotalAllocations(),
				Size2::GetTotalDeallocations(),
				Size2::GetTotalOSAllocations(),
				Size2::GetTotalOSDeallocations());
			LogInfo("\n\tSize3(BlockSize:{} BlockCount:{}):\n\t\tAllocations:{}\n\t\tDeallocations:{}\n\t\tOSAllocations:{}\n\t\tOSDeallocations:{}",
				CSize3ServerSendBufferSize,
				CSize3ServerSendBufferCount,
				Size3::GetTotalAllocations(),
				Size3::GetTotalDeallocations(),
				Size3::GetTotalOSAllocations(),
				Size3::GetTotalOSDeallocations());
			LogInfo("\n\tSize4(BlockSize:{} BlockCount:{}):\n\t\tAllocations:{}\n\t\tDeallocations:{}\n\t\tOSAllocations:{}\n\t\tOSDeallocations:{}",
				CSize4ServerSendBufferSize,
				CSize4ServerSendBufferCount,
				Size4::GetTotalAllocations(),
				Size4::GetTotalDeallocations(),
				Size4::GetTotalOSAllocations(),
				Size4::GetTotalOSDeallocations());
			LogInfo("\n\tSize5(BlockSize:{} BlockCount:{}):\n\t\tAllocations:{}\n\t\tDeallocations:{}\n\t\tOSAllocations:{}\n\t\tOSDeallocations:{}",
				CSize5ServerSendBufferSize,
				CSize5ServerSendBufferCount,
				Size5::GetTotalAllocations(),
				Size5::GetTotalDeallocations(),
				Size5::GetTotalOSAllocations(),
				Size5::GetTotalOSDeallocations());
			LogInfo("\n\tTotal Allocation:{}\n\tTotal Deallocations:{}\n\tTotal OSAllocations:{}\n\tTotal OSDeallocations:{}",
				Size1::GetTotalAllocations() + Size2::GetTotalAllocations() + Size3::GetTotalAllocations() + Size4::GetTotalAllocations() + Size5::GetTotalAllocations(),
				Size1::GetTotalDeallocations() + Size2::GetTotalDeallocations() + Size3::GetTotalDeallocations() + Size4::GetTotalDeallocations() + Size5::GetTotalDeallocations(),
				Size1::GetTotalOSAllocations() + Size2::GetTotalOSAllocations() + Size3::GetTotalOSAllocations() + Size4::GetTotalOSAllocations() + Size5::GetTotalOSAllocations(),
				Size1::GetTotalOSDeallocations() + Size2::GetTotalOSDeallocations() + Size3::GetTotalOSDeallocations() + Size4::GetTotalOSDeallocations() + Size5::GetTotalOSDeallocations());
			LogInfo("TSendBuffer Pools ###############################################################\n");
		}
#endif

	private:
		inline static void WriteBoradcastPacketHeader(ISendBuffer* Buffer, EBroadcastType Type)
		{
			Buffer->Write((TPacketSize)0); //Size placeholder
			Buffer->Write(Opcode_BROADCAST_PACKET);
			Buffer->Write(Type);
		}
	};

	/*------------------------------------------------------------
		TRecvBuffer (Merge of TPtr specialization and GlobalObjectStore for RecvBuffer)
	  ------------------------------------------------------------*/
	using TRecvBufferPtrBase = MemoryResourcePtrBase<RecvBuffer>;

	class TRecvBuffer : public _TPtr<RecvBuffer, TRecvBufferPtrBase>
	{
	public:
		using MyType = TRecvBuffer;
		using Base = _TPtr<RecvBuffer, TRecvBufferPtrBase>;

		TRecvBuffer() : Base() {}
		TRecvBuffer(RecvBuffer* Buffer) : Base(Buffer) {}

		/*------------------------------------------------------------
			Object Pool
		 ------------------------------------------------------------*/
		using Pool = TObjectPool<RecvBuffer, CRecvBufferCount>;

		static RStatus Initialize() noexcept
		{
			RTRY_L(Pool::Preallocate(), "TRecvBuffer::Initialize() Failed to Pool::Preallocate()") {}

			return RSuccess;
		}

		//Allocate
		static inline TRecvBuffer New() noexcept
		{
			RecvBuffer* NewBuffer = Pool::NewRaw();
			if (!NewBuffer) {
				return nullptr;
			}

			NewBuffer->Destroy = [](ptr_t Object, bool bCallDestructor) {
				Pool::Deallocate((RecvBuffer*)Object);
			};

			return NewBuffer;
		}

#ifdef COMMONEX_BUFFERS_STATISTICS
		static void PrintStatistics()
		{
			LogInfo("TRecvBuffer Pool ###############################################################\n");
			LogInfo("\n\tPool(Count:{}):\n\t\tAllocations:{}\n\t\tDeallocations:{}\n\t\tOSAllocations:{}\n\t\tOSDeallocations:{}",
				CRecvBufferCount,
				Pool::GetTotalAllocations(),
				Pool::GetTotalDeallocations(),
				Pool::GetTotalOSAllocations(),
				Pool::GetTotalOSDeallocations());
			LogInfo("TRecvBuffer Pool ###############################################################\n");
		}
#endif
	};
}