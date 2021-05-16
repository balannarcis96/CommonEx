#include "../public/CommonEx.h"

using namespace CommonEx;
using namespace CommonEx::Utils;

//	Signal handlers

/*Abnormal termination handler*/
void  AbortSignal(int sig) noexcept
{
	Alert("Abort Signal received!");
	LogFatal("@TODO in depth trace");

	//std::cin.get();
}

/*Floating-point error handler*/
void  FloatingPointErrorSignal(int sig) noexcept
{
	Alert("Floating Point Error Signal received!");
	LogFatal("@TODO in depth trace");

	//std::cin.get();
}

/*Illegal instruction handler*/
void  IllegalInstructionSignal(int sig) noexcept
{
	Alert("Illegal Instruction Signal received!");
	LogFatal("@TODO in depth trace");

	//std::cin.get();
}

/*CTRL+C signal handler*/
void KillSignal(int sig)
{
	Alert("Kill Signal received!");
	LogFatal("@TODO in depth trace");

	//std::cin.get();
}

/*Illegal storage access handler*/
void IllegalStorageAccessSignal(int sig)
{
	Alert("Illegal Storage Access Signal received!");
	LogFatal("@TODO in depth trace");

	//std::cin.get();
}

/*Termination request handler*/
void TerminationSignal(int sig)
{
	LogFatal("Abort Signal received!");
	LogFatal("@TODO in depth trace");

	//std::cin.get();
}

//Diag.h
namespace CommonEx
{
	//Global diagnostic and logging tool
	GlobalDiagnostics GlobalDiagnostics::GDiag;

	RStatus GlobalDiagnostics::BuildRStatusDescriptions()
	{
		RStatusDesc[(int32_t)RStatus::Success] = "Operation succeeded";
		RStatusDesc[(int32_t)RStatus::Fail] = "Operation failed";
		RStatusDesc[(int32_t)RStatus::Aborted] = "Operation aborted";
		RStatusDesc[(int32_t)RStatus::AcquireFailed] = "Acquire failed";
		RStatusDesc[(int32_t)RStatus::AlreadyPerformed] = "Operation already performed";
		RStatusDesc[(int32_t)RStatus::ConnectionLost] = "Connection lost";
		RStatusDesc[(int32_t)RStatus::NotImplemented] = "Operation not implemented";
		RStatusDesc[(int32_t)RStatus::Timedout] = "Operation timedout";
		RStatusDesc[(int32_t)RStatus::WorkRemains] = "Operation not completly finished";

		return RSuccess;
	}

	RStatus GlobalDiagnostics::Initialize() noexcept
	{
		//Hook sinals
		std::signal(SIGABRT, AbortSignal);
		std::signal(SIGFPE, FloatingPointErrorSignal);
		std::signal(SIGILL, IllegalInstructionSignal);
		std::signal(SIGINT, KillSignal);
		std::signal(SIGSEGV, IllegalStorageAccessSignal);
		std::signal(SIGTERM, TerminationSignal);

		//Build RStatus desc vector
		RTRY_L(BuildRStatusDescriptions(), "Failed to BuildRStatusDescriptions()") {}

		return RSuccess;
	}

	RStatus GlobalDiagnostics::Shutdown() noexcept
	{
		//@TODO save log if needed

		return RSuccess;
	}
}

#if COMMONEX_WIN32_PLATFROM
WSAData		GWSAData;
#else
#error @TODO InitializeCommonEx()
#endif

//CommonEx.h
namespace CommonEx
{
	RStatus InitializeCommonEx(int32_t argc, const char** argv)noexcept
	{
#if COMMONEX_WIN32_PLATFROM
		R_TRY_L((RStatus)WSAStartup(MAKEWORD(2, 2), &GWSAData), "InitializeCommonEx() -> Failed to WSAStartup() !") {}
#else
#error @TODO InitializeCommonEx()
#endif

		R_TRY_L(GlobalDiagnostics::GDiag.Initialize(), "CommonEx::InitializeCommonEx() -> Failed to GlobalDiagnostics::Initialize()() !") {}
		R_TRY_L(MemoryManager::Initialize(), "CommonEx::InitializeCommonEx() -> Failed to MemoryManager::Initialize() !") {}
		R_TRY_L(TSendBuffer::Initialize(), "CommonEx:: Failed to Initialize TSendBuffer Pools"){}
		R_TRY_L(TRecvBuffer::Initialize(), "CommonEx:: Failed to Initialize TRecvBuffer Pools"){}
		R_TRY_L(CurrentAsyncIOSystem::Initialize(), " CommonEx:: Failed to initialize the AsyncIO system (Win32AsyncIO)!") {}

		return RSuccess;
	}

	RStatus ShutdownCommonEx()noexcept
	{
#if COMMONEX_WIN32_PLATFROM
		
#else
#error @TODO ShutdownCommonEx()
#endif

		R_TRY_L(MemoryManager::Shutdown(), "ShutdownCommonEx() -> Failed to MemoryManager::Shutdown() !") {}
		R_TRY_L(GlobalDiagnostics::GDiag.Shutdown(), "InitializeCommonEx() -> Failed to GlobalDiagnostics::Shutdown()() !") {}

		return RSuccess;
	}
}

//MemoryManager.h
namespace CommonEx
{
#ifdef MEMEX_STATISTICS
	std::atomic<size_t> MemoryManager::CustomSizeDeallocations;
	std::atomic<size_t> MemoryManager::CustomSizeAllocations;
#endif
}

//Utils.h

#if COMMONEX_WIN32_PLATFROM
bool GWideCharToMultiByte(const wchar_t* InBuffer, char* OutBuffer, int32_t OutBufferSize) noexcept
{
	int32_t result = 0;
	if ((result = ::WideCharToMultiByte(CP_UTF8, 0, InBuffer, (int32_t)wcslen(InBuffer), OutBuffer, OutBufferSize, 0, 0)) == 0)
	{
		return false;
	}

	OutBuffer[result] = '\0';

	return true;
}

bool GMultiByteToWideChar(const char* InBuffer, wchar_t* OutBuffer, int32_t OutBufferSize) noexcept
{
	int32_t result = 0;
	if ((result = ::MultiByteToWideChar(CP_UTF8, 0, InBuffer, (int32_t)strlen(InBuffer), OutBuffer, OutBufferSize)) == 0)
	{
		return false;
	}

	OutBuffer[result] = '\0';

	return true;
}

#else 
static_assert(false, "@TODO");
#endif