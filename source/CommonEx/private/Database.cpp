#include "../public/CommonEx.h"

using namespace CommonEx;
using namespace Database;

DBConnection::DBLibGuard::DBLibGuard()
{
#if _DEBUG
	LogInfo("DBConnection::DBLibGuard::TryInitLib()");
#endif

	IsValid = !mysql_library_init(0, nullptr, nullptr);
}

DBConnection::DBLibGuard::~DBLibGuard()
{
	if (IsValid)
	{
#if _DEBUG
		LogInfo("DBConnection::DBLibGuard::EndLib()");
#endif

		mysql_library_end();
		IsValid = false;
	}
}

DBStatement::DBStatementResult DBStatement::DBStatementResult::EmptyResult;

DBConnection::~DBConnection()
{
	TryCloseConnection();
}

RStatus DBConnection::InitializeAdaptor(DBConnectionSettings&& settings) noexcept
{
	if (bIsAdaptorInitialized)
	{
		return RSuccess;
	}

	Settings = std::move(settings);

	bIsAdaptorInitialized = TRUE;

	return RSuccess;
}

RStatus DBConnection::Initialize() noexcept
{
	if (IsOpen())
	{
		return RSuccess;
	}

	if (!LibGuard.IsValidLib())
	{
#if _DEBUG
		LogWarning("DBConnection::Initialize() -> Failed Invalid Lib");
#endif
		return RFail;
	}

	auto Result = mysql_init(&Mysql);
	if (!Result)
	{
#if _DEBUG
		LogWarning("DBConnection::Initialize() -> Failed mysql_init()");
#endif
		return RFail;
	}

	if (!SetOptions(CMysqlConnectionTimeout))
	{
		return RFail;
	}

	return RSuccess;
}

RStatus DBConnection::TryOpenConnection() noexcept
{
	if (IsOpen())
	{
		return RSuccess;
	}

	ulong_t Flags = Settings.enableMultiStatements ? CLIENT_MULTI_STATEMENTS : 0;

	auto Result = mysql_real_connect(&Mysql, Settings.host.c_str(), Settings.username.c_str(), Settings.password.c_str(), Settings.database.c_str(), Settings.port, nullptr, Flags);
	if (!Result)
	{

#if DEBUG_MYSQL
		auto MysqlStmtError = mysql_error(&Mysql);

		LogWarning("MYSQL-ERROR::{}", MysqlStmtError);
#endif

		LogWarning("DBConnection::TryOpenConnection() -> Failed");
		return RFail;
	}

	bIsOpen = TRUE;

#if _DEBUG
	LogInfo("DBConnection::TryOpenConnection() -> Success");
#endif

	return RSuccess;
}

void DBConnection::TryCloseConnection() noexcept
{
	if (IsOpen())
	{
		mysql_close(&Mysql);
		bIsOpen = false;

#if _DEBUG
		LogInfo("DBConnection::TryCloseConnection() -> Success");
#endif
	}
}

bool DBConnection::TryReacquireConnection(const INT Tries) noexcept
{
	int32_t i = Tries;

	do
	{
		TryCloseConnection();

		if (TryOpenConnection() == RSuccess)
		{
			break;
		}

		i--;
	} while (i > 0);

	return i != 0;
}

bool DBConnection::SetOptions(UINT ConnectionTimeout) noexcept
{
	INT ProtoType = MYSQL_PROTOCOL_TCP;
	if (mysql_options(&Mysql, MYSQL_OPT_PROTOCOL, &ProtoType))
	{
		return false;
	}

	BOOL Reconnect = TRUE;
	if (mysql_options(&Mysql, MYSQL_OPT_RECONNECT, &Reconnect))
	{
		return false;
	}

	/*if (mysql_options(&Mysql, MYSQL_OPT_CONNECT_TIMEOUT, &ConnectionTimeout)) {
		return false;
	}*/

	if (mysql_set_character_set(&Mysql, "utf8"))
	{
		return false;
	}

	return true;
}