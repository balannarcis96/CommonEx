#pragma once
/**
 * @file Worker.h
 *
 * @brief Symetric Workers Group abstraction
 *			WorkerGroup - Workers(thread) group
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

namespace CommonEx
{
	template<bool bIncludeMainThread, size_t WorkerPayloadSize = 64>
	class _WorkerGroup;

	class WorkerBase;

	/**
	 * \brief Worker Group base class
	*/
	class NOVTABLE WorkerGroupShared
	{
	public:
		WorkerGroupShared()noexcept {}

		/**
		 * \return true if this worker group is running, false otherwise
		*/
		FORCEINLINE bool IsRunning() const noexcept { return bIsRunning.test(std::memory_order_acquire); }

		/**
		 * \return the total workers count in this group
		*/
		FORCEINLINE int32_t GetWorkersCount() const noexcept
		{
			return TotalWorkers.load();
		}

		/**
		 * \return the total active (running) workers count in this group
		*/
		FORCEINLINE int32_t GetRunningWorkerCount() const noexcept
		{
			return RunningWorkers.load();
		}

		/**
		 * \brief Start the worker group
		 *
		 * \returns [RSuccess] on a successful start
		 * \retval [RFail] the start process fail
		*/
		_NODISCARD virtual RStatus Start() noexcept = 0;

		/**
		 * \brief Stop the worker group
		 *
		 * \returns [RSuccess] on a successful stop
		 * \retval [RFail] the stop process fail
		*/
		_NODISCARD virtual RStatus Stop(bool bTryJoinGroup = true) noexcept = 0;

		/**
		 * \brief Wait for the thread [IsRunning] state to change to the expected [bValue]
		 */
		FORCEINLINE void WaitForIsRunning(bool bValue) const noexcept
		{
			bIsRunning.wait(bValue, std::memory_order_acquire);
		}

	protected:
		virtual bool OnWorkerShutdown(WorkerBase* Worker) noexcept = 0;

		FORCEINLINE void SetIsRunning(bool bValue, bool bNotifyChange = true)noexcept
		{
			if (bValue)
			{
				bIsRunning.test_and_set(std::memory_order_acq_rel);
			}
			else
			{
				bIsRunning.clear(std::memory_order_release);
			}

			if (bNotifyChange)
			{
				bIsRunning.notify_all();
			}
		}

		std::atomic_int32_t		RunningWorkers{ 0 };
		std::atomic_int32_t		TotalWorkers{ 0 };
		std::atomic_flag		bIsRunning{ };

		template<size_t PayloadSize>
		friend class Worker;
	};

	/**
	 * \brief Worker<> base class
	*/
	class WorkerBase
	{
	public:
		WorkerBase() {}

		//Can't copy
		WorkerBase(const WorkerBase&) = delete;
		WorkerBase& operator=(const WorkerBase&) = delete;

		//Can't move
		WorkerBase(WorkerBase&& Other) = delete;
		WorkerBase& operator=(WorkerBase&& Other) = delete;

		/**
		 * \return [true] if the thread calling this function the is owner of this instance, [false] otherwise.
		 */
		FORCEINLINE bool IsCurrentThread() const noexcept
		{
			return Thread.get_id() == std::this_thread::get_id();
		}

		/**
		 * \return [true] if the thread is running, [false] otherwise.
		 */
		FORCEINLINE bool IsRunning() const noexcept
		{
			return bIsRunning.test(std::memory_order_acquire);
		}

		/**
		 * \brief Wait for the thread [IsRunning] state to change to the expected [bValue]
		 */
		FORCEINLINE void WaitForIsRunning(bool bValue) const noexcept
		{
			bIsRunning.wait(bValue, std::memory_order_acquire);
		}

		/**
		 * \brief Wait for the thread to end if the thread is joinable (has not finished yet)
		 */
		FORCEINLINE void Join() noexcept
		{
			if (!Thread.joinable())
			{
				return;
			}

			Thread.join();
		}

		/**
		 * \brief Get the underlying thread id
		 */
		FORCEINLINE std::thread::id GetId() const noexcept
		{
			return Thread.get_id();
		}

	protected:
		FORCEINLINE void SetIsRunning(bool bValue, bool bNotifyChange = true)noexcept
		{
			if (bValue)
			{
				bIsRunning.test_and_set(std::memory_order_acq_rel);
			}
			else
			{
				bIsRunning.clear(std::memory_order_release);
			}

			if (bNotifyChange)
			{
				bIsRunning.notify_all();
			}
		}

		std::thread				Thread;
		std::atomic_flag		bIsRunning{ };
		RStatus					Status{ RSuccess };

		template<bool bIncludeMainThread, size_t WorkerPayloadSize>
		friend class _WorkerGroup;
	};

	/**
	 * \brief Worker class (worker thread abstraction)
	 * \param [PayloadSize] The maximum size of the _TaskEx<> used for initialization and as the worker main routine
	 * \param [bAutoStopOnMainWorkerExit] If true when the thread marked as "main thread" exits it will Stop() the whole WorkerGroup
	*/
	template<size_t PayloadSize = 32>
	class Worker : public WorkerBase
	{
	public:
		using MyType = Worker<PayloadSize>;

		typedef RStatus(TaskFunction)(WorkerBase*, WorkerGroupShared*);

		using TWorkerTask = _TaskEx<PayloadSize, TaskFunction>;

	protected:
		TWorkerTask				Task;
		TWorkerTask				TLSInitTask;
		TWorkerTask				OnShutdownTask;

		/**
		 * \brief Initialize and Start the worker
		 *
		 * \param [MyGroup] Pointer to the owning group
		 *
		 * \return RSuccess on a successful start
		*/
		virtual RStatus Start(WorkerGroupShared* MyGroup) noexcept
		{
			if (IsRunning())
			{
				return RSuccess;
			}

			Thread = std::thread(&MyType::RunImpl, this, MyGroup);

			WaitForIsRunning(true);

			if (Status != RSuccess)
			{
				SetIsRunning(false);
			}

			return Status;
		}

		/**
		 * \brief Stop the worker
		 *
		 * \param [bTryToJoin] If true the fuction will call Join() on the worker
		*/
		virtual void Stop(bool bTryToJoin = false) noexcept
		{
			if (!IsRunning())
			{
				return;
			}

			SetIsRunning(false);

			if (bTryToJoin)
			{
				Join();
			}
		}

	private:
		void RunImpl(WorkerGroupShared* MyGroup)noexcept
		{
			if (TLSInitTask)
			{
				bool bHasFailed = false;

				{
					Status = TLSInitTask(this, MyGroup);

					if (Status != RSuccess)
					{
						bHasFailed = true;
					}

					SetIsRunning(true);
				}

				if (bHasFailed)
				{
					return;
				}
			}
			else
			{
				Status = RSuccess;
				SetIsRunning(true);
			}

			RStatus Result = Task(this, MyGroup);
			if (Result != RSuccess)
			{
				LogInfo("Worker<> Failed with status {}", Result);
			}

			if (OnShutdownTask)
			{
				RStatus Result = OnShutdownTask(this, MyGroup);
				if (Result != RSuccess)
				{
					LogInfo("Worker<>::OnShutdownTask() Failed with status {}", Result);
				}
			}

			SetIsRunning(false);

			MyGroup->OnWorkerShutdown(this);
		}

		template<bool bIncludeMainThread, size_t WorkerPayloadSize>
		friend class _WorkerGroup;
	};

	/**
	 * \brief Simetric workers group abstraction
	 *
	 * \param [bIncludeMainThread] If true the thread that calls Initialize is treated as the main thread and its used as an aditional worker
	 * \param [WorkerPayloadSize] The maximum payload size for the _TaskEx<> used for each worker
	*/
	template<bool bIncludeMainThread, size_t WorkerPayloadSize>
	class _WorkerGroup : public WorkerGroupShared
	{
	public:
		using TMyWorker = Worker<WorkerPayloadSize>;
		using TMyTask = _TaskEx<WorkerPayloadSize, void(WorkerGroupShared*)>;

		_WorkerGroup() noexcept : WorkerGroupShared() {}

		//Can't copy
		_WorkerGroup(const _WorkerGroup&) = delete;
		_WorkerGroup& operator=(const _WorkerGroup&) = delete;

		//Can move
		_WorkerGroup(_WorkerGroup&& Other) noexcept
		{
			(*this) = std::move(Other);
		}
		_WorkerGroup& operator=(_WorkerGroup&& Other) noexcept
		{
			if (this == &Other)
			{
				return *this;
			}

			if (IsRunning())
			{
				LogFatal("WorkerGroup<>::operator=() The target worker group was still running when the copy attempted!");
				abort();
			}

			if (Other.IsRunning())
			{
				LogFatal("WorkerGroup<>::operator=() The source worker group was still running when the copy attempted!");
				abort();
			}

			Workers = std::move(Other.Workers);
			Other.RunningWorkers = Other.RunningWorkers;
			Other.TotalWorkers = Other.TotalWorkers;

			return *this;
		}

		~_WorkerGroup()noexcept
		{
			Reset();
		}

		/**
		 * \brief Initialize the worker group by creating and preparing the workers
		 *
		 * \param [WorkersCount] The count of workers; Not including the "main" thread if @@bIncludeMainThread=true
		 * \param [WorkerTask][optional] Task that each worker thread will run (the worker routine task)
		 * \param [WorkerTLSInitTask][optional] Task to be run by each worker thread at the start to initialize any thread-local data
		 * \param [WorkerOnShutdownTask][optional] Task that each worker thread will run (the worker shutdown task)
		 * \param [OnGroupShutdown][optional] Task that run after all threads in the group stop
		*/
		_NODISCARD RStatus Initialize(
			int32_t WorkersCount
			, const typename TMyWorker::TWorkerTask& WorkerTask
			, const typename TMyWorker::TWorkerTask& WorkerTLSInitTask = {}
			, const typename TMyWorker::TWorkerTask& WorkerOnShutdownTask = {}
			, const	TMyTask& OnGroupShutdown = {}
		) noexcept
		{
			if (IsRunning())
			{
				LogInfo("WorkerGroup<>::Initialize() Can't initialize running worker group!");
				abort();
			}

			this->OnGroupShutdown = OnGroupShutdown;

			if constexpr (bIncludeMainThread)
			{
				WorkersCount++;
			}

			TotalWorkers.store(WorkersCount);
			if (!TotalWorkers.load())
			{
				return RSuccess;
			}

			Workers = std::unique_ptr<TMyWorker[]>(new TMyWorker[WorkersCount]);
			if (!Workers.get())
			{
				LogWarning("WorkerGroup<>::Initialize() Failed to allocate workers!");
				return RFail;
			}

			for (int32_t i = 0; i < WorkersCount; i++)
			{
				if (WorkerTask)
				{
					Workers[i].Task = WorkerTask;
				}

				if (WorkerTLSInitTask)
				{
					Workers[i].TLSInitTask = WorkerTLSInitTask;
				}

				if (WorkerOnShutdownTask)
				{
					Workers[i].OnShutdownTask = WorkerOnShutdownTask;
				}
			}

			return RSuccess;
		}

		/**
		 * \brief Start the workers in this group.
		 *	Call Initialize() before calling Start()
		 *
		 * \returns [RSuccess] on a successful start
		*/
		_NODISCARD RStatus Start() noexcept
		{
			if (!Workers.get())
			{
				return RFail;
			}

			if (IsRunning())
			{
				return RSuccess;
			}

			int32_t WorkersCount = TotalWorkers.load();

			if constexpr (bIncludeMainThread)
			{
				WorkersCount--;
			}

			RunningWorkers = 0;

			for (int32_t i = 0; i < WorkersCount; i++)
			{
				R_TRY_L(this->Workers[i].Start(this), "WorkerGroup<>::Start() Failed to Worker::Start()") {}

				RunningWorkers++;
			}

			SetIsRunning(true);

			if constexpr (bIncludeMainThread)
			{
				RunningWorkers++;
				this->Workers[WorkersCount].RunImpl(this);
			}

			return RSuccess;
		}

		/**
		 * \brief Stops the workers in this group
		 *
		 * \returns [RSuccess] on a successful stop
		*/
		RStatus Stop(bool bTryJoinGroup = true) noexcept
		{
			if (!IsRunning())
			{
				return RSuccess;
			}

			SetIsRunning(false);

			int32_t WorkersCount = GetRunningWorkerCount();

			for (int32_t i = 0; i < WorkersCount; i++)
			{
				Workers[i].Stop();
			}

			bool bIsCalledFromWithin = false;
			WorkersCount = GetWorkersCount();
			for (int32_t i = 0; i < WorkersCount; i++)
			{
				if (Workers[i].IsCurrentThread())
				{
					bIsCalledFromWithin = true;
					break;
				}
			}

			if (bIsCalledFromWithin)
			{
				return RSuccess;
			}

			if (bTryJoinGroup)
			{
				JoinGroup();
			}

			return RSuccess;
		}

		/**
		 * \brief Joins all the running threads if the group is running.
		 */
		FORCEINLINE void JoinGroup() noexcept
		{
			int32_t WCount = GetRunningWorkerCount();
			if (!WCount)
			{
				return;
			}

			for (int32_t i = 0; i < WCount; i++)
			{
				Workers[i].Join();
			}
		}

		/**
		 * \returns [true] if the [TargetThreadId] is part of this group, false otherwise.
		 */
		FORCEINLINE bool IsThisAGroupThread(const std::thread::id TargetThreadId = std::this_thread::get_id()) const noexcept
		{
			if (!Workers.get() || !IsRunning())
			{
				return false;
			}

			int32_t WCount = GetWorkersCount();
			for (int32_t i = 0; i < WCount; i++)
			{
				if (Workers[i].Thread.get_id() == TargetThreadId)
				{
					return true;
				}
			}

			return false;
		}

		/**
		 * \brief Resets the worker group. Stops the workers and deletes all the data.
		 *  Call Initialize() after this if needed
		 * 
		 * \returns [true] if the [TargetThreadId] is part of this group, false otherwise.
		*/
		FORCEINLINE RStatus Reset() noexcept
		{
			if (!IsRunning() && !Workers.get())
			{
				return RSuccess;
			}

			if (IsThisAGroupThread())
			{
				return RStatus::ThreadIsOwnedByTheGroup;
			}

			Stop(false);

			JoinGroup();

			Workers.reset();

			return RSuccess;
		}

	private:
		virtual bool OnWorkerShutdown(WorkerBase* Worker) noexcept
		{
			if (RunningWorkers.fetch_sub(1) > 1)
			{
				return false;
			}

			if (OnGroupShutdown)
			{
				OnGroupShutdown(this);
			}

			SetIsRunning(false);

			return true;
		}

		std::unique_ptr<TMyWorker[]>	Workers{ nullptr };
		TMyTask							OnGroupShutdown{ };
	};

	template<size_t WorkerPayloadSize = 32>
	using WorkerGroup = _WorkerGroup<false, WorkerPayloadSize>;

	template<size_t WorkerPayloadSize = 32>
	using WorkerGroupWithMainThread = _WorkerGroup<true, WorkerPayloadSize>;
}
