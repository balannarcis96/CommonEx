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
	template<size_t BodySize, typename T> class TaskEx;

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
	class TaskEx<BodySize, ReturnType(Args...)>
	{
		typedef ReturnType(*HandlerType)(ptr_t, Args...) noexcept;
		using MyType = TaskEx<BodySize, ReturnType(Args...)>;

	public:
		FORCEINLINE ~TaskEx() noexcept
		{
			Clear();
		}

		FORCEINLINE TaskEx() noexcept {}

		template<typename Lambda>
		FORCEINLINE TaskEx(const Lambda& ByConstRef) noexcept
		{
			BuildHandler<Lambda>(ByConstRef);
		}

		template<typename Lambda>
		FORCEINLINE void operator=(const Lambda& ByConstRef)noexcept
		{
			Clear();
			BuildHandler<Lambda>(ByConstRef);
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

			Handler = nullptr;
		}

	private:
		template<typename Lambda>
		FORCEINLINE void BuildHandler(const Lambda& ByConstRef) noexcept
		{
			static_assert(sizeof(Lambda) <= CTaskExBodySize, "TaskEx<> Lambda type is bigger than the body, please adjust CTaskExBodySize");

			if constexpr (std::is_destructible_v<Lambda>)
			{
				DestroyStub = (ptr_t)&DestroyStubFunction<Lambda>;
			}
			else
			{
				DestroyStub = nullptr;
			}

			Handler = &CallStub<Lambda>;

			new (GetBody()) Lambda(ByConstRef);
		}

		template<typename Lambda>
		FORCEINLINE static ReturnType CallStub(ptr_t Body, Args... args)noexcept
		{
			return ((Lambda*)Body)->operator()(std::forward<Args>(args)...);
		}

		template<typename Lambda>
		FORCEINLINE static void DestroyStubFunction(ptr_t Body)noexcept
		{
			if constexpr (std::is_destructible_v<Lambda>)
			{
				((Lambda*)Body)->~Lambda();
			}
		}

	private:
		FORCEINLINE constexpr ptr_t GetBody() noexcept
		{
			return (ptr_t)(Body);
		}

		HandlerType			Handler{ nullptr };
		ptr_t				DestroyStub{ nullptr };
		uint8_t				Body[BodySize];
	};
}