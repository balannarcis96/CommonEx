#pragma once
/**
 * @file Task.h
 *
 * @brief CommonEx task abstractions
 *			Task	- task object, can be initialized with a lambda object, supports any size capture pack!
 *						* it allocates memory through the MemoryManager *
 *						* IMPORTANT: Tries to do [small lambda optimization] ie. if the sizeof(Lambda) <= sizeof(void*) it wont allocate memory for the lambda *
 *						*				so for all lambda's with no capture or small capture there won't be any memory allocation !					 *
 *						* TIP: Strive to use Task where most of the lambdas passed in are suitable for [small lambda optimization] *
 *
 *			TaskEx	- task object, can be initialized with a lambda object, supports specific size capture pack, keeps lambda object flat(inside the TaskEx type)!
 *						* it does not allocate memory but the Lambda capture should fit inside [CTaskExBodySize] bytes ie. sizeof(Lambda) <= CTaskExBodySize *
 *						* if needed adjust CTaskExBodySize to fit the needed type *
 *						* IMPORTANT: KEEP CTaskExBodySize AS LITTLE AS POSSIBLE *
 *						* TIP: Allocate TaskEx thorugh the MemoryManager for best performance yield *
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

namespace CommonEx
{
	class MemoryBlockBaseResource;

	template<typename T> class Task;
	template<size_t BodySize, typename T> class _TaskEx;

	/*
		Task - task object, can be initialized with a lambda object, supports any size capture pack!
		@info: It allocates memory through the MemoryManager
		@important: Tries to do[small lambda optimization] ie. if the sizeof(Lambda) <= sizeof(void*) it wont allocate memory for the lambda; so for all lambda's with no capture or small capture block, there won't be any memory allocation done!
		@tip: Strive to use Task where most of the lambdas passed in are suitable for[small lambda optimization]
	*/

	class TaskBase
	{
	protected:
		FORCEINLINE ~TaskBase() noexcept
		{
			Clear();
		}

		FORCEINLINE TaskBase() noexcept {}

		//Can copy (duplicate)
		FORCEINLINE TaskBase(const TaskBase& Other) noexcept
		{
			(*this) = Other;
		}
		FORCEINLINE TaskBase& operator=(const TaskBase& Other)noexcept;

		//Can move
		FORCEINLINE TaskBase(TaskBase&& Other) noexcept
		{
			(*this) = std::move(Other);
		}
		FORCEINLINE TaskBase& operator=(TaskBase&& Other)noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			Clear();

			Handler = Other.Handler;
			DestroyStub = Other.DestroyStub;
			Body = Other.Body;

			Other.Release();

			return *this;
		}

		FORCEINLINE void Clear() noexcept;

		FORCEINLINE bool IsNull() const noexcept
		{
			return !Handler || !Body;
		}

		FORCEINLINE explicit operator bool() const noexcept
		{
			return !IsNull();
		}

		FORCEINLINE void Release() noexcept
		{
			Handler = nullptr;
			DestroyStub = nullptr;
			Body = nullptr;
		}

		template<typename Lambda, typename ReturnType, typename ...Args>
		FORCEINLINE void BuildHandler(const Lambda& ByConstRef) noexcept;

		template<typename Lambda, typename ReturnType, typename ...Args>
		FORCEINLINE static ReturnType CallStub(ptr_t* Body, Args... args)noexcept;

		template<typename ReturnType, typename ...Args>
		FORCEINLINE ReturnType Call(Args... args) noexcept
		{
			using HandlerType = ReturnType(ptr_t**, Args...) noexcept;

			return ((HandlerType*)Handler)((ptr_t**)Body, std::forward<Args>(args)...);
		}

		template<typename Lambda>
		FORCEINLINE static void DestroyStubFunction(ptr_t ptr)noexcept
		{
			if constexpr (std::is_destructible_v<Lambda>)
			{
				((Lambda*)Body)->~Lambda();
			}
		}

		ptr_t							Handler{ nullptr };
		ptr_t							DestroyStub{ nullptr };
		MemoryBlockBaseResource* PTR	Body{ nullptr };
	};

	template<typename ReturnType, typename ...Args>
	class Task<ReturnType(Args...)> : public TaskBase
	{
		typedef ReturnType(*HandlerType)(ptr_t**, Args...) noexcept;

	public:
		FORCEINLINE Task()noexcept : TaskBase() {}

		template<typename Lambda>
		FORCEINLINE Task(const Lambda& ByConstRef) noexcept : TaskBase()
		{
			this->BuildHandler<Lambda, ReturnType, Args...>(ByConstRef);
		}

		//Can copy (duplicate)
		FORCEINLINE Task(const Task& Other) noexcept : TaskBase(Other) {}
		FORCEINLINE Task& operator=(const Task& Other) noexcept
		{
			return (Task&)TaskBase::operator=(Other);
		}

		//Can move
		FORCEINLINE Task(Task&& Other) noexcept :TaskBase(std::move(Other)) {}
		FORCEINLINE Task& operator=(Task&& Other) noexcept
		{
			return (Task&)TaskBase::operator=(std::move(Other));
		}

		FORCEINLINE ~Task() noexcept
		{
			TaskBase::~TaskBase();
		}

		template<typename Lambda>
		FORCEINLINE void operator=(const Lambda& ByConstRef)noexcept
		{
			Clear();
			this->BuildHandler<Lambda, ReturnType, Args...>(ByConstRef);
		}

		FORCEINLINE ReturnType operator()(Args... args) noexcept
		{
			return this->Call<ReturnType, Args...>(std::forward<Args>(args)...);
		}

		FORCEINLINE ReturnType DoTask(Args... args) noexcept
		{
			return this->Call<ReturnType, Args...>(std::forward<Args>(args)...);
		}
	};

	/*
		TaskEx - task object, can be initialized with a lambda object, supports specific size capture pack, keeps lambda object flat(inside the TaskEx type)!

		-info:		It does not allocate memory but the Lambda capture should fit inside[BodySize] bytes ie. sizeof(Lambda) <= BodySize
		-info:		If needed adjust BodySize to fit the needed max lambda capture size for the usage
		-tip:		Allocate TaskEx thorugh the MemoryManager for best performance yield
		-important: KEEP BodySize AS LITTLE AS POSSIBLE
	*/
	template<size_t BodySize, typename ReturnType, typename ...Args>
	class _TaskEx<BodySize, ReturnType(Args...)>
	{
		typedef ReturnType(*HandlerType)(ptr_t, Args...) noexcept;
		using MyType = _TaskEx<BodySize, ReturnType(Args...)>;

	public:
		~_TaskEx() noexcept
		{
			Clear();
		}

		_TaskEx() noexcept {}

		//Duplicate Task (be careful, may produce shared state)
		FORCEINLINE _TaskEx(const MyType& Other) noexcept
		{
			(*this) = Other;
		}
		FORCEINLINE _TaskEx& operator=(const MyType& Other) noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			Mirror(Other);

			return *this;
		}

		FORCEINLINE _TaskEx(MyType&& Other) noexcept
		{
			(*this) = std::move(Other);
		}
		FORCEINLINE _TaskEx& operator=(MyType&& Other) noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			if (!Other)
			{
				Clear();
				return *this;
			}

			Handler = Other.Handler;
			DestroyStub = Other.DestroyStub;
			CopyStub = Other.CopyStub;

			if (memcpy_s(
				Body,
				BodySize,
				Other.Body,
				BodySize
			))
			{
				LogFatal("_TaskEx::operator=(_Task&&) Failed to mempcy_s!");
				abort();
			}

			//not necessary
			//Other.ZeroBody();
			Other.Handler = nullptr;
			Other.DestroyStub = nullptr;

			return *this;
		}

		template<typename TLambda>
		FORCEINLINE _TaskEx(MyType&& Other) noexcept
		{
			this->operator=<TLambda>(Other);
		}

		template<typename Lambda>
		FORCEINLINE _TaskEx(Lambda&& Other) noexcept
		{
			static_assert(sizeof(Lambda) <= BodySize, "Lambda cannot fit into this task, please resize the task or the lambda capture scope");

			this->BuildHandler<Lambda>(std::move(Other));
		}

		template<typename TLambda, bool bDontCheckBodySize = false>
		FORCEINLINE _TaskEx& operator=(TLambda&& Lambda)noexcept
		{
			if constexpr (std::is_same_v<MyType, std::decay_t<TLambda>>)
			{
				return (*this) = std::move(Lambda);
			}
			else if constexpr (std::is_lambda_f<std::decay_t<TLambda>, ReturnType, Args...>::value)
			{
				if constexpr (!bDontCheckBodySize)
				{
					static_assert(sizeof(TLambda) <= BodySize, "Lambda cannot fit into this task, please resize the task or the lambda capture scope");
				}

				Clear();
				this->BuildHandler<TLambda>(std::move(Lambda));
			}
			else
			{
				static_assert(false, "Pass a functor or same type to this operator!");
			}

			return *this;
		}

		FORCEINLINE ReturnType operator()(Args... args) noexcept
		{
			return Handler(GetBody(), std::forward<Args>(args)...);
		}

		FORCEINLINE ReturnType DoTask(Args... args) noexcept
		{
			return Handler(GetBody(), std::forward<Args>(args)...);
		}

		FORCEINLINE bool IsNull() const noexcept
		{
			return !Handler;
		}

		FORCEINLINE explicit operator bool() const noexcept
		{
			return !IsNull();
		}

		FORCEINLINE void Clear() noexcept
		{
			if (IsNull())
			{
				return;
			}

			typedef void(*DestroyStubType)(ptr_t) noexcept;

			if (DestroyStub)
			{
				//Destroy the lambda
				((DestroyStubType)DestroyStub)(GetBody());
			}

			DestroyStub = nullptr;
			Handler = nullptr;
		}

		FORCEINLINE void Mirror(const MyType& Other)noexcept
		{
			Handler = Other.Handler;
			DestroyStub = Other.DestroyStub;
			CopyStub = Other.CopyStub;

			if (CopyStub)
			{
				typedef void(*CopyStubType)(ptr_t, const ptr_t) noexcept;

				((CopyStubType)CopyStub)(GetBody(), Other.GetBody());
			}
			else
			{
				if (memcpy_s(
					Body,
					BodySize,
					Other.Body,
					BodySize
				))
				{
					LogFatal("_TaskEx::operator=(const _Task&) Failed to mempcy_s!");
					abort();
				}
			}
		}

	private:
		template<typename Lambda>
		FORCEINLINE void BuildHandler(const Lambda& ByConstRef) noexcept
		{
			static_assert(sizeof(Lambda) <= CTaskExBodySize, "TaskEx<> Lambda type is bigger than the body, please adjust CTaskExBodySize");

			if constexpr (std::is_nothrow_destructible_v<Lambda>)
			{
				DestroyStub = (ptr_t)&DestroyStubFunction<Lambda>;
			}
			else if constexpr (std::is_destructible_v<Lambda>)
			{
				static_assert(false, "_TaskEx::BuildHandler<Lambda>() Lambda must be nothrow destructible");
			}
			else
			{
				DestroyStub = nullptr;
			}

			CopyStub = (ptr_t)&CopyStubFunction<Lambda>;

			Handler = &CallStub<Lambda>;

			new (GetBody()) Lambda(ByConstRef);
		}

		template<typename Lambda>
		FORCEINLINE void BuildHandler(Lambda&& ByRefRef) noexcept
		{
			static_assert(sizeof(Lambda) <= CTaskExBodySize, "TaskEx<> Lambda type is bigger than the body, please adjust CTaskExBodySize");

			if constexpr (std::is_nothrow_destructible_v<Lambda>)
			{
				DestroyStub = (ptr_t)&DestroyStubFunction<Lambda>;
			}
			else if constexpr (std::is_destructible_v<Lambda>)
			{
				static_assert(false, "_TaskEx::BuildHandler<Lambda>() Lambda must be nothrow destructible");
			}
			else
			{
				DestroyStub = nullptr;
			}

			CopyStub = (ptr_t)&CopyStubFunction<Lambda>;

			Handler = &CallStub<Lambda>;

			ZeroBody();
			new (GetBody()) Lambda(std::move(ByRefRef));
		}

		template<typename Lambda>
		FORCEINLINE static ReturnType CallStub(ptr_t Body, Args... args)noexcept
		{
			return ((Lambda*)Body)->operator()(std::forward<Args>(args)...);
		}

		template<typename Lambda>
		FORCEINLINE static void DestroyStubFunction(ptr_t Body)noexcept
		{
			((Lambda*)Body)->~Lambda();
		}

		template<typename Lambda>
		FORCEINLINE static void CopyStubFunction(ptr_t Body, const ptr_t SourceBody)noexcept
		{
			((Lambda*)Body)->~Lambda();

			new (Body) Lambda(*((const Lambda*)SourceBody));
		}

	private:
		FORCEINLINE ptr_t GetBody() noexcept
		{
			return (ptr_t)(Body);
		}

		FORCEINLINE const ptr_t GetBody() const noexcept
		{
			return (ptr_t)(Body);
		}

		FORCEINLINE void ZeroBody() noexcept
		{
			memset(Body, 0, BodySize);
		}

		HandlerType			Handler{ nullptr };
		ptr_t				DestroyStub{ nullptr };
		ptr_t				CopyStub{ nullptr };

		//Body must be last!
		uint8_t				Body[BodySize]{ 0 };
	};

	template<typename FunctionType>
	using TaskEx = _TaskEx<CTaskExBodySize, FunctionType>;
}