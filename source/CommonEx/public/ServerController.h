#pragma once

namespace CommonEx
{
	class Controller : public System
	{

	};

	template<bool bUseCallingThread = false>
	class ServerController : public Controller
	{
	public:
		using TAcceptTask = _TaskEx<8, RStatus(TSocket, sockaddr_in*)>;
		using TStopTask = _TaskEx<8, void(void)>;
		using TStartTask = _TaskEx<8, RStatus(void)>;

		using TWorkerGroup = _WorkerGroup<bUseCallingThread, 8>;
		using TWorker = typename TWorkerGroup::TMyWorker;

		ServerController() noexcept : Controller()
		{

		}

		~ServerController() noexcept
		{
			Stop();
		}

		RStatus Initialize(int32_t WorkersCount, ulong_t Ip, uint16_t Port, TAcceptTask&& AcceptTask)noexcept
		{
			this->AcceptTask = std::move(AcceptTask);

			RStatus Result = Listener.Initialize(Port, Ip,
				[this](TSocket Socket, sockaddr_in* Info) noexcept
				{
					RStatus Result = this->AcceptTask(Socket, Info);
					if (Result != RSuccess)
					{
						shutdown(Socket, SD_BOTH);
						closesocket(Socket);

#if RVERBOSE
						LogWarning("ServerController<>::AcceptTask() Connection refused Status[{}]!", Result);
#endif
					}

				});
			R_TRY_L(Result, "ServerController<>::Initialize() Failed to initialize the Listener")
			{};

			R_TRY_L(Workers.Initialize(WorkersCount
				, WorkerRoutineTask
				, WorkerTLSInitTask
				, WorkerShutdownTask
				, WorkerGroupShutdownTask
			), "ServerController<>::Initialize() Failed to initialize the Workers Group")
			{};

			return RSuccess;
		}

		RStatus Start() noexcept
		{
			if (IsStarted())
			{
				return RSuccess;
			}

			Listener.Start();

			if (StartTask)
			{
				R_TRY_L(StartTask(), "ServerController<>::Start() StartTask() Failed!") {}
			}

			if constexpr (bUseCallingThread)
			{
				SetIsStarted(true);
			}

			R_TRY_L(Workers.Start(), "ServerController<>::Start() Failed to Start() Workers Group!") {}

			if constexpr (!bUseCallingThread)
			{
				SetIsStarted(true);
			}

			if constexpr (bUseCallingThread)
			{
				Stop();
				WaitForController();
			}

			return RSuccess;
		}

		RStatus Stop() noexcept
		{
			if (!IsStarted())
			{
				return RSuccess;
			}

			Listener.Stop();

			if (StopTask)
			{
				StopTask();
			}

			R_TRY_L(Workers.Stop(), "ServerController<>::Stop() Failed to Stop() Workers Group!") {}

			SetIsStarted(false);
			return RSuccess;
		}

		FORCEINLINE void WaitForController() noexcept
		{
			Workers.JoinGroup();
		}

		FORCEINLINE bool IsStarted() const noexcept
		{
			return bIsStarted.test(std::memory_order_acquire);
		}

	protected:
		FORCEINLINE void SetIsStarted(bool bValue, bool bNotify = true)  noexcept
		{
			if (bValue)
			{
				bIsStarted.test_and_set(std::memory_order_acq_rel);
			}
			else
			{
				bIsStarted.clear(std::memory_order_release);
			}

			if (bNotify)
			{
				bIsStarted.notify_all();
			}
		}

		TAcceptTask							AcceptTask{};
		TStopTask							StopTask{};
		TStartTask							StartTask{};

		typename TWorker::TWorkerTask		WorkerRoutineTask{};
		typename TWorker::TWorkerTask		WorkerTLSInitTask{};
		typename TWorker::TWorkerTask		WorkerShutdownTask{};
		typename TWorkerGroup::TMyTask		WorkerGroupShutdownTask{};

		TCPListenerWorker					Listener;
		TWorkerGroup						Workers;

		std::atomic_flag					bIsStarted{};
	};
}
