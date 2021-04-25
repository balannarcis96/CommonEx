#pragma once
/**
 * @file TransportLayer.h
 *
 * @brief Transport layer abstraction
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

 //@TODO UDP abstraction

namespace CommonEx {
	enum class ENetNameType : uint16_t {
		None,
		DataGram
	};

	struct NetName {
		//Network name type.
		ENetNameType		Type{ ENetNameType::None };

		//Network name, IP and PORT.
		sockaddr_in			Name;

		//Name size.
		int32_t				NameLen{ 0 };

		NetName() {
			NameLen = sizeof(Name);
			memset(&Name, 0, sizeof(Name));
		}
		NetName(uint16_t Port, TAddressFamily AdFamily = AF_INET) {
			NameLen = sizeof(Name);
			memset(&Name, 0, sizeof(Name));

			Name.sin_port = htons(Port);
			Name.sin_family = AdFamily;
			Name.sin_addr.s_addr = INADDR_ANY;
		}
		NetName(uint16_t Port, ulong_t Addr, TAddressFamily AdFamily = AF_INET) {
			NameLen = sizeof(Name);
			memset(&Name, 0, sizeof(Name));

			Name.sin_port = htons(Port);
			Name.sin_family = AdFamily;
			Name.sin_addr.s_addr = Addr;
		}
	};

	struct NetEndpoint : NetName {
		//Endpoint socket.
		TSocket				Socket;

		NetEndpoint() : NetName() {
			Socket = INVALID_SOCKET;
		}
		NetEndpoint(uint16_t Port, TAddressFamily AdFamily = AF_INET) : NetName(Port, AdFamily) {
			Socket = INVALID_SOCKET;
		}
		NetEndpoint(uint16_t Port, ulong_t Addr, TAddressFamily AdFamily = AF_INET) : NetName(Port, Addr, AdFamily) {
			Socket = INVALID_SOCKET;
		}

		RStatus Initialize(TSocket Socket, const sockaddr_in* Name) noexcept {
			this->Socket = Socket;
			NameLen = sizeof(this->Name);

			memcpy_s(&this->Name, NameLen, Name, NameLen);

			return RSuccess;
		}
		RStatus Initialize(uint16_t Port, ulong_t Addr, TAddressFamily AdFamily = AF_INET) {
			NameLen = sizeof(Name);
			memset(&Name, 0, sizeof(Name));

			Name.sin_port = htons(Port);
			Name.sin_family = AdFamily;
			Name.sin_addr.s_addr = Addr;

			return RSuccess;
		}

		inline RStatus Close() noexcept {
			if (shutdown(Socket, SD_BOTH)) {
#if COMMONEX_WIN32_PLATFROM
				R_SET_LAST_ERROR_FMT("NetEndpoint::Close() Failed to shutdown() WSAERROR[{}]", WSAGetLastError());
#else
				static_assert(false, "@TODO")
#endif
			}

			if (closesocket(Socket))
			{
#if COMMONEX_WIN32_PLATFROM
				R_SET_LAST_ERROR_FMT("NetEndpoint::Close() Failed to closesocket() WSAERROR[{}]", WSAGetLastError());
#else
				static_assert(false, "@TODO")
#endif
					return RFail;
			}

			return RSuccess;
		}
	};

	struct TCPEndpoint : NetEndpoint {
		//Receive flags.
		ulong_t				Flags = 0;

		TCPEndpoint() : NetEndpoint() {}

		inline RStatus Initialize(uint16_t Port, ulong_t Addr, TAddressFamily AdFamily = AF_INET, bool bOverlappedSocket = true) noexcept {
#if COMMONEX_WIN32_PLATFROM
			Socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, bOverlappedSocket ? WSA_FLAG_OVERLAPPED : 0);
			if (Socket == INVALID_SOCKET)
			{
				return RFail;
			}
#else
			static_assert(false, "@TODO");
#endif

			RTRY_L_FMT(NetEndpoint::Initialize(Port, Addr), "TCPEndpoint::Initialize() failed to NetEndpoint::Initialize({}, {})", Port, Addr) {}

			return RSuccess;
		}

		inline RStatus Initialize(TSocket Socket, const sockaddr_in* Name) noexcept {
			return NetEndpoint::Initialize(Socket, Name);
		}

		RStatus AssoctiateToAsyncIO(CurrentAsyncIOSystem* AsyncIOInterface)  noexcept {
#if COMMONEX_WIN32_PLATFROM
			this->AsyncIOInterface = AsyncIOInterface;

			HANDLE TempHandle = CreateIoCompletionPort((HANDLE)Socket, AsyncIOInterface->IOCPHandle, NULL, AsyncIOInterface->WorkersCount);
			if (TempHandle != AsyncIOInterface->IOCPHandle)
			{
				return RFail;
			}
#else
			static_assert(false, "@TODO");
#endif

			return RSuccess;
		}

		/*Attempts to connect the Socket to Name*/
		FORCEINLINE RStatus Connect() noexcept {
			if (connect(Socket, (const sockaddr*)&Name, sizeof(Name)) == SOCKET_ERROR)
			{
				//RTRY_L_FMT(RFail, "TCPEndpoint::Connect() failed to connect() WSAERROR[{}]", WSAGetLastError());
				return (RStatus)WSAGetLastError();
			}

			return RSuccess;
		}

		FORCEINLINE int32_t Receive(IBuffer* Buffer, int32_t length = -1, int32_t flags = 0) noexcept {
			if (length < 0)
				length = (int)Buffer->Length;

			return recv(Socket, (char*)Buffer->Buffer, length, flags);
		}

		FORCEINLINE int32_t Send(const IBuffer* Buffer, int32_t length = -1, int32_t flags = 0) noexcept {
			if (length < 0)
				length = (int)Buffer->Length;

			return send(Socket, (const char*)Buffer->Buffer, length, flags);
		}

		FORCEINLINE RStatus ReceiveAsync(IBuffer* Buffer, OsOverlappedType* Work) noexcept {
			return AsyncIOInterface->ReceiveAsync(Socket, Buffer, Work);
		}

		FORCEINLINE RStatus SendAsync(IBuffer* Buffer, OsOverlappedType* Work) const noexcept {
			return AsyncIOInterface->SendAsync(Socket, Buffer, Work);
		}

	private:
		CurrentAsyncIOSystem* PTR		AsyncIOInterface{ nullptr };
	};

	struct TCPListenerWorker : NetEndpoint {
		using AcceptHandler = Delegate<void, TCPListenerWorker*, SOCKET, sockaddr_in*>;

		RStatus Initialize(uint16_t Port, ulong_t Addr, AcceptHandler OnAccept, int32_t MaxPendingConnections = SOMAXCONN) noexcept {
#if COMMONEX_WIN32_PLATFROM
			Socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (Socket == INVALID_SOCKET)
			{
				return RFail;
			}

			Name.sin_family = AF_INET;
			Name.sin_addr.s_addr = Addr;
			Name.sin_port = htons(Port);
#else 
			static_assert(false, "@TODO");
#endif
			if (bind(Socket, (const sockaddr*)&Name, NameLen) == SOCKET_ERROR)
			{
				RTRY_L(RFail, "TCPListenerWorker::Initialize() failed to bind()") {}
			}

			if (listen(Socket, MaxPendingConnections))
			{
				RTRY_L_FMT(RFail, "TCPListenerWorker::Initialize() failed to listen(MaxPendingConnections: {})", MaxPendingConnections) {}
			}

#if COMMONEX_WIN32_PLATFROM

			AcceptEvent = WSACreateEvent();
			if (AcceptEvent == WSA_INVALID_EVENT)
			{
				RTRY_L(RFail, "TCPListenerWorker::Initialize() failed to WSACreateEvent()") {}
			}

			int32_t Result = WSAEventSelect(Socket, AcceptEvent, FD_ACCEPT);
			if (Result)
			{
				Result = WSAGetLastError();
				RTRY_L_FMT(RFail, "TCPListenerWorker::Initialize() failed to WSAEventSelect() WSAERROR[{}]", Result) {}
			}

#else 
			static_assert(false, "@TODO");
#endif

			this->OnAccept = std::move(OnAccept);

			Thread = std::thread(&TCPListenerWorker::AcceptRoutine, this);

			return RSuccess;
		}

		inline void Start()noexcept {
			Run.store(TRUE);
		}
		inline void Stop()noexcept {
			if (Run.load() == FALSE) {
				return;
			}

			Run.store(FALSE);

#if COMMONEX_WIN32_PLATFROM
			if (!WSACloseEvent(AcceptEvent)) {
				LogWarning("TCPListenerWorker::Failed to close WSA event!");
			}

			AcceptEvent = nullptr;
#else
			static_assert(false, "@TODO");
#endif

			Close();
			Thread.join();
		}

		~TCPListenerWorker() {
			Stop();
		}

	private:
		static void	WaitForRun(TCPListenerWorker* Worker, int32_t Milliseconds) noexcept {
			auto Start = std::chrono::high_resolution_clock::now();

			while (!Worker->Run.load())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

				auto Elapsed = std::chrono::high_resolution_clock::now() - Start;

				if (std::chrono::duration<double, std::milli>(Elapsed).count() >= Milliseconds)
				{
					break;
				}
			}

			if (!Worker->Run.load())
			{
#if VERBOSE_TLAYER
				LogFatal("TCPListenerWorker::WaitForRun():: Worker timedout!");
#endif
			}
		}
		static void AcceptRoutine(TCPListenerWorker* Listener) noexcept {
			TSocket listenSock = Listener->Socket;

#if COMMONEX_WIN32_PLATFROM
			int32_t result = 0;
			sockaddr_in clientInfo;
			TSocket sock = 0;
			WSANETWORKEVENTS WSAEvents;

			if (!Listener->Run.load())
			{
				TCPListenerWorker::WaitForRun(Listener, 2 * 60 * 1000);
			}

			if (Listener->Run.load())
			{
#if VERBOSE_TLAYER
				LogInfo("::TCPListenerWorker:: Started listening for connections...");
#endif
			}

			while (Listener->Run.load() == true)
			{
				if (WSA_WAIT_TIMEOUT != WSAWaitForMultipleEvents(1, &Listener->AcceptEvent, FALSE, 200, FALSE))
				{
					WSAEnumNetworkEvents(listenSock, Listener->AcceptEvent, &WSAEvents);
					if ((WSAEvents.lNetworkEvents & FD_ACCEPT) && (0 == WSAEvents.iErrorCode[FD_ACCEPT_BIT]))
					{
						sock = accept(listenSock, (sockaddr*)&clientInfo, NULL);
						if (sock == INVALID_SOCKET)
						{
							//@TODO log
							continue;
						}

						Listener->OnAccept(Listener, sock, &clientInfo);
					}
				}
			}
#else
			static_assert(false, "@TODO");
#endif

		}

#if COMMONEX_WIN32_PLATFROM
		//Win32 accept event handle.
		HANDLE				AcceptEvent = NULL;
#else
		static_assert(false, "@TODO");
#endif

		//Accept thread.
		std::thread			Thread;

		//Should this run.
		std::atomic<bool>	Run = false;

		//On accept handler.
		AcceptHandler		OnAccept{};
	};
}
