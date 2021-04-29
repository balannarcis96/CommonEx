# task usage example

```cpp
	TPtr<TypeA> a{ nullptr };

		{
			TaskEx<16, void(int)> T1;

			{
				auto d = MakeUnique<TypeA>();

				//we move the ownershit of d into the task body
				T1 = [d{ std::move(d) }, &a](int i) mutable
				{
					// move the ownership of d into a
					a.reset(d.release());

					//if we dont move ownership of d into a , d will be destroyed with the task and freed (as expected!)

					LogInfo("T1()");
				};

				LogInfo("### Out Of Scope ###");
			}

			T1(23);

			LogInfo("### Out Of Scope 2 ###");
		}

		LogInfo("### Out Of Scope 3 ###");
	}
  ```
  
  ![image](https://user-images.githubusercontent.com/8436410/116505262-1affe880-a8c3-11eb-90db-739319cf1398.png)
