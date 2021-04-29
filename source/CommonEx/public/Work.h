#pragma once
/**
 * @file AsyncWork.h
 *
 * @brief CommonEx AsyncWork abstraction
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

namespace CommonEx {
	enum EWorkType : uint32_t
	{
		EWorkType_None,
		EWorkType_Any,
		EWorkType_SendBuffer,
		EWorkType_ReceiveBuffer,
		EWorkType_ClientConnection,

		EWorkType_MAX
	};

	class DummyWorkPayload {};

	class AsyncWorkBase {
	protected:
#if COMMONEX_WIN32_PLATFROM
		OsOverlappedType	WorkOverllapped;
#else 
#error @TODO AsyncWorkBase
#endif
		EWorkType			WorkType{ EWorkType_None };

	public:
		union
		{
			struct
			{
				unsigned	bIsReusable : 1;
				unsigned	bDontDelete : 1;
				unsigned	bDontDeleteData : 1;
				unsigned	bHasCompletionHandler : 1;
				unsigned	bHasPayload : 1;
			};
			uint32_t		WorkFlags{ 0 };
		};

		uint32_t			NoOfBytesTransferred{ 0 };

		FORCEINLINE AsyncWorkBase(EWorkType WorkType) noexcept
			:WorkType(WorkType)
			, WorkFlags(0)
		{
			memset(&WorkOverllapped, 0, sizeof(OsOverlappedType));
		}

	public:
		FORCEINLINE EWorkType GetWorkType() const noexcept
		{
			return WorkType;
		}
		FORCEINLINE uint32_t GetNoOfBytesTransferred() const noexcept { return NoOfBytesTransferred; }
	};

	/*------------------------------------------------------------
		AsyncWork (All work items base)
	  ------------------------------------------------------------*/
	template<typename TSupper, class TPayload = DummyWorkPayload>
	class AsyncWork : public AsyncWorkBase
	{
	public:
		static const bool HasPayload = !std::is_same_v<TPayload, DummyWorkPayload>;

		using MyType = AsyncWork<TPayload, TSupper>;
		using TCompletionHandler = TaskEx<12, void(TSupper*, TPayload*, RStatus)>;

		TCompletionHandler	CompletionHandler;
		TPayload			Payload{};

		FORCEINLINE AsyncWork(EWorkType WorkType) noexcept
			: AsyncWorkBase(WorkType)
		{
			bHasPayload = HasPayload;
			memset(&WorkOverllapped, 0, sizeof(OsOverlappedType));
		}
		FORCEINLINE AsyncWork(const AsyncWork& Other) noexcept
			: AsyncWorkBase(WorkType)
		{
			memset(&WorkOverllapped, 0, sizeof(OsOverlappedType));
		}

		~AsyncWork() noexcept {}

		FORCEINLINE TPayload* GetPayload() noexcept {
			return &Payload;
		}
		FORCEINLINE bool SetCompletionHandler(TCompletionHandler&& Handler) noexcept {
			CompletionHandler = std::move(Handler);

			bHasCompletionHandler = true;

			return true;
		}
		FORCEINLINE void CompleteWork(RStatus Result, uint32_t NoOfBytesTransferred) noexcept
		{
			this->NoOfBytesTransferred = NoOfBytesTransferred;

			CompletionHandler((TSupper*)this, &Payload, Result);
		}

		FORCEINLINE AsyncWork& operator=(const AsyncWork& Other) noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			//WorkOverllapped = Other.WorkOverllapped;
			WorkType = Other.WorkType;
			WorkFlags = Other.WorkFlags;

			return *this;
		}
		FORCEINLINE AsyncWork& operator=(AsyncWork&& Other) noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			//WorkOverllapped = Other.WorkOverllapped;
			WorkType = Other.WorkType;
			WorkFlags = Other.WorkFlags;

			return *this;
		}
	};

	class IWork : public AsyncWork<IWork> {};

	//Any work (execute the completion handler async)
	class AnyWorkAsync : public AsyncWork<AnyWorkAsync> {
	public:
		using Base = AsyncWork<AnyWorkAsync>;

		AnyWorkAsync(Base::TCompletionHandler&& CompletionHandler) noexcept : Base(EWorkType_Any) {
			SetCompletionHandler(std::move(CompletionHandler));
		}
	};
}