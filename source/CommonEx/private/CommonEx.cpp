#include "../public/CommonEx.h"

using namespace CommonEx;

//	Signal handlers

/*Abnormal termination handler*/
void  AbortSignal(int sig) noexcept {
	Alert("Abort Signal received!");
	LogFatal("@TODO in depth trace");

	//std::cin.get();
}

/*Floating-point error handler*/
void  FloatingPointErrorSignal(int sig) noexcept {
	Alert("Floating Point Error Signal received!");
	LogFatal("@TODO in depth trace");

	//std::cin.get();
}

/*Illegal instruction handler*/
void  IllegalInstructionSignal(int sig) noexcept {
	Alert("Illegal Instruction Signal received!");
	LogFatal("@TODO in depth trace");

	//std::cin.get();
}

/*CTRL+C signal handler*/
void KillSignal(int sig) {
	Alert("Kill Signal received!");
	LogFatal("@TODO in depth trace");

	//std::cin.get();
}

/*Illegal storage access handler*/
void IllegalStorageAccessSignal(int sig) {
	Alert("Illegal Storage Access Signal received!");
	LogFatal("@TODO in depth trace");

	//std::cin.get();
}

/*Termination request handler*/
void TerminationSignal(int sig) {
	LogFatal("Abort Signal received!");
	LogFatal("@TODO in depth trace");

	//std::cin.get();
}

//Diag.h
namespace CommonEx {
	//Global diagnostic and logging tool
	GlobalDiagnostics GlobalDiagnostics::GDiag;

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

	RStatus GlobalDiagnostics::BuildRStatusDescriptions()
	{
		RStatusDesc[(int32_t)RStatus::Fail] = "General failure";

		return RSuccess;
	}
}