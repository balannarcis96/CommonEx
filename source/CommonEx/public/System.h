#pragma once

namespace CommonEx
{
	class System : public MemoryResource<true>
	{
	protected:
		std::vector<TSharedPtr<System>> SystemRefs;
	};
}


