#pragma once

namespace CommonEx
{
	namespace Async
	{
		/**
		 * Single consumer, multiple producers custom async task queue.
		 */
		class TaskQueue
		{
		public:
			TaskQueue() noexcept
				:Head(&Stub)
				, Tail(&Stub)
			{}
			~TaskQueue() noexcept
			{
				AsyncTaskBase* Item = Pop();
				while (Item)
				{
					Item->Destroy(true);
					Item = Pop();
				}
			}

			FORCEINLINE void Push(AsyncTaskBase* Task) noexcept
			{
				AsyncTaskBase* PrevNode = (AsyncTaskBase*)std::atomic_exchange_explicit(&Head, Task, std::memory_order_acq_rel);

				PrevNode->Next = Task;
			}

			_NODISCARD AsyncTaskBase* Pop() noexcept
			{
				AsyncTaskBase* L_Tail = (AsyncTaskBase*)Tail;
				AsyncTaskBase* L_Next = L_Tail->Next;

				if (L_Tail == &Stub)
				{
					//Empty case
					if (L_Next == nullptr)
					{
						return nullptr;
					}

					//First pop
					Tail = L_Next;
					L_Tail = L_Next;
					L_Next = L_Next->Next;
				}

				//Most cases
				if (L_Next) _LIKELY
				{
					Tail = L_Next;
					return L_Tail;
				}

				AsyncTaskBase* L_Head = Head.load(std::memory_order_acquire);
				if (L_Tail != L_Head)
				{
					return nullptr;
				}

				//Last pop
				Stub.Next = nullptr;

				AsyncTaskBase* L_PrevNode = std::atomic_exchange_explicit(&Head, &Stub, std::memory_order_acq_rel);

				L_PrevNode->Next = &Stub;

				L_Next = Tail->Next;
				if (L_Next)
				{
					Tail = L_Next;
					return L_Tail;
				}

				return nullptr;
			}

		private:
			std::atomic<AsyncTaskBase*>		Head;

			AsyncTaskBase* PTR				Tail;
			AsyncTaskBase					Stub{};
		};

		/**
		 * \brief Thread local deffered tasks dispatcher.
		 */
		class AsyncTimerTaskDispatcher
		{
		public:
			FORCEINLINE void Initialize() noexcept
			{
				BeginTick = TClock::now();
				CurrentLoopBeginTick = TClock::now();
			}

			FORCEINLINE void Update() noexcept
			{
				CurrentLoopBeginTick = TClock::now();
			}

			/**
			 * \return Milliseconds from the call to Initialize till now
			 */
			FORCEINLINE TimeDiff GetUpTicks() const noexcept
			{
				return std::chrono::duration_cast<std::chrono::milliseconds>(TClock::now() - BeginTick).count();
			}

			/**
			 * \return Milliseconds from the last call to Update() till now
			 */
			FORCEINLINE int64_t GetCurrentLoopTick() const noexcept
			{
				return std::chrono::duration_cast<std::chrono::milliseconds>(TClock::now() - CurrentLoopBeginTick).count();
			}

			void PushTask(uint32_t Time, AsyncTaskBase* Task, AsyncDispatcher* Origin) noexcept;

			void Tick() noexcept;

			void Shutdown() noexcept;

			~AsyncTimerTaskDispatcher() noexcept;

		private:
			AsyncTimerTaskDispatcher() noexcept {}

			TClock::time_point				CurrentLoopBeginTick{};
			TClock::time_point				BeginTick{};
			AsyncTimerTaskPriorityQueue		TaskQueue{};

			friend class AsyncSystem;
			friend class TAsyncDispatcher;
		};

		/**
		 * \brief Async dispatcher class, inherit from this to DoAsync etc.
		 */
		class AsyncDispatcher : public MemoryResource<true>
		{
		public:
			virtual ~AsyncDispatcher() noexcept
			{
#if _DEBUG
				assert(GetRefCount() == 0);
#endif
				RemainingTasksCount = 0;
			}

			template <typename T, typename... TArgs>
			FORCEINLINE RStatus DoAsync(void (T::* MemberFunction)(TArgs...), TArgs... Args) noexcept
			{
				AsyncTaskBase* Task = MakeMemberFunctionTaskRaw<T, TArgs...>((T*)this, MemberFunction, std::forward<TArgs>(Args)...);
				if (!Task)
				{
					LogFatal("AsyncDispatcher::DoAsync<>() Failed to allocate Task!");
					return RFail;
				}

				Dispatch(Task);

				return RSuccess;
			}

			template <typename TLambda>
			FORCEINLINE RStatus DoAsync(TLambda&& Lambda) noexcept
			{
				AsyncTaskBase* Task = MakeTaskRaw<TLambda>(std::move(Lambda));
				if (!Task)
				{
					LogFatal("AsyncDispatcher::DoAsync<>(TLambda&&) Failed to allocate Task!");
					return RFail;
				}

				Dispatch(Task);

				return RSuccess;
			}

			template <typename T, typename... TArgs>
			FORCEINLINE RStatus DoAfterAsync(uint32_t AfterMilliseconds, void (T::* MemberFunction)(TArgs...), TArgs... Args) noexcept
			{
				AsyncTaskBase* Task = MakeMemberFunctionTaskRaw<T, TArgs...>((T*)this, MemberFunction, std::forward<TArgs>(Args)...);
				if (!Task)
				{
					LogFatal("AsyncDispatcher::DoAfterAsync<>() Failed to allocate Task!");
					return RFail;
				}

				AsyncSystem::GTimer->PushTask(AfterMilliseconds, Task, this);

				return RSuccess;
			}

			template <typename TLambda>
			FORCEINLINE RStatus DoAfterAsync(uint32_t AfterMilliseconds, TLambda&& Lambda) noexcept
			{
				AsyncTaskBase* Task = MakeTaskRaw<TLambda>(std::move(Lambda));
				if (!Task)
				{
					LogFatal("AsyncDispatcher::DoAfterAsync<>(TLambda&&) Failed to allocate Task!");
					return RFail;
				}

				AsyncSystem::GTimer->PushTask(AfterMilliseconds, Task, this);

				return RSuccess;
			}
		private:
			/*
			* \brief Execute all tasks registered in the TaskQueue of this AsyncDispatcher instance
			*/
			void Flush() noexcept;

			/*
			* \brief Push a task into Job Queue and then Execute tasks if possible
			*/
			void Dispatch(AsyncTaskBase* Task) noexcept;

			TaskQueue					Queue;
			std::atomic<int64_t>		RemainingTasksCount{ 0 };

			friend class AsyncTimerTaskDispatcher;
		};
	}
}
