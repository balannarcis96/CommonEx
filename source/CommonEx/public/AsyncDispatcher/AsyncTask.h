#pragma once

namespace CommonEx
{
	namespace Async
	{
		constexpr auto CTaskBaseMaxSize = 16;
		
		class NodeEntry
		{
		public:
			NodeEntry* volatile Next{ nullptr };
		};

		class AsyncTaskBase :public NotSharedMemoryResourceBase
		{
		public:
			using TExecuteTask = _TaskEx<CTaskBaseMaxSize, void(void)>;

			FORCEINLINE void ExecuteTask() noexcept
			{
				OnExecute();
			}

			NodeEntry			Node{};
			TExecuteTask		OnExecute{};
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
			using TMemberFunction = void(TInstance::*)(TArgsPack...);
			using TArgs = std::tuple<TInstance*, TArgsPack...>;

			AsyncMemberFunctionTask(TInstance* Object, TMemberFunction MemberFunction, TArgsPack... CallArgs) noexcept
				: AsyncTaskBase()
				, Args(Object, std::forward<TArgsPack>(CallArgs)...)
			{
				this->OnExecute = [this, MemberFunction]()
				{
					std::apply(MemberFunction, Args);
				};
			}

			TArgs Args;
		};

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

			//Set same destroy handler
			((AsyncTaskBase*)Block->Block)->Destroy = Block->Destroy;

			//Return raw pointer
			return (AsyncTaskBase*)Block->Block;
		}

		template<typename TLambda>
		FORCEINLINE TPtr<AsyncTaskBase> MakeTask(TLambda&& Lambda)noexcept
		{
			return { MakeTaskRaw<TLambda>(std::move(Lambda)) };
		}
	}
}
