#pragma once

namespace CommonEx
{
	namespace Database
	{
		template<size_t MaxSize>
		struct DbString
		{
			static const size_t CUtf16Size = MaxSize;
			static const size_t CUtf8Size = (CUtf16Size) * 4; //utf8mb4

			union
			{
				struct
				{
					unsigned	bHasSource : 1;
					unsigned	bIsUTF8Src : 1;
					unsigned	bHasUTF8 : 1;
					unsigned	bHasUTF16 : 1;
				};
				char			Flags = 0;
			};
			char				Utf8[CUtf8Size];
			wchar_t				Utf16[CUtf16Size];

			DbString() : bHasSource(false) { Utf8[0] = '\0'; Utf16[0] = L'\0'; }
			//DbString(bool IsUtf8Src = true) : bHasSource(true), bIsUTF8Src(IsUtf8Src) { Utf8[0] = '\0'; Utf16[0] = L'\0'; }

			//Copy
			DbString(const DbString& other)noexcept
			{
				Flags = other.Flags;
				memcpy_s(Utf8, CUtf8Size, other.Utf8, CUtf8Size);
				memcpy_s(Utf16, sizeof(wchar_t) * CUtf16Size, other.Utf16, sizeof(wchar_t) * CUtf16Size);
			}
			DbString& operator=(const DbString& other)noexcept
			{
				if (&other == this)
					return *this;

				Flags = other.Flags;
				memcpy_s(Utf8, CUtf8Size, other.Utf8, CUtf8Size);
				memcpy_s(Utf16, sizeof(wchar_t) * CUtf16Size, other.Utf16, sizeof(wchar_t) * CUtf16Size);

				return *this;
			}

			//Cant move
			DbString(DbString&& other) = delete;
			DbString& operator=(DbString&& other) = delete;

			static DbString<MaxSize> FromUtf8(const char* Utf8)
			{
				return DbString<MaxSize>(Utf8);
			}
			static DbString<MaxSize> FromUtf16(const wchar_t* Utf16)
			{
				return DbString<MaxSize>(Utf16);
			}

			const char* GetUtf8() noexcept
			{
				if (bHasSource && !bHasUTF8)
				{
					if (!GWideCharToMultiByte(Utf16, Utf8))
					{
						auto err = GGetLastError();

						return nullptr;
					}

					bHasUTF8 = true;
				}

				return Utf8;
			}
			const wchar_t* GetUtf16() noexcept
			{
				if (bHasSource && bHasUTF16)
				{
					if (!GMultiByteToWideChar(Utf8, Utf16))
					{
						auto Err = GGetLastError();
						return nullptr;
					}

					bHasUTF16 = true;
				}

				return Utf16;
			}

			inline const size_t GetUtf8Size() const noexcept
			{
				if (!bHasSource)
				{
					return 0;
				}

				return strnlen_s(Utf8, CUtf8Size);
			}
			inline const size_t GetUtf16Size() const noexcept
			{
				if (!bHasSource)
				{
					return 0;
				}

				return wcsnlen_s(Utf16, CUtf16Size);
			}
			inline const size_t GetUtf8Size() noexcept
			{
				if (!bHasSource) { return 0; }

				if (!bHasUTF8)
				{
					return strnlen_s(GetUtf8(), CUtf8Size);
				}

				return strnlen_s(Utf8, CUtf8Size);
			}
			inline const size_t GetUtf16Size() noexcept
			{
				if (!bHasSource) { return 0; }

				if (!bHasUTF16)
				{
					return wcsnlen_s(GetUtf16(), CUtf16Size);
				}

				return wcsnlen_s(Utf16, CUtf16Size);
			}

			inline bool operator==(const char* Utf8Str)  noexcept
			{
				if (!bHasSource)
				{
					return false;
				}

				if (!bHasUTF8)
				{
					return strncmp(GetUtf8(), Utf8Str, CUtf8Size) == 0;
				}

				return strncmp(Utf8, Utf8Str, CUtf8Size) == 0;
			}
			inline bool operator==(const wchar_t* Utf16Str)  noexcept
			{
				if (!bHasSource)
				{
					return false;
				}

				if (!bHasUTF16)
				{
					return wcsncmp(GetUtf16(), Utf16Str, CUtf16Size) == 0;
				}

				return wcsncmp(Utf16, Utf16Str, CUtf16Size) == 0;
			}
			inline bool operator!=(const char* Utf8Str)  noexcept
			{
				return !operator==(Utf8Str);
			}
			inline bool operator!=(const wchar_t* Utf16Str)  noexcept
			{
				return !operator==(Utf16Str);
			}

			inline void CopyUtf16Into(wchar_t* TargetBuffer, size_t TargetBufferSizeInWords) noexcept
			{
				wcscpy_s(TargetBuffer, TargetBufferSizeInWords, GetUtf16());
			}
			inline void CopyUtf8Into(char* TargetBuffer, size_t TargetBufferSize) noexcept
			{
				strcpy_s(TargetBuffer, TargetBufferSize, GetUtf8());
			}
		private:

			DbString(const char* Utf8) noexcept : bHasSource(true), bIsUTF8Src(true)
			{
				strcpy_s(this->Utf8, Utf8);
				Utf16[0] = L'\0';
			}
			DbString(const wchar_t* Utf16) noexcept : bHasSource(true), bIsUTF8Src(false)
			{
				wcscpy_s(this->Utf16, Utf16);
				Utf8[0] = '\0';
			}
		};

		struct DBConnection
		{
			struct DBConnectionSettings
			{
				std::string			username;
				std::string			password;
				std::string			database;
				std::string			host;
				ulong_t				port = 0;
				bool				enableMultiStatements = false;

				DBConnectionSettings() {}

				//Can move
				DBConnectionSettings(DBConnectionSettings&& other) noexcept
				{
					username = std::move(other.username);
					password = std::move(other.password);
					host = std::move(other.host);
					database = std::move(other.database);
					port = other.port;
					enableMultiStatements = other.enableMultiStatements;
				}
				DBConnectionSettings& operator=(DBConnectionSettings&& other) noexcept
				{
					if (this == &other) { return *this; }

					username = std::move(other.username);
					password = std::move(other.password);
					host = std::move(other.host);
					database = std::move(other.database);
					port = other.port;
					enableMultiStatements = other.enableMultiStatements;

					return *this;
				}

				//Cant copy
				DBConnectionSettings(const DBConnectionSettings& other) = delete;
				DBConnectionSettings& operator=(const DBConnectionSettings& other) = delete;
			};
			struct DBLibGuard
			{
				DBLibGuard();
				~DBLibGuard();

				bool IsValidLib() const noexcept { return IsValid; }

			private:
				bool IsValid = false;
			} static inline LibGuard;

			DBConnection()
			{
				memset(&Mysql, 0, sizeof(MYSQL));
			}
			DBConnection(DBConnection&& other) noexcept
			{
				Mysql = std::move(other.Mysql);
			}
			DBConnection& operator=(DBConnection&& other) noexcept
			{
				if (this == &other) { return *this; }
				Mysql = std::move(other.Mysql);
				return *this;
			}

			DBConnection(const DBConnection& other) = delete;
			DBConnection& operator=(const DBConnection& other) = delete;

			~DBConnection();

			static RStatus InitializeAdaptor(DBConnectionSettings&& settings)noexcept;
			static bool IsAdaptorInitialized()noexcept
			{
				return bIsAdaptorInitialized;
			}

			RStatus Initialize() noexcept;

			RStatus TryOpenConnection()noexcept;
			void TryCloseConnection()noexcept;

			inline bool IsOpen() const noexcept
			{
				return (bIsOpen = (mysql_ping(&Mysql) == 0));
			}

			const char* GetStatus() noexcept
			{
				return mysql_stat(&Mysql);
			}
			const char* GetLastMysqlError() noexcept
			{
				return mysql_error(&Mysql);
			}

			bool HasTransaction() const noexcept { return bIsTransactionStarted; }
			bool StartTransaction() noexcept
			{
				if (bIsTransactionStarted)
				{
					return false;
				}

				if (!ExecuteUpdateQuery("START TRANSACTION"))
				{
					return false;
				}

				bIsTransactionStarted = true;

				return true;
			}
			bool RollbackTransaction() noexcept
			{
				if (!bIsTransactionStarted)
				{
					return false;
				}

				if (!ExecuteUpdateQuery("ROLLBACK"))
				{
					return false;
				}

				bIsTransactionStarted = false;

				return true;
			}
			bool CommitTransaction() noexcept
			{
				if (!bIsTransactionStarted)
				{
					return false;
				}

				if (!ExecuteUpdateQuery("COMMIT"))
				{
					return false;
				}

				bIsTransactionStarted = false;

				return true;
			}

			bool ExecuteUpdateQuery(const char* query) noexcept
			{
				if (mysql_query(&Mysql, query))
				{
					LogWarning("DBConnection::StartTransaction() -> Failed! MysqlError[{}]", GetLastMysqlError());
					return false;
				}

				return true;
			}
		private:
			bool TryReacquireConnection(const int32_t Tries = CDBReacquireConnectionTries) noexcept;

			bool SetOptions(UINT ConnectionTimeout = 0)noexcept;

			mutable MYSQL								Mysql;
			mutable bool								bIsOpen = false;
			bool										bIsTransactionStarted = false;

		private:
			static inline DBConnectionSettings			Settings;
			static inline bool							bIsAdaptorInitialized = false;

			friend struct DBStatement;
		};

		struct DBStatement
		{
			struct DBStatementParam
			{
				inline void Reset(void* buffer, ulong_t buffer_length) noexcept
				{
					memset(&bind, 0, sizeof(MYSQL_BIND));

					bind.buffer = buffer;
					bind.buffer_length = buffer_length;
					bind.is_null_value = buffer == nullptr;
				}
				inline void SetType(const enum enum_field_types Type, const bool IsUnsigned = false)noexcept
				{
					bind.buffer_type = Type;
					bind.is_unsigned = IsUnsigned;
				}

				DBStatementParam(DBStatementParam&& other) = delete;
				DBStatementParam(const DBStatementParam& other) = delete;
				DBStatementParam& operator=(const DBStatementParam& other) = delete;
				DBStatementParam& operator=(DBStatementParam&& other) = delete;

			private:
				DBStatementParam() {}

				MYSQL_BIND	bind;

				friend struct DBStatement;
			};
			struct DBStatementResult
			{

				bool IsEmpty() const noexcept
				{
					return noOfRows == 0;
				}

				bool IsValid() const noexcept
				{
					return stmt != nullptr;
				}

				ulong_t GetNoOfRows() const noexcept
				{
					return noOfRows;
				}

				~DBStatementResult() noexcept
				{
					if (resultMetadata)
					{
						mysql_free_result(resultMetadata);
						resultMetadata = nullptr;
					}
				}

				template<typename T>
				inline bool Get(int32_t Index, T* Val)noexcept
				{
					if (!DBStatement::BindImpl((DBStatementParam*)&getBind, Val))
					{
						return false;
					}

					int32_t Result = mysql_stmt_fetch_column(stmt->stmt, &getBind, Index - 1, 0);
					if (Result)
					{

#if DEBUG_MYSQL
						auto MysqlStmtError = mysql_stmt_error(stmt->stmt);

						LogWarning("MYSQL-ERROR(DBStatementResult::Get())::[{}]::{}", Result, MysqlStmtError);
#endif

						return false;
					}

					return true;
				}

				template<typename CharType>
				inline bool GetString(int32_t Index, const CharType* string, ulong_t length = 0)noexcept
				{
					if (!DBStatement::BindStringImpl((DBStatementParam*)&getBind, string, length))
					{
						return false;
					}

					int32_t Result = mysql_stmt_fetch_column(stmt->stmt, &getBind, Index - 1, 0);
					if (Result)
					{

#if DEBUG_MYSQL
						auto MysqlStmtError = mysql_stmt_error(stmt->stmt);

						LogWarning("MYSQL-ERROR(DBStatementResult::GetString())::[{}]::{}", Result, MysqlStmtError);
#endif

						return false;
					}

					return true;
				}

				template<typename T>
				inline bool Bind(int32_t Index, T* Val)noexcept
				{
					return stmt->BindOutput(Index, Val);
				}

				template<size_t Size>
				inline bool BindString(int32_t Index, DbString<Size>& Value)noexcept
				{
					return stmt->BindOutputString(Index, Value);
				}

				inline bool Next() noexcept
				{
					auto Status = mysql_stmt_fetch(stmt->stmt);
					if (Status == 1)
					{

						return false;
					}

					return Status != MYSQL_NO_DATA;
				}

				inline bool GetOneResult() noexcept
				{
					if (!PrepareResult())
					{
						return false;
					}

					return Next();
				}

				inline bool PrepareResult() noexcept
				{
					if (mysql_stmt_bind_result(stmt->stmt, (MYSQL_BIND*)stmt->output))
					{

#if DEBUG_MYSQL
						auto MysqlStmtError = mysql_stmt_error(stmt->stmt);

						LogWarning("MYSQL-ERROR(DBStatementResult::PrepareResult())::{}", MysqlStmtError);
#endif
						return false;
					}

					return true;
				}

			private:
				DBStatementResult()
				{
					ZeroMemory(&getBind, sizeof(getBind));
				}
				DBStatementResult(ulong_t noOfRows, DBStatement* stmt, MYSQL_RES* resultMetadata)
					: noOfRows(noOfRows), stmt(stmt), resultMetadata(resultMetadata)
				{
					ZeroMemory(&getBind, sizeof(getBind));
				}

				DBStatementResult(DBStatementResult&& other) noexcept : noOfRows(other.noOfRows), stmt(other.stmt), resultMetadata(other.resultMetadata)
				{
					other.stmt = nullptr;
					other.resultMetadata = nullptr;
					other.noOfRows = 0;
					ZeroMemory(&getBind, sizeof(getBind));
				}
				DBStatementResult& operator=(DBStatementResult&& other) noexcept
				{
					resultMetadata = other.resultMetadata;
					other.resultMetadata = nullptr;

					stmt = other.stmt;
					other.stmt = nullptr;

					noOfRows = other.noOfRows;

					other.noOfRows = 0;
					ZeroMemory(&getBind, sizeof(getBind));
				}

				DBStatementResult(const DBStatementResult& other) = delete;
				DBStatementResult& operator=(const DBStatementResult& other) = delete;

			private:
				MYSQL_BIND							getBind;
				MYSQL_RES* PTR						resultMetadata = nullptr;
				DBStatement* PTR					stmt = nullptr;
				ulong_t								noOfRows = 0;

				friend struct DBStatement;

				static DBStatementResult EmptyResult;
			};

			DBStatement() : queryStringSize(0)
			{
				memset(&stmt, 0, sizeof(MYSQL_STMT));
				memset(input, 0, sizeof(DBStatementParam) * CDBStatementMaxInputParams);
				memset(output, 0, sizeof(DBStatementParam) * CDBStatementMaxOutputParams);
				bConnectionReacquired = false;
				query[0] = 0;

#if DEBUG_MYSQL
				//LogInfo("DBStatement::DBStatement()");
#endif
			}
			~DBStatement()
			{
				if (stmt)
				{
					mysql_stmt_close(stmt);
					stmt = nullptr;
				}

				connection = nullptr;

#if DEBUG_MYSQL
				//LogInfo("DBStatement::~DBStatement()");
#endif
			}

			DBStatement(DBStatement&& other) = delete;
			DBStatement(const DBStatement& other) = delete;
			DBStatement& operator=(DBStatement&& other) = delete;
			DBStatement& operator=(const DBStatement& other) = delete;

			template<size_t QueryStringSize>
			void SetQuery(const char(&QueryString)[QueryStringSize])
			{
				if constexpr (QueryStringSize >= CDBStatementQueryMaxSize)
				{
					static_assert(false, "DBStatement::Prepare() -> query to long! max DBStatementQueryMaxSize");
				}

				strcpy_s(this->query, QueryString);

				queryStringSize = QueryStringSize;
			}

			bool Prepare(DBConnection* connection)noexcept
			{
				if (!Initialize(connection))
				{
					return false;
				}

				if (query[0] == 0)
				{
					LogWarning("DbStatement::Prepare() -> query not set!");
					return false;
				}

			DBStatement_PrepareStatement:
				int32_t Result = mysql_stmt_prepare(stmt, query, queryStringSize - 1);
				if (Result)
				{

					if (bConnectionReacquired)
					{
						LogWarning("DBStatment::Prepare() -> failed after connection reqcuired !?");
						return false;
					}

					//Try to reacquire the connection
					if ((Result == CR_SERVER_LOST || Result == CR_SERVER_GONE_ERROR) &&
						connection->TryReacquireConnection())
					{
						bConnectionReacquired = true;
#if DEBUG_MYSQL
						LogInfo("DBStatment::Prepare() -> SQL::Connection reacquired!");
#endif
						goto DBStatement_PrepareStatement;
					}

#if DEBUG_MYSQL
					auto MysqlStmtError = mysql_stmt_error(stmt);

					LogWarning("MYSQL-ERROR(DBStatment::Prepare())::[{}]::{}", Result, MysqlStmtError);
#endif

					return false;
				}

				paramsCount = mysql_stmt_param_count(stmt);

				return true;
			}

			bool Reset() noexcept
			{
				inputsBoundCount = 0;
				outputsBoundCount = 0;
				//return mysql_stmt_reset(stmt);
				return true;
			}

			/// <summary>
			/// Bind (as input for the query) a parameter in query to a memory region (value)
			/// </summary>
			/// <typeparam name="T">The type of the value param</typeparam>
			/// <param name="Index">1-based index of param to bind</param>
			/// <param name="Val">Value param</param>
			/// <returns></returns>
			template<typename T, bool IsInput = true>
			bool Bind(int32_t Index, T* Val)noexcept
			{

				DBStatementParam* param = nullptr;

				if constexpr (IsInput)
				{
					if (Index > CDBStatementMaxInputParams)
					{
#if _DEBUG
						LogWarning("DBStatement::Bind()-> reached max input params");
#endif
						return false;
					}

					inputLengths[Index - 1] = sizeof(T);
					param = &input[Index - 1];

					inputsBoundCount++;
				}
				else
				{
					if (Index > CDBStatementMaxOutputParams)
					{
#if _DEBUG
						LogWarning("DBStatement::Bind()-> reached max output params");
#endif
						return false;
					}

					outputLengths[Index - 1] = sizeof(T);
					param = &output[Index - 1];

					outputsBoundCount++;
				}

				return BindImpl(param, Val);
			}

			template<size_t Size, bool IsInput = true>
			bool BindString(int32_t Index, DbString<Size>& Value) noexcept
			{
				DBStatementParam* param = nullptr;

				const char* Utf8Value = Value.GetUtf8();

				if constexpr (IsInput)
				{
					if (Index >= CDBStatementMaxInputParams)
					{
#if _DEBUG
						LogWarning("DBStatement::BindString()-> reached max input params");
#endif
						return false;
					}

					inputLengths[Index - 1] = (ulong_t)Value.GetUtf8Size();
					param = &input[Index - 1];

					inputsBoundCount++;

					return BindStringImpl(param, Utf8Value, &inputLengths[Index - 1]);
				}
				else
				{
					if (Index >= CDBStatementMaxOutputParams)
					{
#if _DEBUG
						LogWarning("DBStatement::BindString()-> reached max output params");
#endif
						return false;
					}

					outputLengths[Index - 1] = Size * 4;;
					param = &output[Index - 1];

					outputsBoundCount++;

					return BindStringImpl(param, Utf8Value, &outputLengths[Index - 1]);
				}
			}

			bool BindBlob(int32_t Index, const char* Buffer, ulong_t BufferSize)noexcept
			{
				if (Index >= CDBStatementMaxInputParams)
				{
#if _DEBUG
					LogWarning("DBStatement::BindBlob()-> reached max input params");
#endif
					return false;
				}

				inputLengths[Index - 1] = BufferSize;
				DBStatementParam* param = &input[Index - 1];

				inputsBoundCount++;

				param->Reset((void*)Buffer, BufferSize);
				param->SetType(MYSQL_TYPE_BLOB);
				param->bind.length = &inputLengths[Index - 1];

				return true;
			}

			template<size_t BufferSize>
			bool BindBlob(int32_t Index, const char(&Buffer)[BufferSize])noexcept
			{
				return BindBlob(Index, Buffer, BufferSize);
			}

			template<typename T>
			bool BindOutput(int32_t Index, T* Val)noexcept
			{
				return Bind<T, false>(Index, Val);
			}

			template<size_t Size>
			bool BindOutputString(int32_t Index, DbString<Size>& Value) noexcept
			{
				return BindString<Size, false>(Index, Value);
			}

			bool ExecuteUpdate()noexcept
			{

				if (inputsBoundCount)
				{
					if (mysql_stmt_bind_param(stmt, (MYSQL_BIND*)input))
					{
#if DEBUG_MYSQL
						LogWarning("DBStatment::ExecuteUpdate() -> Failed to bind input parameters need {}. mysql-error[{}]", paramsCount, mysql_stmt_error(stmt));
#endif
						return false;
					}
				}

				if (outputsBoundCount)
				{
					if (mysql_stmt_bind_result(stmt, (MYSQL_BIND*)output))
					{
#if DEBUG_MYSQL
						LogWarning("DBStatment::ExecuteUpdate() -> Failed to bind output parameters");
#endif
						return false;
					}
				}

			DBStatement_ExecuteStatementUpdate:
				int32_t Result = mysql_stmt_execute(stmt);
				if (Result)
				{
					Result = stmt->last_errno;

					if (bConnectionReacquired)
					{
						LogWarning("DBStatment::ExecuteUpdate() -> failed after connection reqcuired !?");
						return false;
					}

					//Try to reacquire the connection
					if ((Result == CR_SERVER_LOST || Result == CR_SERVER_GONE_ERROR) &&
						connection->TryReacquireConnection())
					{
						bConnectionReacquired = true;
#if DEBUG_MYSQL
						LogInfo("DBStatment::ExecuteUpdate() -> SQL::Connection reacquired!");
#endif
						goto DBStatement_ExecuteStatementUpdate;
					}

#if DEBUG_MYSQL
					auto MysqlStmtError = mysql_stmt_error(stmt);

					LogWarning("MYSQL-ERROR(DBStatment::ExecuteUpdate())::[{}]::{}", Result, MysqlStmtError);
#endif

					return false;
				}

				return mysql_stmt_affected_rows(stmt);
			}
			DBStatementResult Execute()noexcept
			{
				if (inputsBoundCount)
				{
					if (mysql_stmt_bind_param(stmt, (MYSQL_BIND*)input))
					{
#if DEBUG_MYSQL
						LogWarning("DBStatment::ExecuteUpdate() -> Failed to bind input parameters need {}. mysql-error[{}]", paramsCount, mysql_stmt_error(stmt));
#endif
						return std::move(DBStatementResult::EmptyResult);
					}
				}

				if (outputsBoundCount)
				{
					if (mysql_stmt_bind_result(stmt, (MYSQL_BIND*)output))
					{
#if DEBUG_MYSQL
						LogWarning("DBStatment::ExecuteUpdate() -> Failed to bind output parameters");
#endif
						return std::move(DBStatementResult::EmptyResult);
					}
				}

			DBStatement_ExecuteStatement:
				int32_t Result = mysql_stmt_execute(stmt);
				if (Result)
				{

					if (bConnectionReacquired)
					{
						LogWarning("DBStatment::Execute() -> failed after connection reqcuired !?");
						return std::move(DBStatementResult::EmptyResult);
					}

					//Try to reacquire the connection
					if ((Result == CR_SERVER_LOST || Result == CR_SERVER_GONE_ERROR) &&
						connection->TryReacquireConnection())
					{
						bConnectionReacquired = true;
#if DEBUG_MYSQL
						LogInfo("DBStatment::Execute() -> SQL::Connection reacquired!");
#endif
						goto DBStatement_ExecuteStatement;
					}

#if DEBUG_MYSQL
					auto MysqlStmtError = mysql_stmt_error(stmt);

					LogWarning("MYSQL-ERROR(DBStatment::Execute())::[{}]::{}", Result, MysqlStmtError);
#endif

					return std::move(DBStatementResult::EmptyResult);
				}

				MYSQL_RES* resultMetadata = mysql_stmt_result_metadata(stmt);

				Result = mysql_stmt_store_result(stmt);
				if (Result)
				{

#if DEBUG_MYSQL
					auto MysqlStmtError = mysql_stmt_error(stmt);

					LogWarning("MYSQL-ERROR(DBStatment::Execute())::[{}]::{}", Result, MysqlStmtError);
#endif

					return std::move(DBStatementResult::EmptyResult);
				}

				return DBStatementResult((ulong_t)mysql_stmt_num_rows(stmt), this, resultMetadata);
			}

			const char* GetLastStatementError()noexcept
			{
				return mysql_stmt_error(stmt);
			}

			template<typename T>
			static inline bool BindImpl(DBStatementParam* param, T* Val) noexcept
			{
				using BindType = typename std::remove_const<typename std::remove_reference<T>::type>::type;

				param->Reset((void*)Val, sizeof(T));

				if constexpr (
					std::is_same<BindType, int>::value ||
					std::is_same<BindType, long>::value)
				{
					param->SetType(MYSQL_TYPE_LONG);
				}
				else if constexpr (
					std::is_same<BindType, unsigned int>::value ||
					std::is_same<BindType, unsigned long>::value)
				{
					param->SetType(MYSQL_TYPE_LONG, true);
				}
				else if constexpr (
					std::is_same<BindType, short>::value)
				{
					param->SetType(MYSQL_TYPE_SHORT);
				}
				else if constexpr (
					std::is_same<BindType, unsigned short>::value)
				{
					param->SetType(MYSQL_TYPE_SHORT, true);
				}
				else if constexpr (
					std::is_same<BindType, long long>::value)
				{
					param->SetType(MYSQL_TYPE_LONGLONG);
				}
				else if constexpr (
					std::is_same<BindType, unsigned long long>::value)
				{
					param->SetType(MYSQL_TYPE_LONGLONG, true);
				}
				else if constexpr (
					std::is_same<BindType, float>::value)
				{
					param->SetType(MYSQL_TYPE_FLOAT);
				}
				else if constexpr (
					std::is_same<BindType, double>::value)
				{
					param->SetType(MYSQL_TYPE_DOUBLE);
				}
				else if constexpr (
					std::is_same<BindType, bool>::value)
				{
					param->SetType(MYSQL_TYPE_BIT);
				}
				else if constexpr (
					std::is_same<BindType, bool>::value)
				{
					param->SetType(MYSQL_TYPE_BIT);
				}
				else if constexpr (
					std::is_same<BindType, EntityId>::value)
				{
					param->SetType(MYSQL_TYPE_LONGLONG, true);
				}
				else
				{
					static_assert(false, "DBStatement::Bind() -> Unknown bind type");
				}

				return true;
			}

			template<typename CharType>
			static inline bool BindStringImpl(DBStatementParam* param, const CharType* string, ulong_t* length) noexcept
			{
				using BindType = typename std::remove_const<typename std::remove_reference<CharType>::type>::type;

				param->Reset((void*)string, *length);
				param->SetType(MYSQL_TYPE_STRING);
				param->bind.length = length;

				return true;
			}

			inline bool Close()noexcept
			{
				return mysql_stmt_close(stmt);
			}

		private:
			bool Initialize(DBConnection* connection) noexcept
			{
				bConnectionReacquired = false;

				stmt = mysql_stmt_init(&connection->Mysql);
				if (!stmt)
				{
#if DEBUG_MYSQL
					LogWarning("MYSQL-ERROR::mysql_stmt_init() -> nullptr");
#endif

					return false;
				}

				this->connection = connection;

				return true;
			}

			DBConnection* PTR		connection = nullptr;
			MYSQL_STMT* PTR			stmt = nullptr;
			DBStatementParam		input[CDBStatementMaxInputParams];
			DBStatementParam		output[CDBStatementMaxOutputParams];
			ulong_t					inputLengths[CDBStatementMaxOutputParams];
			ulong_t					outputLengths[CDBStatementMaxOutputParams];
			char					query[CDBStatementQueryMaxSize];
			ulong_t					queryStringSize = 0;
			bool					bConnectionReacquired = false;
			ulong_t					paramsCount = 0;

			ulong_t					inputsBoundCount = 0;
			ulong_t					outputsBoundCount = 0;
		};

		struct DBTransaction
		{

			DBTransaction(DBConnection* connection)noexcept : connection(connection)
			{
				bIsValid = connection->StartTransaction();
			}
			~DBTransaction()
			{
				RollbackTransaction();
			}

			DBTransaction(const DBTransaction&) = delete;
			DBTransaction(DBTransaction&&) = delete;
			DBTransaction& operator=(const DBTransaction&) = delete;
			DBTransaction& operator=(DBTransaction&&) = delete;

			bool CommitTransaction()noexcept
			{
				if (!bIsValid)
				{
					return false;
				}

				return connection->CommitTransaction();
			}
			bool RollbackTransaction()noexcept
			{
				if (!bIsValid)
				{
					return false;
				}

				return connection->RollbackTransaction();
			}

			bool IsValid() const noexcept { return bIsValid; }

		private:
			bool				bExitScope = false;
			bool				bIsValid = false;

			DBConnection* PTR	connection = nullptr;

			friend struct DBTransactionScopeControl;
		};
		struct DBTransactionScopeControl
		{
			DBTransactionScopeControl(DBConnection* connection) : transaction(connection) {}

			DBTransactionScopeControl(const DBTransactionScopeControl&) = delete;
			DBTransactionScopeControl(DBTransactionScopeControl&&) = delete;
			DBTransactionScopeControl& operator=(const DBTransactionScopeControl&) = delete;
			DBTransactionScopeControl& operator=(DBTransactionScopeControl&&) = delete;

			bool EnterScope() const noexcept
			{
				return !transaction.bExitScope;
			}
			void SetExitScope()noexcept
			{
				transaction.bExitScope = true;
			}
			DBTransaction* operator->() noexcept
			{
				return &transaction;
			}

		private:
			DBTransaction transaction;
		};

#define DB_TRANSACTION(connection)																	\
			for (																					\
					DBTransactionScopeControl Transaction = DBTransactionScopeControl(connection);	\
					Transaction.EnterScope();														\
					Transaction.SetExitScope()														\
			    )
	}
}
