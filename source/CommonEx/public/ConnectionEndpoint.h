#pragma once

namespace CommonEx {
#pragma once
	
	//template<typename TSuper, typename TSession>
	//struct ConnectionEndpoint {
	//	//Connection id.
	//	EntityId						Id{ 0 };

	//	//Connection flags.
	//	union {
	//		struct {
	//			unsigned				bReady : 1;
	//			unsigned				bIsReceivingHead : 1;
	//		};
	//		uint32_t					ConnectionFlags{ 0 };
	//	};

	//	//Connection receive init state.
	//	uint32_t						IOState{ 0 };

	//	//Connection tcp endpoint.
	//	TCPEndpoint						NetEndpoint{};

	//	//Connection session.
	//	TSession						Session{};

	//	RStatus SetConnection(TSocket Socket, const sockaddr_in* Info, CurrentAsyncIOSystem* AsyncIOInterface) noexcept {
	//		R_TRY_L(NetEndpoint.Initialize(Socket, Info), "RefBasedConnectionEndpoint::SetConnection() Failed to Initialize the net interface with Socket") {}
	//		R_TRY_L(NetEndpoint.AssoctiateToAsyncIO(AsyncIOInterface), "RefBasedConnectionEndpoint::SetConnection() Failed to associate socket with the AsyncIOInterface") {}

	//		return RSuccess;
	//	}

	//	//Attempt to open a connection
	//	RStatus Connect(ulong_t IpV4Address, uint16_t Port, CurrentAsyncIOSystem* AsyncIOInterface) noexcept {
	//		RTRY_L_FMT(NetEndpoint.Initialize(Port, IpV4Address), "RefBasedConnectionEndpoint::Connect() Failed to Initialize the net interface with Ip{} Port{}", IpV4Address, Port) {}
	//		R_TRY_L(NetEndpoint.AssoctiateToAsyncIO(AsyncIOInterface), "RefBasedConnectionEndpoint::Connect() Failed to associate socket with the AsyncIOInterface") {}

	//		RStatus Result = RSuccess;
	//		size_t ConnectStartTime = time(NULL);
	//		while (1)
	//		{
	//			Result = NetEndpoint.Connect();
	//			if (Result != RSuccess)
	//			{
	//				if ((int32_t)Result == WSAETIMEDOUT)
	//				{
	//					if (((size_t)time(NULL) - ConnectStartTime) >= ((size_t)2 * 1000i64))
	//					{
	//						LogInfo("Connect attempt timedout. Retrying...");
	//					}
	//				}
	//				else {
	//					LogFatal("Connection attempt failed with error[{}]", Result);
	//					break;
	//				}
	//			}
	//			else
	//			{
	//				LogInfo("Successfully connected!");
	//				break;
	//			}

	//			std::this_thread::sleep_for(std::chrono::milliseconds(500));
	//		}

	//		if (Result != RSuccess) {
	//			return Result;
	//		}

	//		return RSuccess;
	//	}

	//	inline RStatus Close() noexcept {
	//		//this will cause any out-standing recv/send operations to fail on this socket
	//		return NetEndpoint.Close();
	//	}

	//	inline RStatus ReceiveAsync(TRecvBuffer& Buffer) noexcept {
	//		RStatus RecvResult = ReceiveAsync(Buffer.Get());
	//		if (RecvResult != RSuccess) {
	//			R_SET_LAST_ERROR_FMT("ConnectionEndpoint::Receive(TRecvBuffer) Failed!", Id.Id);
	//		}
	//		else {
	//			Buffer.Release();
	//		}

	//		return RecvResult;
	//	}

	//	inline RStatus ReceiveAsync(CommonEx::RecvBuffer* Buffer) noexcept {
	//		Buffer->GetPayload()->Connection = Id;

	//		if (!Buffer->GetPayload()->ConnectionPtr) {
	//			Buffer->GetPayload()->ConnectionPtr = this;
	//		}

	//		((TSuper*)this)->IncRef(); //add ref for this async operation

	//		RStatus RecvResult = NetEndpoint.ReceiveAsync(&Buffer->GetInterface(), (OsOverlappedType*)Buffer);
	//		if (RecvResult != RSuccess) {
	//			((TSuper*)this)->DecRef(); //dec ref for this async operation

	//			R_SET_LAST_ERROR_FMT("ConnectionEndpoint::Receive(RecvBuffer*) Failed!", Id.Id);
	//		}

	//		return RecvResult;
	//	}

	//	//Sends buffer to connection.
	//	//Invariant: If the operation succeeded, the CompletionHandler is responsible for the lifetime of the SendBuffer
	//	//Invariant: If the completion handler is not set, then the ref for the operation was not added(no call to IncRef()) [for ref based connections]
	//	template<bool bInSession = true>
	//	inline RStatus SendAsync(TSendBuffer& Buffer) noexcept {
	//		Buffer->Prepare();
	//		Buffer->GetPayload()->Connection = Id;

	//		if (!Buffer->GetPayload()->ConnectionPtr) {
	//			Buffer->GetPayload()->ConnectionPtr = this;
	//		}

	//		if constexpr (bInSession && !TSession::IsPassthrough) {
	//			Session.Encrypt(Buffer->GetBuffer(), Buffer->GetSize());
	//		}

	//		if (!Buffer->bHasCompletionHandler) {
	//			return ((TSuper*)this)->SendWithGenericCompletionHandlerAsync(Buffer);
	//		}

	//		((TSuper*)this)->IncRef(); //add ref for this async operation

	//		RStatus SendResult = NetEndpoint.SendAsync(&Buffer->GetInterface(), (OsOverlappedType*)Buffer.Get());
	//		if (SendResult != RSuccess) {
	//			((TSuper*)this)->DecRef(); //dec ref for this async operation
	//			R_SET_LAST_ERROR_FMT("ConnectionEndpoint::Send(TSendBuffer) Failed!", Id.Id);
	//		}
	//		else {
	//			Buffer.Release();
	//		}

	//		return SendResult;
	//	}

	//	template<bool bInSession = true>
	//	inline RStatus SendAsync(TSendBuffer&& Buffer) noexcept {
	//		RStatus Result = SendAsync<bInSession>(Buffer);

	//		Buffer.Reset();

	//		return Result;
	//	}

	//	//Sends packet to connection.
	//	//Invariant: If the operation succeeded, the CompletionHandler is responsible for the lifetime of the SendBuffer
	//	template<typename T, PacketContextFlags Flags, TPacketOpcode Opcode>
	//	inline RStatus SendAsync(FixedBodyPacketContext<T, Opcode, Flags>& Packet) noexcept {
	//		return SendAsync<true>(std::move(PacketBuilder::BuildPacket(Packet)));
	//	}

	//	//Sends packet to connection.
	//	//Invariant: If the operation succeeded, the CompletionHandler is responsible for the lifetime of the SendBuffer
	//	template<typename Super, typename T, size_t PacketBaseSize, PacketContextFlags Flags, TPacketOpcode Opcode>
	//	inline RStatus SendAsync(PacketContext<Super, T, PacketBaseSize, Flags, Opcode>& Packet) noexcept {
	//		return SendAsync<true>(std::move(PacketBuilder::BuildPacket(Packet)));
	//	}

	//	//inline RStatus SendWithGenericCompletionHandlerAsync(TSendBuffer& Buffer)noexcept;
	//};

//	template<typename TSession>
//	struct SharedConnectionEndpoint : MemoryResource<true>, ConnectionEndpoint<SharedConnectionEndpoint, TSession> {
//		using Base = ConnectionEndpoint<SharedConnectionEndpoint, TSession>;
//
//		RefBasedConnectionEndpoint() : Base() {
//			ConnectionFlags = 0;
//			IsShuttingdown.clear();
//		}
//
//		virtual ~RefBasedConnectionEndpoint() {
//			LogInfo("~RefBasedConnectionEndpoint()");
//		}
//
//		std::atomic_flag			IsShuttingdown{ };
//
//		PacketHandlerDelegate		OnPacketHandler;
//
//		inline RStatus SendWithGenericCompletionHandlerAsync(TSendBuffer& Buffer)noexcept;
//
//		//Start communication
//		RStatus StartCommunication() noexcept;
//
//		//Receive packet head
//		RStatus ReceiveHeadAsync() noexcept;
//
//		//Receive packet body
//		RStatus ReceiveBodyAsync(TRecvBuffer& RecvBuffer) noexcept;
//	};
//
//	struct RefBasedConnectionGuard {
//		RefBasedConnectionEndpoint* PTR		Connection;
//		RStatus								Result{ RSuccess };
//
//		RefBasedConnectionGuard() noexcept : Connection(nullptr) {  }
//		RefBasedConnectionGuard(RefBasedConnectionEndpoint* Connection) noexcept : Connection(Connection) { Connection->IncRef(); }
//		RefBasedConnectionGuard(RefBasedConnectionEndpoint* Connection, bool bExistingRef) noexcept : Connection(Connection) {}
//		~RefBasedConnectionGuard()noexcept {
//			if (Connection) {
//				if (Result != RSuccess) {
//					//If we set the IsShuttingdown to true, 
//					if (!Connection->IsShuttingdown.test_and_set(std::memory_order_acq_rel)) {
//						//Close the connection
//						Connection->Close();
//
//						//Remove the initial ref, so when all outstanding refs are release the connection will be destroyed
//						Connection->DecRef();
//					}
//
//					//Remove ref for this scope
//					Connection->DecRef();
//				}
//				else {
//					Connection->DecRef();
//				}
//			}
//		}
//
//		inline bool TryAcquire(RefBasedConnectionEndpoint* Connection)noexcept {
//			if (Connection->IsShuttingdown.test(std::memory_order_acquire)) {
//				return false;
//			}
//
//			this->Connection = Connection;
//
//			return true;
//		}
//		inline bool IsValid() const noexcept {
//			if (!Connection) {
//				return false;
//			}
//
//			if (Connection->IsShuttingdown.test(std::memory_order_acquire)) {
//				return false;
//			}
//
//			return true;
//		}
//		inline void Release() noexcept {
//			Connection = nullptr;
//		}
//	};
//
//	//Acquire connection for existing ref
//#define TRY_ACQUIRE_CONNECTION(Conn)			\
//		RefBasedConnectionGuard Guard;			\
//		if(!Guard.TryAcquire(Conn))
//
//	//Add ref to existing connection and acquire iff IsValid()
//#define GUARD_CONNECTION(Conn)					\
//		RefBasedConnectionGuard Guard(Conn);	\
//		if(!Guard.IsValid())
}