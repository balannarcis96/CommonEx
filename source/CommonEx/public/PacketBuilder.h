#pragma once
/**
 * @file PacketBuilder.h
 *
 * @brief PacketBuilder abstractions
 *			Allows for efficient and elegant packet construction
 *			There are 4 types of packet context:
 *				-PacketContext				base of all contexts for packets of unknown compile time size 
 *				-FixedBodyPacketContext		context for packets where the size is known at compile time
 *				-RoutedPacketContext		context for packets that need to be routed
 *				-ListPacketWriteContext		context (helper) for efficient and elegant building of stream packets
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

namespace CommonEx {

	using PacketContextFlags = uint64_t;
	using TPacketCount = uint16_t;

	enum EPacketContextFlags : PacketContextFlags {
		EPacketContextFlags_FixedLength = 1 << 0,
		EPacketContextFlags_WriteHeader = 1 << 1,
		EPacketContextFlags_List = 1 << 2,
		EPacketContextFlags_Routed = 1 << 3,
	};

	constexpr PacketContextFlags CDefaultPacketContextFlags = EPacketContextFlags_WriteHeader;

	constexpr auto CPacketContextWriteIgnore = 0;
	constexpr auto CPacketContextFirstItem = 1;
	constexpr auto CPacketContextLastItem = 1;

	consteval bool PacketContextHasFlag(const PacketContextFlags Flags, const PacketContextFlags TestFlags) {
		return (Flags & TestFlags) == TestFlags;
	}

	/*------------------------------------------------------------
		Packet context base class
	  ------------------------------------------------------------*/
	template
		<
		PTR		typename Super,
		PTR		typename PacketData,
		PTR		size_t _PacketBaseSize,
		PTR		PacketContextFlags _Flags = CDefaultPacketContextFlags,
		PTR		TPacketOpcode _Opcode = PacketData::Opcode
		>
		class PacketContext {
		public:
#pragma region Type traits

			using PacketContextType = PacketContext<Super, PacketData, _PacketBaseSize, _Flags, _Opcode>;
			using PacketDataType = PacketData;

			static const size_t	PacketBaseSize = _PacketBaseSize;
			static const PacketContextFlags	Flags = _Flags;
			static const TPacketOpcode Opcode = _Opcode;

#pragma endregion

			static_assert(std::is_base_of_v<PacketBaseTag, PacketData>, "ProtocolEx:PacketContext<> PacketData must be a derived class of [InternalPacketBodyBase] or [PacketDataBase]");

			EntityId	TargetConnectionId = EntityId::None;
			PacketData	Data;

			FORCEINLINE PacketData* operator->()noexcept {
				return &Data;
			}

			FORCEINLINE size_t GetPacketSize() const noexcept {
				return ((Super*)this)->GetPacketSize();
			}

			FORCEINLINE RStatus BuildPacket(TStream& Stream) const noexcept {
				if constexpr (PacketContextHasFlag(Flags, EPacketContextFlags_WriteHeader)) {
					WritePacketHead(Stream);
				}

				return ((Super*)this)->BuildPacket(Stream);
			}

			FORCEINLINE RStatus BuildPacketOnly(TStream& Stream) const noexcept {
				return ((Super*)this)->BuildPacket(Stream);
			}

		protected:
			FORCEINLINE void WritePacketHead(TStream& Stream) const noexcept {
				Stream.Write((TPacketSize)0); //Size placeholder
				Stream.Write((TPacketOpcode)Opcode); //Opcode
			}
	};

	/*------------------------------------------------------------
		Fixed size packet context (body)
	  ------------------------------------------------------------*/
	template<typename PacketData, TPacketOpcode _Opcode = PacketData::Opcode, PacketContextFlags _Flags = CDefaultPacketContextFlags>
	class FixedBodyPacketContext : public PacketContext
		PTR								<
		PTR									FixedBodyPacketContext<PacketData>,
		PTR									PacketData,
		PTR									sizeof(PacketData) + CPacketHeaderSize,
		PTR									_Flags | EPacketContextFlags_FixedLength,
		PTR									_Opcode
		PTR								>
	{
	public:
		using MyBaseType = PacketContext
			PTR								<
			PTR									FixedBodyPacketContext<PacketData>,
			PTR									PacketData,
			PTR									sizeof(PacketData) + CPacketHeaderSize,
			PTR									_Flags | EPacketContextFlags_FixedLength,
			PTR									_Opcode
			PTR								>;

		FORCEINLINE size_t GetPacketSize() const noexcept {
			return sizeof(PacketData) + CPacketHeaderSize;
		}

		inline RStatus BuildPacket(TStream& Stream) const noexcept {
			if constexpr (!PacketContextHasFlag(_Flags, EPacketContextFlags_WriteHeader)) {
				MyBaseType::WritePacketHead(Stream);
			}

			if constexpr (PacketData::HasWriteMethod) {
				this->Data.Write(Stream);
			}

			//packet body
			Stream.Write((const uint8_t*)&this->Data, sizeof(PacketData));

			return RSuccess;
		}
	};

	/*------------------------------------------------------------
		Packet context class for routed packets
	  ------------------------------------------------------------*/
	template<typename Super, typename PacketData, size_t _PacketBaseSize, PacketContextFlags _Flags = CDefaultPacketContextFlags, TPacketOpcode _Opcode = PacketData::Opcode>
	class RoutedPacketContext : public PacketContext<Super, PacketData, _PacketBaseSize, (_Flags | EPacketContextFlags_Routed), _Opcode> {};

	/*------------------------------------------------------------
		Fixed size packet context class for routed packets
	  ------------------------------------------------------------*/
	template<typename PacketData, TPacketOpcode _Opcode = PacketData::Opcode, PacketContextFlags _Flags = CDefaultPacketContextFlags>
	class RoutedFixedBodyPacketContext : public FixedBodyPacketContext<PacketData, _Opcode, (_Flags | EPacketContextFlags_Routed)> {};

	/*------------------------------------------------------------
		List Packet Write Context
			Used for smart listed packets continous building
	  ------------------------------------------------------------*/
	template<typename FixedSizePacketType>
	class ListPacketWriteContext {
	public:
		int32_t						ToWriteCount{ 0 };
		int32_t						Written{ 0 };
		uint32_t					Purpose{ 0 };
		TSendBuffer					Buffer{ nullptr };

		FixedSizePacketType			Packet;

		ListPacketWriteContext(int32_t Count) : ToWriteCount(Count), Buffer(nullptr) {}

		//Cant copy
		ListPacketWriteContext(const ListPacketWriteContext&) = delete;
		ListPacketWriteContext& operator=(const ListPacketWriteContext&) = delete;

		inline auto* operator->() noexcept {
			return &Packet.Data;
		}

		RStatus Build() noexcept;

	private:
		inline int32_t GetWritten() const noexcept {
			return Written;
		}

		//Increments the written count and returns true if reached the end
		inline bool OnWrite() noexcept {
			return (++Written) == ToWriteCount;
		}

		inline int32_t Rebase() noexcept {
			int32_t Temp = Written;
			ToWriteCount -= Written;
			Written = 0;

			return Temp;
		}
	};

	/*------------------------------------------------------------
		Packet builder
	  ------------------------------------------------------------*/
	class PacketBuilder {
	public:
		template<typename IModel, PacketContextFlags Flags, TPacketOpcode Opcode>
		inline static TSendBuffer BuildBroadcastPacket(FixedBodyPacketContext<IModel, Flags, Opcode>& Data, EBroadcastType BrodacastType, bool CommitPacket = true)  noexcept
		{
			static const size_t PacketSize = FixedBodyPacketContext<IModel, Flags, Opcode>::PacketBaseSize;

			TSendBuffer Buffer = TSendBuffer::AllocBroadcastPacket(BrodacastType);
			if (!Buffer.Get()) {
				return nullptr;
			}

			TStream Stream = Buffer->ToStream();

			RTRY_S_L(Data.BuildPacket(Stream), {}, "PacketBuilder::BuildBroadcastPacket(FixedBodyPacketContext) Failed to BuildPacket()") {}

			if (CommitPacket) {
				Stream.CommitPacket();
			}

			return Buffer;
		};

		template<typename Supre, typename IModel, size_t PacketBaseSize, PacketContextFlags Flags, TPacketOpcode Opcode>
		inline static TSendBuffer BuildBroadcastPacket(PacketContext<Supre, IModel, PacketBaseSize, Flags, Opcode>& Data, EBroadcastType BrodacastType, bool CommitPacket = true)  noexcept
		{
			TSendBuffer Buffer = TSendBuffer::AllocBroadcastPacket(BrodacastType);
			if (!Buffer.Get()) {
				return nullptr;
			}

			TStream Stream = Buffer->ToStream();

			RTRY_S_L(Data.BuildPacket(Stream), {}, "PacketBuilder::BuildBroadcastPacket(PacketContext) Failed to BuildPacket()") {}

			if (CommitPacket) {
				Stream.CommitPacket();
			}

			return Buffer;
		};

		template<typename IModel, PacketContextFlags Flags, TPacketOpcode Opcode>
		inline static TSendBuffer BuildPacket(FixedBodyPacketContext<IModel, Flags, Opcode>& Data, bool CommitPacket = true)  noexcept
		{
			static const size_t PacketSize = FixedBodyPacketContext<IModel, Flags, Opcode>::PacketBaseSize;

			TSendBuffer Buffer = TSendBuffer::New<PacketSize>();
			if (!Buffer.Get()) {
				return {};
			}

			TStream Stream = Buffer->ToStream();

			RTRY_S_L(BuildPacket(Stream, Data, CommitPacket), {}, "PacketBuilder::BuildPacket(FixedBodyPacketContext) Failed to BuildPacket()") {}

			return Buffer;
		};

		template<typename Supre, typename IModel, size_t PacketBaseSize, PacketContextFlags Flags, TPacketOpcode Opcode>
		inline static TSendBuffer BuildPacket(PacketContext<Supre, IModel, PacketBaseSize, Flags, Opcode>& Data, bool CommitPacket = true)  noexcept
		{
			TSendBuffer Buffer{  };

			if constexpr (PacketContextHasFlag(Flags, EPacketContextFlags_FixedLength)) {
				Buffer = TSendBuffer::New<PacketBaseSize>();
				if (!Buffer.Get()) {
					return nullptr;
				}
			}
			else {
				const size_t PacketSize = Data.GetPacketSize();

				Buffer = TSendBuffer::New((uint32_t)PacketSize);
				if (!Buffer.Get()) {
					return nullptr;
				}
			}

			TStream Stream = Buffer->ToStream();

			RTRY_S_L(BuildPacket(Stream, Data, CommitPacket), {}, "PacketBuilder::BuildPacket(PacketContext) Failed to BuildPacket()") {}

			return Buffer;
		};

		template<typename Supre, typename IModel, size_t PacketBaseSize, PacketContextFlags Flags, TPacketOpcode Opcode>
		inline static RStatus BuildPacket(TSendBuffer& Buffer, PacketContext<Supre, IModel, PacketBaseSize, Flags, Opcode>& Data, bool CommitPacket = true)  noexcept
		{
			TStream Stream = Buffer->ToStream();

			return BuildPacket(Stream, Data, CommitPacket);
		};

		template<typename Supre, typename IModel, size_t PacketBaseSize, PacketContextFlags Flags, TPacketOpcode Opcode>
		inline static RStatus BuildPacket(TStream& Stream, PacketContext<Supre, IModel, PacketBaseSize, Flags, Opcode>& Data, bool CommitPacket = true)  noexcept
		{
			if constexpr (PacketContextHasFlag(Flags, EPacketContextFlags_Routed)) {
				Stream.Write((TPacketSize)0);							//[ 2]Size	placeholder		
				Stream.Write(Opcode_ROUTED_PACKET);						//[ 2]Opcode_ROUTED_PACKET opcode
				Stream.Write(Data.TargetConnectionId);					//[ 8]EntityId (connectionId)

				Stream.Commit(true);
			}

			R_TRY_L(Data.BuildPacket(Stream), "PacketBuilder::BuildPacket<> Failed!") {}

			if (CommitPacket) {
				Stream.CommitPacket();
			}

			return RSuccess;
		};
	};

	template<typename FixedSizePacketType>
	RStatus ListPacketWriteContext<FixedSizePacketType>::Build() noexcept {

		if (!Buffer.Get()) {
			Buffer = TSendBuffer::New();
			if (!Buffer) {
				return RFail;
			}

			TStream Stream = Buffer->ToStream();

			//Write head
			Stream.Write((TPacketSize)0);//Size Placeholder
			Stream.Write(Opcode_STREAM_PACKET);

			//Stream(List) packet header ####
			Stream.Write<TPacketCount>(0); //Count Placeholder
			Stream.Write((TPacketSize)(Packet.GetPacketSize() - CPacketHeaderSize)); //Actual size without the header
			Stream.Write((TPacketOpcode)FixedSizePacketType::Opcode); //Actual packet opcode
			Stream.Write(Purpose);
			//###############################(DONT TOUCH)

			Stream.Commit();
		}

		TStream Stream = Buffer->ToStream();

		if (!Stream.CanFit(Packet.GetPacketSize())) {
			//Write the written count
			Buffer->WriteCount(Rebase(), CPacketHeaderSize);

			return RWorkRemains;
		}

		R_TRY_L(Packet.BuildPacketOnly(Stream), "ListPacketWriteContext::Build() Failed to Packet.BuildPacketOnly(Stream)") {}

		Stream.Commit();

		if (OnWrite()) {
			//Write the written count
			Buffer->WriteCount(Rebase(), CPacketHeaderSize);

			return RWorkRemains;
		}

		return RSuccess;
	}

#define ROUTED_CONTEXT_CTOR(Name)							\
		public:												\
		Name(EntityId TargetConnectionId)					\
			{ this->TargetConnectionId = TargetConnectionId ; }
}