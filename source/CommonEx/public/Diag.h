#pragma once 
/**
 * @file Diag.h
 *
 * @brief CommonEx error reporting, signal handling and logging
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

namespace CommonEx {
	struct GlobalDiagnostics {
		RStatus Initialize() noexcept;
		RStatus Shutdown() noexcept;

		template<typename ...Types>
		inline void SetLastError(const char* Message, Types... Args) {
			if constexpr (sizeof...(Types) != 0) {
				LastError = fmt::format(Message, Args...);
			}
			else {
				LastError = Message;
			}
		}

		template<typename ...Types>
		inline void SetLastError(const char* FileName, int32_t LineNumber, const char* Message, Types... Args) noexcept {
			SetLastError(Message, Args...);

			LastError = fmt::format("{}\n\tAt: Line[{}] - File:{}", LastError.c_str(), LineNumber, FileName);
		}

		inline const char* GetLastError() const noexcept {
			return LastError.c_str();
		}

		static GlobalDiagnostics GDiag;

		std::string RStatusDesc[(int32_t)RStatus::MAX];

	private:
		RStatus BuildRStatusDescriptions();

		//Thread local LastError
		static inline thread_local std::string LastError;
	};

	//////////////////////
	// Logging API
	//////////////////////

	static inline std::string BuildTimeHeader(const char* Message, fmt::terminal_color color /*@TODO coloring*/) noexcept {
		time_t now = time(0);
		tm ltm;
		localtime_s(&ltm, &now);

		std::string Header = fmt::format(
			"[{}:{}:{}] {}:",
			ltm.tm_hour, ltm.tm_min, ltm.tm_sec, Message);

		return std::move(Header);
	}

	static inline std::wstring BuildTimeHeaderW(const wchar_t* Message, fmt::terminal_color color /*@TODO coloring*/)  noexcept {
		time_t now = time(0);
		tm ltm;
		localtime_s(&ltm, &now);

		std::wstring Header = fmt::format(
			L"[{}:{}:{}] {}:",
			ltm.tm_hour, ltm.tm_min, ltm.tm_sec, Message);

		return std::move(Header);
	}

	template<typename ...Types>
	inline void Log(const char* Format, Types... Args)  noexcept {
		fmt::print(Format, Args...);

		//@TODO to disk buffered logging
		/*const std::string Msg = fmt::format(Format, Args...);

		Log(Msg.c_str());*/
	}

	template<typename ...Types>
	inline void Log(const wchar_t* Format, Types... Args)  noexcept {
		fmt::print(Format, Args...);

		//@TODO to disk buffered logging
		/*const std::string Msg = fmt::format(Format, Args...);

		Log(Msg.c_str());*/
	}

	template<typename ...Types>
	inline void LogFatal(const char* Format, Types... Args) noexcept {
		std::string Header = BuildTimeHeader("Fatal", fmt::terminal_color::red);
		std::string Msg = fmt::format(Format, Args...);

		Log("{} {}\n", Header.c_str(), Msg.c_str());
	}

	template<typename ...Types>
	inline void LogFatal(const wchar_t* Format, Types... Args) noexcept {
		std::wstring Header = BuildTimeHeaderW(L"Fatal", fmt::terminal_color::red);
		std::wstring Msg = fmt::format(Format, Args...);

		Log(L"{} {}\n", Header.c_str(), Msg.c_str());
	}

	template<typename ...Types>
	inline void LogWarning(const char* Format, Types... Args) noexcept {
		std::string Header = BuildTimeHeader("Warning", fmt::terminal_color::yellow);
		std::string Msg = fmt::format(Format, Args...);

		Log("{} {}\n", Header.c_str(), Msg.c_str());
	}

	template<typename ...Types>
	inline void LogWarning(const wchar_t* Format, Types... Args) noexcept {
		std::wstring Header = BuildTimeHeaderW(L"Warning", fmt::terminal_color::yellow);
		std::wstring Msg = fmt::format(Format, Args...);

		Log(L"{} {}\n", Header.c_str(), Msg.c_str());
	}

	template<typename ...Types>
	inline void LogInfo(const char* Format, Types... Args)  noexcept {
		std::string Header = BuildTimeHeader("Info", fmt::terminal_color::bright_blue);
		std::string Msg = fmt::format(Format, Args...);

		Log("{} {}\n", Header.c_str(), Msg.c_str());
	}

	template<typename ...Types>
	inline void LogInfo(const wchar_t* Format, Types... Args)  noexcept {
		std::wstring Header = BuildTimeHeaderW(L"Info", fmt::terminal_color::bright_blue);
		std::wstring Msg = fmt::format(Format, Args...);

		Log(L"{} {}\n", Header.c_str(), Msg.c_str());
	}

	template<typename ...Types>
	inline void LogTODO(const char* Format, Types... Args)  noexcept {
		std::string Header = BuildTimeHeader("TODO", fmt::terminal_color::yellow);
		std::string Msg = fmt::format(Format, Args...);

		Log("{} {}\n", Header.c_str(), Msg.c_str());
	}

	template<typename ...Types>
	inline void LogTODO(const wchar_t* Format, Types... Args) noexcept {
		std::wstring Header = BuildTimeHeaderW(L"TODO", fmt::terminal_color::yellow);
		std::wstring Msg = fmt::format(Format, Args...);

		Log(L"{} {}\n", Header.c_str(), Msg.c_str());
	}

	template<typename ...Types>
	inline void Alert(const char* Format, Types... Args)  noexcept;

	template<typename ...Types>
	inline void Alert(const wchar_t* Format, Types... Args)  noexcept;

	//////////////////////
	// Report API
	//////////////////////
	static std::string RStatus_Unknown = "Unknown Error Code";

	inline bool BuildFailReport(const RStatus Status) noexcept {
		if (Status == RSuccess) {
			return false;
		}

		std::string& Desc = GlobalDiagnostics::GDiag.RStatusDesc[(int32_t)Status];
		if (Desc == "") {
			Desc = RStatus_Unknown;
		}

		LogFatal("Failed with status ({}) Desc({})\n\tLastError: {}",
			(int32_t)Status,
			Desc.c_str(),
			GlobalDiagnostics::GDiag.GetLastError());

		return true;
	}
}

namespace CommonEx {

#if RVERBOSE
	//Set Last error message [verbose]
#define R_SET_LAST_ERROR(Message) \
			GlobalDiagnostics::GDiag.SetLastError(__FILE__, __LINE__, Message);

//Set Last error message [format] [verbose]
#define R_SET_LAST_ERROR_FMT(Message, ...) \
			GlobalDiagnostics::GDiag.SetLastError(__FILE__, __LINE__, Message, __VA_ARGS__);
#else 
	//Set Last error message 
#define R_SET_LAST_ERROR(Message) \
			GlobalDiagnostics::GDiag.SetLastError(Message);

//Set Last error message [format]
#define R_SET_LAST_ERROR_FMT(Message, ...) \
			GlobalDiagnostics::GDiag.SetLastError(Message, __VA_ARGS__);
#endif

//RTRY_SIMPLE
//If (expr == RFail) { 1.Build a detailed report 2.Return RFail }
#define R_TRY(expr)															\
					const RStatus CONCAT(RTryResult, __LINE__) = expr;		\
					if(0 != (int32_t)CONCAT(RTryResult, __LINE__)) {		\
						BuildFailReport(CONCAT(RTryResult, __LINE__));		\
						return RFail;										\
					} else

//RTRY_SIMPLE
//If (expr == RFail) { 1.Build a detailed report 2.Return [RFailStatus] }
#define RTRY_S(expr, RFailStatus)											\
					const RStatus CONCAT(RTryResult, __LINE__) = expr;		\
					if(0 != (int32_t)CONCAT(RTryResult, __LINE__)) {		\
						BuildFailReport(CONCAT(RTryResult, __LINE__));		\
						return RFailStatus;									\
					} else

//RTRY_SIMPLE_LAST_ERROR
//If (expr == RFail) { 1.Set LastError=[FailMessage] 2.Build a detailed report 3.Return [RFailStatus] }
#define RTRY_S_L(expr, RFailStatus, FailMessage)							\
					const RStatus CONCAT(RTryResult, __LINE__) = expr;		\
					if(0 != (int32_t)CONCAT(RTryResult, __LINE__)) {		\
						R_SET_LAST_ERROR(FailMessage);						\
						BuildFailReport(CONCAT(RTryResult, __LINE__));		\
						return RFailStatus;									\
					} else

//RTRY_LAST_ERROR
//If (expr == RFail) { 1.Set LastError=[FailMessage] 2.Build a detailed report 3.Return RFail }
#define RTRY_L(expr, FailMessage)											\
					const RStatus CONCAT(RTryResult, __LINE__) = expr;		\
					if(0 != (int32_t)CONCAT(RTryResult, __LINE__)) {		\
						R_SET_LAST_ERROR(FailMessage);						\
						BuildFailReport(CONCAT(RTryResult, __LINE__));		\
						return RFail;										\
					} else

//R_TRY_LAST_ERROR
//If (expr == RFail) { 1.Set LastError=[FailMessage] 2.Build a detailed report 3.Return given status }
#define R_TRY_L(expr, FailMessage)											\
					const RStatus CONCAT(RTryResult, __LINE__) = expr;		\
					if(0 != (int32_t)CONCAT(RTryResult, __LINE__)) {		\
						R_SET_LAST_ERROR(FailMessage);						\
						BuildFailReport(CONCAT(RTryResult, __LINE__));		\
						return CONCAT(RTryResult, __LINE__);				\
					} else

//RTRY_LAST_ERROR_FORMAT
//If (expr == RFail) { 1.Set LastError=[fmt(FailMessage, ...)] 2.Build a detailed report 3.Return RFail }
#define RTRY_L_FMT(expr, FailMessage, ...)									\
					const RStatus CONCAT(RTryResult, __LINE__) = expr;		\
					if(0 != (int32_t)CONCAT(RTryResult, __LINE__)) {		\
						R_SET_LAST_ERROR_FMT(FailMessage, __VA_ARGS__);		\
						BuildFailReport(CONCAT(RTryResult, __LINE__));		\
						return RFail;										\
					} else

//RTRY
//If (expr == RFail) { 1.Set LastError=[FailMessage] 2.Build a detailed report 3.Return [RFailStatus] }
#define RTRY(expr, FailMessage, RFailStatus)								\
					const RStatus CONCAT(RTryResult, __LINE__) = expr;		\
					if(0 != (int32_t)CONCAT(RTryResult, __LINE__)) {		\
						R_SET_LAST_ERROR(FailMessage);						\
						BuildFailReport(CONCAT(RTryResult, __LINE__));		\
						return RFailStatus;									\
					} else

//RTRY_FOMAT
//If (expr == RFail) { 1.Set LastError=[fmt(FailMessage, ...)] 2.Build a detailed report 3.Return [RFailStatus] }
#define RTRY_FMT(expr, FailMessage, RFailStatus, ...)						\
					const RStatus CONCAT(RTryResult, __LINE__) = expr;		\
					if(0 != (int32_t)CONCAT(RTryResult, __LINE__)) {		\
						R_SET_LAST_ERROR_FMT(FailMessage, __VA_ARGS__);		\
						BuildFailReport(CONCAT(RTryResult, __LINE__));		\
						return RFailStatus;									\
					} else
}