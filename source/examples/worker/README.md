# worker usage example

```cpp
RStatus WorkersExample()
{
	//		                 <12> - Task body size (lambda capture body max size) = 12bytes
	//WorkerGroupWithMainThread<12> Group;
	WorkerGroup<12>	Group;

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
