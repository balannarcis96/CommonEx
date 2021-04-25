#pragma once
/**
 * @file Win32Platform.h
 *
 * @brief CommonEx Win32 Platform specific abstractions
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

#ifdef COMMONEX_WIN32_PLATFROM

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

namespace CommonEx {
	using TSocket = SOCKET;

	using OsOverlappedType = OVERLAPPED;

	using TAddressFamily = ADDRESS_FAMILY;

	struct Win32AsyncIO {
		static RStatus Initialize() noexcept {
			int32_t WSAStartupResult = WSAStartup(MAKEWORD(2, 2), &WSA);
			if (WSAStartupResult) {
				RTRY_L_FMT(RFail, "Win32AsyncIO::Initialize() Failed to WSAStartup() returned [{}]", WSAStartupResult) {}
			}

			return RSuccess;
		};
		static RStatus ShutdownSystem() noexcept {
			int32_t WSACleanupResult = WSACleanup();
			if (WSACleanupResult) {
				RTRY_L_FMT(RFail, "Win32AsyncIO::Shutdown() Failed to WSACleanup() returned [{}]", WSACleanupResult) {}
			}

			return RSuccess;
		};

		RStatus Start(int32_t WorkersCount) noexcept {
			this->WorkersCount = WorkersCount;

			IOCPHandle = CreateIoCompletionPort((HANDLE)INVALID_HANDLE_VALUE, NULL, NULL, WorkersCount);
			if (IOCPHandle == NULL)
			{
				LastWSAError = WSAGetLastError();
				R_SET_LAST_ERROR_FMT("Win32AsyncIO::Start() Failed to create IOCP Handle WSAERROR[{}]", LastWSAError);
				return RFail;
			}

			return RSuccess;
		};
		RStatus Shutdown(int32_t Timeout = -1) noexcept {
			if (IOCPHandle == NULL) {
				return RSuccess;
			}

			if (!CloseHandle(IOCPHandle)) {
				return RFail;
			}

			IOCPHandle = nullptr;

			return RSuccess;
		};

		//Pool ready to process work from the underlying system(called by the worker threads)
		RStatus GetWork(OsOverlappedType** OutWork, int32_t Timeout = -1) noexcept
		{
			if (!GetQueuedCompletionStatus(
				IOCPHandle,
				reinterpret_cast<LPDWORD>(&NumberOfBytesTransferred),
				reinterpret_cast<PULONG_PTR>(&CompletionKey),
				reinterpret_cast<OVERLAPPED**>(OutWork),
				static_cast<DWORD>(Timeout)))
			{
				LastWSAError = WSAGetLastError();
				LogWarning("Win32AsyncIO::GetWork() failed with WSAERROR[{}]", LastWSAError);
				return (RStatus)LastWSAError;
			}

			return RSuccess;
		};

		//General work API
		RStatus DoAsync(OsOverlappedType* Work) noexcept
		{
			if (!PostQueuedCompletionStatus(IOCPHandle, 1, NULL, (OVERLAPPED*)Work)) {
				LastWSAError = WSAGetLastError();
				LogWarning("Win32AsyncIO::DoAsync() failed with WSAERROR[{}]", LastWSAError);
				return RFail;
			}

			return RSuccess;
		};

		//NetworkIO (TCP)
		RStatus ReceiveAsync(TSocket Socket, IBuffer* Buffer, OsOverlappedType* Work) noexcept {
			DWORD Flags = 0;
			int32_t SendResult = WSARecv(
				Socket,
				reinterpret_cast<LPWSABUF>(Buffer),
				1,
				NULL,
				&Flags,
				reinterpret_cast<OVERLAPPED*>(Work),
				NULL);

			if (SendResult == SOCKET_ERROR) {
				LastWSAError = WSAGetLastError();
				if (LastWSAError != WSA_IO_PENDING) {
					LogWarning("Win32AsyncIO::ReceiveAsync() Failed WSAERROR[{}]", LastWSAError);
					return RFail;
				}
			}

			return RSuccess;
		}
		RStatus SendAsync(TSocket Socket, IBuffer* Buffer, OsOverlappedType* Work) noexcept {
			DWORD Flags = 0;
			int32_t SendResult = WSASend(
				Socket,
				reinterpret_cast<LPWSABUF>(Buffer),
				1,
				NULL,
				Flags,
				reinterpret_cast<OVERLAPPED*>(Work),
				NULL);

			if (SendResult == SOCKET_ERROR) {
				LastWSAError = WSAGetLastError();
				if (LastWSAError != WSA_IO_PENDING) {
					LogWarning("Win32AsyncIO::SendAsync() Failed WSAERROR[{}]", LastWSAError);
					return RFail;
				}
			}

			return RSuccess;
		}

		//Uppon work completions, call this to get the result of the async operation
		RStatus OnWorkCompleted(uint32_t& OutNumberOfBytesTransferred) noexcept {
			OutNumberOfBytesTransferred = NumberOfBytesTransferred;
			if (!NumberOfBytesTransferred) {
				return RFail;
			}

			return RSuccess;
		}

	private:
		HANDLE						IOCPHandle{ INVALID_HANDLE_VALUE };
		int32_t						WorkersCount{ 0 };
		int32_t						LastWSAError{ 0 };
		uint32_t					NumberOfBytesTransferred{ 0 };
		uint32_t					CompletionKey{ 0 };
		static inline WSADATA		WSA{};

		friend struct TCPEndpoint;
	};
}

#endif