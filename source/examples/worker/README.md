# worker usage example

```cpp
RStatus WorkersExample()
{
	//		                 <12> - Task body size (lambda capture body max size) = 32bytes
	//WorkerGroupWithMainThread<12> Group;
	WorkerGroup<12>	Group;

	constexpr auto t = sizeof(Group);
	constexpr auto t2 = sizeof(WorkerGroup<12>::TMyWorker);

	RStatus Result = Group.Initialize(
		2 //2 workers in this group
		, [/* capture body can be max 12bytes in size */](WorkerBase* Worker, WorkerGroupShared* Group) mutable noexcept
		{

			LogInfo("Worker<>::OnRun");

			return RSuccess;
		});
	if (Result != RSuccess)
	{
		return RFail;
	}

	Result = Group.Start();
	if (Result != RSuccess)
	{
		return RFail;
	}

	return RSuccess;
}
  ```
