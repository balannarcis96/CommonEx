#pragma once
/**
 * @file Worker.h
 *
 * @brief Symetric Worker Group abstraction
 *			WorkerGroup - Workers(thread) group
 *			AsyncWorkerGroup - Workers(thread) group with an ASYNC IO API instance
 *			
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

namespace CommonEx {
	struct WorkerGroupBase;
	struct WorkerBase;

	template<typename TUpper, typename TSharedState>
	struct Worker;

	template<typename TWorker, typename TSharedState, bool bIncludeMainThread>
	struct AsyncWorkerGroup;

	template<typename TWorker, typename TSharedState, bool bIncludeMainThread>
	struct WorkerGroup;

	struct IWorkerGroupSharedState {
		std::thread::id				MainThreadId{};
		WorkerGroupBase* PTR		MyGroup{ nullptr };

		IWorkerGroupSharedState() : MainThreadId(std::this_thread::get_id()) {}

		bool IsThisTheMainThread() const noexcept {
			return MainThreadId == std::this_thread::get_id();
		}
	};

	struct IAsyncWorkerGroupSharedState : IWorkerGroupSharedState {
		CurrentAsyncIOSystem* PTR	AsyncIOInterface{ nullptr };
	};

	template<typename TSharedState>
	struct NOVTABLE IWorker {
		virtual void Run(TSharedState*) noexcept = 0;
		virtual RStatus ThreadLocalInitialize() noexcept { return RSuccess; };

	protected:
		WorkerBase* PTR				MyWorker{ nullptr };
	};

	template<typename TSharedState>
	struct NOVTABLE IAsyncWorker {
		virtual void Run(TSharedState*) noexcept = 0;
		virtual RStatus ThreadLocalInitialize() noexcept { return RSuccess; };

	protected:
		WorkerBase* PTR				MyWorker{ nullptr };

		template<typename TUpper, typename _TSharedState>
		friend struct Worker;
	};

	struct WorkerBase {
		bool IsRunning() const noexcept { return bIsRunning.test(std::memory_order_acquire); }
		bool IsMainWorker() const noexcept { return bIsMainWorker; }
		bool IsCurrentThread() const noexcept {
			return Thread.get_id() == std::this_thread::get_id();
		}

	protected:
		std::atomic_flag				bIsRunning{  };
		std::thread						Thread;
		WorkerGroupBase* PTR			Group{ nullptr };

		union {
			struct {
				unsigned				bIsMainWorker : 1;
				unsigned				bAutoStopOnMainWorkerExit : 1;
			};
			uint32_t					Flags{ 0 };
		};
	};

	struct WorkerGroupBase {
		virtual RStatus Stop(int32_t Timeout = -1) noexcept = 0;

		inline bool IsRunning() const noexcept { return bIsRunning.test(std::memory_order_acquire); }

	protected:

		std::atomic_flag				bIsRunning{  };
		std::atomic<int32_t>			RunningWorkesCount{ 0 };

		template<typename TUpper, typename TSharedState>
		friend struct Worker;
	};

	template<typename TUpper, typename TSharedState>
	struct Worker : WorkerBase {
		static_assert(std::is_base_of_v<IWorker<TSharedState>, TUpper> || std::is_base_of_v<IAsyncWorker<TSharedState>, TUpper>, "TUpper must inherit from IWorker or IAsyncWorker");
		static_assert(std::is_base_of_v<IWorkerGroupSharedState, TSharedState> || std::is_base_of_v<IAsyncWorkerGroupSharedState, TSharedState>, "TSharedState must inherit from IWorkerGroupSharedState or IAsyncWorkerGroupSharedState");

		using MyType = Worker<TUpper, TSharedState>;

		Worker() {
			Interface.MyWorker = this;
		}

		RStatus Start(TSharedState* SharedState) noexcept
		{
			if (bIsRunning.test(std::memory_order_acquire)) {
				return RStatus::AlreadyPerformed;
			}

			Thread = std::thread(&MyType::RunImpl, this, SharedState);
			//Thread.detach();

			return RSuccess;
		}

		RStatus Stop(int32_t Timeout = -1) noexcept
		{
			if (Timeout < 0) {
				if (!IsCurrentThread()) {
					while (IsRunning()) {
						std::this_thread::yield();
					}
				}
			}
			else {
				TClock::time_point Start = TClock::now();
				while (IsRunning()) {
					std::this_thread::yield();

					auto Delta = TCLOCK_MILLIS((TClock::now() - Start).count());
					if (Delta.count() > Timeout) {
						return RTimedout;
					}
				}
			}

			if (!IsCurrentThread() && Thread.joinable()) {
				Thread.join();
			}

			return RSuccess;
		}

	private:
		TUpper	Interface;

		RStatus ThreadLocalInitializeImpl() noexcept {
			RTRY_L(Interface.ThreadLocalInitialize(), "Worker::TUpper::ThreadLocalInitialize()") {}

			return RSuccess;
		}
		void RunImpl(TSharedState* SharedState)noexcept
		{
			RTRY_S_L(ThreadLocalInitializeImpl(), void(), "Worker::RunImpl() Failed  ThreadLocalInitialize()") {}

			//Set is running 
			this->bIsRunning.test_and_set(std::memory_order_acq_rel);

			//Actual worker loop
			Interface.Run(SharedState);

			//Clear is running
			this->bIsRunning.clear(std::memory_order_release);

			if (this->Group) {
				if (bAutoStopOnMainWorkerExit) {
					if (IsMainWorker()) {
						this->Group->Stop();
					}
					else {
						this->Group->RunningWorkesCount--;
					}
				}
				else {
					this->Group->RunningWorkesCount--;
				}
			}
		}

		friend WorkerGroupBase;
		friend WorkerGroup;
	};

	template<typename TWorker, typename TSharedState, bool bIncludeMainThread>
	struct WorkerGroup : WorkerGroupBase {
		static_assert(std::is_base_of_v<IWorker<TSharedState>, TWorker> || std::is_base_of_v<IAsyncWorker<TSharedState>, TWorker>, "TUpper must inherit from IWorker or IAsyncWorker");
		static_assert(std::is_base_of_v<IWorkerGroupSharedState, TSharedState> || std::is_base_of_v<IAsyncWorkerGroupSharedState, TSharedState>, "TSharedState must inherit from IWorkerGroupSharedState or IAsyncWorkerGroupSharedState");

		using MyWorker = Worker<TWorker, TSharedState>;

		WorkerGroup(int32_t WorkersCount) : WorkersCount(WorkersCount) {
			if constexpr (bIncludeMainThread) {
				this->WorkersCount++;
			}

			Workers = std::unique_ptr<MyWorker[]>(new MyWorker[this->WorkersCount]);
		}
		~WorkerGroup() {
			Stop();
		}

		int32_t GetWorkersCount() const noexcept {
			return WorkersCount;
		}

		inline void SetSharedState(TSharedState&& State) noexcept {
			SharedState = std::move(State);

			SharedState.MyGroup = this;
		}

		virtual RStatus Start() noexcept {
			if (IsRunning()) {
				return RStatus::AlreadyPerformed;
			}

			int32_t  WorkersCount = this->WorkersCount;
			if constexpr (bIncludeMainThread) {
				WorkersCount--;
			}

			RunningWorkesCount = WorkersCount;

			for (int32_t i = 0; i < WorkersCount; i++)
			{
				Workers[i].Group = this;
				Workers[i].bIsMainWorker = false;

				/*if constexpr (bIncludeMainThread) {
					Workers[i].bAutoStopOnMainWorkerExit = true;
				}
				else {
					Workers[i].bAutoStopOnMainWorkerExit = false;
				}*/

				if (Workers[i].Start(&SharedState) != RSuccess) {
					RunningWorkesCount--;
					RTRY_L(RFail, "WorkerGroup<>::Start() Failed to Worker::Start()") {}
				}
			}

			//The group is now running
			this->bIsRunning.test_and_set(std::memory_order_acq_rel);

			if constexpr (bIncludeMainThread) {
				//Give this thread to the last worker 

				//Set as main worker
				Workers[WorkersCount].Group = this;
				Workers[WorkersCount].bIsMainWorker = true;
				Workers[WorkersCount].bAutoStopOnMainWorkerExit = true;
				Workers[WorkersCount].RunImpl((TSharedState*)&SharedState);
			}

			return RSuccess;
		}

		virtual RStatus Stop(int32_t Timeout = -1) noexcept override {
			if (!IsRunning()) {
				return RStatus::AlreadyPerformed;
			}

			//The group is now stopped
			this->bIsRunning.clear(std::memory_order_release);

			for (int32_t i = 0; i < GetWorkersCount(); i++)
			{
				Workers[i].Stop(Timeout);
			}

			return RSuccess;
		}

	protected:
		TSharedState					SharedState{};

		int32_t							WorkersCount{ 0 };
		std::unique_ptr<MyWorker[]>		Workers;

		friend Worker<TWorker, TSharedState>;
		friend IWorker<TSharedState>;
		friend IAsyncWorker<TSharedState>;
	};

	template<typename TWorker, typename TSharedState, bool bIncludeMainThread>
	struct NOVTABLE AsyncWorkerGroup : WorkerGroup<TWorker, TSharedState, bIncludeMainThread> {
		static_assert(std::is_base_of_v<IAsyncWorker<TSharedState>, TWorker>, "TWorker must inherit from IAsyncWorker");
		static_assert(std::is_base_of_v<IAsyncWorkerGroupSharedState, TSharedState>, "TSharedState must inherit from IAsyncWorkerGroupSharedState");

		using Base = WorkerGroup<TWorker, TSharedState, bIncludeMainThread>;

		AsyncWorkerGroup(int32_t WorkersCount) : Base(WorkersCount) {}

		virtual RStatus Initialize()noexcept {
			RTRY_L(AsyncIOInterface.Start(this->WorkersCount), "AsyncWorkerGroup::Initialize() -> Failed to TAsyncIO::Start()") {}

			return RSuccess;
		}

		virtual RStatus Start() noexcept {
			if (this->IsRunning()) {
				return RStatus::AlreadyPerformed;
			}

			return Base::Start();
		}

		virtual RStatus Stop(int32_t Timeout = -1) noexcept {
			if (!this->IsRunning()) {
				return RStatus::AlreadyPerformed;
			}

			RTRY_L(AsyncIOInterface.Shutdown(Timeout), "AsyncWorkerGroup::Stop() Failed to TAsyncIO::Shutdown()") {}

			return Base::Stop(Timeout);
		}

		inline void SetSharedState(TSharedState&& State) noexcept {
			Base::SetSharedState(std::move(State));

			this->SharedState.AsyncIOInterface = &AsyncIOInterface;
		}

		inline CurrentAsyncIOSystem* GetAsyncInterface() noexcept {
			return &AsyncIOInterface;
		}
		inline const CurrentAsyncIOSystem* GetAsyncInterface() const noexcept {
			return &AsyncIOInterface;
		}

	protected:
		CurrentAsyncIOSystem	AsyncIOInterface;
	};
}