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

namespace CommonEx {

	//	Task - task object, can be initialized with a lambda object, supports any size capture pack!
	//		*it allocates memory through the MemoryManager*
	//		* IMPORTANT: Tries to do[small lambda optimization] ie. if the sizeof(Lambda) <= sizeof(void*) it wont allocate memory for the lambda*
	//		* so for all lambda's with no capture or small capture there won't be any memory allocation !*
	//		*TIP : Strive to use Task where most of the lambdas passed in are suitable for[small lambda optimization] *
	template<typename ReturnType, typename ...Args>
	class Task {
		typedef ReturnType(*HandlerType)(Args...) noexcept;

	public:
		~Task() {
			if (Body) {
				if (Body->Destroy) {
					Body->Destroy(Body, false);
				}
				else
				{
					delete Body;
				}
			}
		}

		Task() {}
		template<typename Lambda>
		Task(const Lambda& ByConstRef) noexcept {
			BuildHandler<Lambda>(ByConstRef);
		}

		//Can't copy
		Task(const Task&) = delete;
		Task& operator=(const Task&) = delete;

		//Can move
		Task(Task&& Other) noexcept {
			(*this) = std::move(Other);
		}
		Task& operator=(Task&& Other) noexcept {
			if (this == &Other) {
				return *this;
			}

			Handler = Other.Handler;
			Body = Other.Body;

			Other.Handler = nullptr;
			Other.Body = nullptr;

			return *this;
		}

		template<typename Lambda>
		void operator=(const Lambda& ByConstRef)noexcept {
			BuildHandler<Lambda>(ByConstRef);
		}

		ReturnType operator()(Args... args) noexcept {
			return Handler((ptr_t*)&Body, std::forward<Args...>(args)...);
		}

		ReturnType DoTask(Args... args) noexcept {
			return Handler((ptr_t*)&Body, std::forward<Args...>(args)...);
		}

		bool IsNull() const noexcept {
			return !Handler || !Body;
		}

	private:

		template<typename Lambda>
		void BuildHandler(const Lambda& ByConstRef) noexcept {
			constexpr auto Size = sizeof(Lambda);

			if constexpr (Size <= sizeof(IMemoryBlock*)) { //we have sizeof(IMemoryBlock*) bytes that we can use to store the lambda in
				new ((void*)(&Body)) Lambda(ByConstRef); //construct the lambda into the pointer's memory space
			}
			else {
				Body = MemoryManager::AllocBlock<Lambda>(ByConstRef);
				if (!Body) {
					LogFatal("Task<> Failed to allocate body size({})!", sizeof(Lambda));
					abort(); //to dramatic?
				}
			}

			Handler = &CallStub<Lambda>;
		}

		template<typename Lambda>
		FORCEINLINE static ReturnType CallStub(ptr_t* Body, Args... args)noexcept {
			constexpr auto Size = sizeof(Lambda);
			if constexpr (Size <= sizeof(IMemoryBlock*))
			{
				return ((Lambda*)Body)->operator()(std::forward<Args...>(args)...);
			}
			else {
				return ((Lambda*)*Body)->operator()(std::forward<Args...>(args)...);
			}
		}

	private:
		HandlerType			Handler{ nullptr };
		IMemoryBlock* PTR	Body{ nullptr };
	};

	//	TaskEx - task object, can be initialized with a lambda object, supports specific size capture pack, keeps lambda object flat(inside the TaskEx type)!
	//		*it does not allocate memory but the Lambda capture should fit inside[CTaskExBodySize] bytes ie. sizeof(Lambda) <= CTaskExBodySize *
	//		*if needed adjust CTaskExBodySize to fit the needed type*
	//		* IMPORTANT: KEEP CTaskExBodySize AS LITTLE AS POSSIBLE*
	//		* TIP : Allocate TaskEx thorugh the MemoryManager for best performance yield*
	//
	template<typename ReturnType, typename ...Args>
	class TaskEx : public IResource<TaskEx<ReturnType, Args...>> {
		typedef ReturnType(*HandlerType)(Args...) noexcept;

		using MyType = TaskEx<ReturnType, Args...>;

	public:
		~TaskEx() = delete;

		template<typename Lambda>
		TaskEx(const Lambda& ByConstRef) noexcept {
			BuildHandler<Lambda>(ByConstRef);
		}

		template<typename Lambda>
		void operator=(const Lambda& ByConstRef)noexcept {
			BuildHandler<Lambda>(ByConstRef);
		}

		ReturnType operator()(Args... args) noexcept {
			return Handler((ptr_t)Body, std::forward<Args...>(args)...);
		}

		ReturnType DoTask(Args... args) noexcept {
			return Handler((ptr_t)Body, std::forward<Args...>(args)...);
		}

	private:
		template<typename Lambda>
		void BuildHandler(const Lambda& ByConstRef) noexcept {
			static_assert(sizeof(Lambda) <= CTaskExBodySize, "TaskEx<> Lambda type is bigger than the body, please adjust CTaskExBodySize");

			new (Body) Lambda(ByConstRef);

			Handler = &CallStub<Lambda>;
		}

		template<typename Lambda>
		FORCEINLINE static ReturnType CallStub(ptr_t Body, Args... args)noexcept {
			return ((Lambda*)Body)->operator()(std::forward<Args...>(args)...);
		}

	private:
		HandlerType			Handler{ nullptr };
		uint8_t				Body[CTaskExBodySize];
	};

	//Allocates a new TaskEx<ReturnType, Args...> through the MemoryManager
	template<typename ReturnType, typename ...Args, typename Lambda>
	inline MPtr<TaskEx<ReturnType, Args...>> CreateTaskEx(const Lambda& ByConstRef)noexcept {
		return std::move(TaskEx<ReturnType, Args...>::New(ByConstRef));
	}

	//Allocates a new TaskEx<void> through the MemoryManager
	template<typename Lambda>
	inline MPtr<TaskEx<void>> CreateTaskEx(const Lambda& LambdaByConstRef)noexcept {
		return std::move(TaskEx<void>::New(LambdaByConstRef));
	}
}