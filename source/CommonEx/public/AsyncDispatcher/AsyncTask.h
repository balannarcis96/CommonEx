#pragma once

namespace CommonEx
{
	namespace Async
	{
		using TimeDiff = int64_t;

		constexpr auto CTaskBaseMaxSize = 24;

		class AsyncTaskBase :public NotSharedMemoryResourceBase
		{
		public:
			using TExecuteTask = _TaskEx<CTaskBaseMaxSize, void(void)>;

			AsyncTaskBase() noexcept {}

			FORCEINLINE void ExecuteTask() noexcept
			{
				OnExecute();
			}

			//For intrusive singly linked list
			AsyncTaskBase* volatile		Next{ nullptr };

			//Main task
			TExecuteTask				OnExecute{};

			friend class TaskQueue;
		};

		template<int64_t BodySize = 8>
		class AsyncTask : public AsyncTaskBase
		{
		private:
			//! DONT TOUCH !
			//! The task (OnExecute) in AsyncTaskBase has body size CTaskBaseMaxSize,
			//!		if the Lambda given doesnt fit into that task (OnExecute), we "extend" here by 
			//!		having BodyExtension right after the [OnExecute] so when the lambda is 
			//!		copied into OnExecute the "excess" will fall into the [BodyExtension].
			//! This will generate an warning (C4789)
			static const auto BodyExtensionSize = CMax((int64_t)1, (BodySize - (int64_t)CTaskBaseMaxSize));
			uint8_t BodyExtension[BodyExtensionSize];

		public:
			template<typename TLambda>
			AsyncTask(TLambda&& Task)noexcept :AsyncTaskBase()
			{
				static_assert(sizeof(TLambda) <= BodySize + CTaskBaseMaxSize, "Lambda can not fit into this task, please resize the body!");

				this->OnExecute.operator=<TLambda, true>(std::move(Task));
			}
		};

		template<class TInstance, typename... TArgsPack>
		class AsyncMemberFunctionTask : public AsyncTaskBase
		{
		public:
			typedef void(TInstance::* TMemberFunction)(TArgsPack...);
			using TArgs = std::tuple<TInstance*, TArgsPack...>;

			AsyncMemberFunctionTask(TInstance* Object, TMemberFunction MemberFunction, TArgsPack... CallArgs) noexcept
				: AsyncTaskBase()
				, Args(Object, std::forward<TArgsPack>(CallArgs)...)
			{
				auto Lambda = [this, MemberFunction]()
				{
					std::apply(MemberFunction, this->Args);
				};

				this->OnExecute = std::move(Lambda);
			}

			TArgs Args;
		};

		class AsyncTimerTask
		{
		public:
			~AsyncTimerTask() noexcept
			{
				Reset();
			}

			AsyncTimerTask(TimeDiff Time, AsyncTaskBase* Task, AsyncDispatcher* Origin) noexcept
				:Time(Time)
				, Task(Task)
				, Origin(Origin)
			{
#if _DEBUG
				assert(Task && Origin);
#endif
			}

			AsyncTimerTask(AsyncTimerTask&& Other) noexcept
				:Time(Other.Time)
				, Task(Other.Task)
				, Origin(Other.Origin)
			{
				Other.Task = nullptr;
				Other.Time = -1;
				Other.Origin = nullptr;
			}
			AsyncTimerTask& operator=(AsyncTimerTask&& Other) noexcept
			{
				if (this == &Other)
				{
					return *this;
				}

				Reset();

				Task = Other.Task;
				Time = Other.Time;
				Origin = Other.Origin;

				Other.Task = nullptr;
				Other.Time = -1;
				Other.Origin = nullptr;

				return *this;
			}

			FORCEINLINE bool operator<(const AsyncTimerTask& Other)const noexcept
			{
				return Time < Other.Time;
			}

			FORCEINLINE bool operator>(const AsyncTimerTask& Other)const noexcept
			{
				return Time > Other.Time;
			}

			void Release() const noexcept
			{
				Task = nullptr;
				Origin = nullptr;
				Time = -1;
			}

			void Reset() noexcept
			{
				if (Task)
				{
					Task->Destroy(true);
					Task = nullptr;
				}
			}

			//private:
			mutable TimeDiff						Time;
			mutable AsyncTaskBase* PTR				Task;
			mutable AsyncDispatcher* PTR			Origin;

			friend class AsyncTimerTaskDispatcher;
		};

		using AsyncTimerTaskPriorityQueue = TPriorityQueue<AsyncTimerTask>;

		template<typename TLambda>
		FORCEINLINE AsyncTaskBase* MakeTaskRaw(TLambda&& Lambda)noexcept
		{
			using TTaskType = AsyncTask<sizeof(TLambda)>;

			//Allocated mamanged block
			auto* Block = MemoryManager::AllocBlock<sizeof(TTaskType)>();
			if (!Block)
			{
				return nullptr;
			}

			//Construct the task into the block
			new (Block->Block) TTaskType(std::move(Lambda));

			//Set destroy handler
			((AsyncTaskBase*)Block->Block)->Destroy = [Block](bool bCallDestructor) noexcept
			{
				if (bCallDestructor)
				{
					GDestructNothrow<TTaskType>((TTaskType*)Block->Block);
				}

				Block->Destroy(false);
			};

			//Return raw pointer
			return (AsyncTaskBase*)Block->Block;
		}

		template<typename TLambda>
		FORCEINLINE TPtr<AsyncTaskBase> MakeTask(TLambda&& Lambda) noexcept
		{
			return { MakeTaskRaw<TLambda>(std::move(Lambda)) };
		}

		template<class TInstance, typename... TArgsPack>
		FORCEINLINE AsyncTaskBase* MakeMemberFunctionTaskRaw(TInstance* Object, void(TInstance::* MemberFunction)(TArgsPack...), TArgsPack... CallArgs)noexcept
		{
			using TTaskType = AsyncMemberFunctionTask<TInstance, TArgsPack...>;

			//Allocated mamanged block
			auto* Block = MemoryManager::AllocBlock<sizeof(TTaskType)>();
			if (!Block)
			{
				return nullptr;
			}

			//Construct the task into the block
			new (Block->Block) TTaskType(Object, MemberFunction, std::forward<TArgsPack>(CallArgs)...);

			//Set destroy handler
			((AsyncTaskBase*)Block->Block)->Destroy = [Block](bool bCallDestructor) noexcept
			{
				if (bCallDestructor)
				{
					GDestructNothrow<TTaskType>((TTaskType*)Block->Block);
				}

				Block->Destroy(false);
			};

			//Return raw pointer
			return (AsyncTaskBase*)Block->Block;
		}

		template<class TInstance, typename... TArgsPack>
		FORCEINLINE TPtr<AsyncTaskBase> MakeMemberFunctionTask(TInstance* Object, void(TInstance::* MemberFunction)(TArgsPack...), TArgsPack... CallArgs)noexcept
		{
			return { MakeMemberFunctionTaskRaw<TInstance, TArgsPack...>(Object, MemberFunction, std::forward<TArgsPack>(CallArgs)...) };
		}
	}
}
