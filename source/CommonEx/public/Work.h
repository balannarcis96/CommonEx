#pragma once

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

	struct DummyWorkPayload {};

	struct WorkBase {
	protected:
		OsOverlappedType	WorkOverllapped{};
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

	protected:
		WorkBase(EWorkType WorkType)
			:WorkType(WorkType)
			, WorkFlags(0)
		{
			memset(&WorkOverllapped, 0, sizeof(OsOverlappedType));
		}

	public:
		inline EWorkType GetWorkType() const noexcept
		{
			return WorkType;
		}
		inline uint32_t GetNoOfBytesTransferred() const noexcept { return NoOfBytesTransferred; }
	};

	/*------------------------------------------------------------
		Work (All work items base)
	  ------------------------------------------------------------*/
	template<typename TPayload, typename TSupper>
	struct Work : WorkBase
	{
		static const bool HasPayload = !std::is_same_v<TPayload, DummyWorkPayload>;

		using MyType = Work<TPayload, TSupper>;
		using CompletionHandler = Delegate<void, TSupper*, TPayload*, RStatus>;

		CompletionHandler	CompleteHandler;
		TPayload			Payload{};

	public:
		Work(EWorkType WorkType)
			: WorkBase(WorkType)
		{
			bHasPayload = HasPayload;
			memset(&WorkOverllapped, 0, sizeof(OsOverlappedType));
		}
		Work(const Work& Other)
			: WorkBase(WorkType)
		{
			memset(&WorkOverllapped, 0, sizeof(OsOverlappedType));
		}

		inline TPayload* GetPayload() {
			return &Payload;
		}
		inline bool SetCompletionHandler(CompletionHandler&& Handler) noexcept {
			CompleteHandler = std::move(Handler);

			bHasCompletionHandler = true;

			return true;
		}
		inline void CompleteWork(RStatus Result, uint32_t NoOfBytesTransferred) noexcept
		{
			this->NoOfBytesTransferred = NoOfBytesTransferred;

			CompleteHandler((TSupper*)this, &Payload, Result);
		}

		inline Work& operator=(const Work& Other) noexcept
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
		inline Work& operator=(Work&& Other) noexcept
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

	struct IWork : Work<DummyWorkPayload, IWork> {};

	//Any work, (execute the completion handler async)
	struct AnyWork : Work<DummyWorkPayload, IWork> {
		using Base = Work<DummyWorkPayload, IWork>;

		AnyWork(Base::CompletionHandler&& CompletionHandler) : Work(EWorkType_Any) {
			SetCompletionHandler(std::move(CompletionHandler));
		}
	};
}