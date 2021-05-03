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
# work usage example
  
  ```cpp
  RStatus WorkExample()
{
	auto work = MakeUniqueManaged<AnyWorkAsync<>>(std::move([](AnyWorkAsync<>* Self, RStatus Result) mutable noexcept
		{
			LogInfo("AsyncWork!!!");
		}));

	auto work2 = MakeUniqueManaged<MyWork>();
	work2->SetCompletionHandler([](MyWork* self, std::string* Payload, RStatus Result)
		{
			LogInfo("MyAsyncWork!!!");
		});

	//Post work item into the async system to be processed by workers
	//	async->DoAsync(std::move(work));
	//or
	//	R_TRY(async->DoAsync(work)){ work.Release(); }

	{
		//on worker
		work->CompleteWork(RSuccess, 0);
		work2->CompleteWork(RSuccess, 0);
	}

	return RSuccess;
}
```
