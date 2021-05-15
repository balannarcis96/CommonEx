#pragma once

namespace CommonEx
{
	/**
	 * Queue structure (dynamic sized queue).
	 * * Memory management is done through the MemoryManager *
	 */
	template <typename T, uint32_t GrowStep = 64, uint32_t FrontSlackMax = 64>
	class TQueue
	{
	public:
		~TQueue() noexcept
		{
			Elements.Destroy<true>();
			Front = 0;
		}

		FORCEINLINE void Push(T&& Element) noexcept
		{
			if (!Elements.Push(std::move(Element)))
			{
				LogFatal("TQueue<>::Push(T&&) Failed!");
				abort();
			}
		}

		FORCEINLINE void Push(const T& Element) noexcept
		{
			if (!Elements.Push(Element))
			{
				LogFatal("TQueue<>::Push(T&&) Failed!");
				abort();
			}
		}

		FORCEINLINE T& Top() noexcept
		{
			return Elements[Front];
		}

		FORCEINLINE const T& Top() const noexcept
		{
			return Elements[Front];
		}

		FORCEINLINE void Pop() noexcept
		{
			if (GetSize() == 0)
			{
				return;
			}

			Front++;

			Balance();
		}

		FORCEINLINE bool GetSize() const noexcept
		{
			return Elements.GetCount() - Front;
		}

	protected:
		void Balance() noexcept
		{
			if (GetSize() == 0)
			{
				return;
			}

			uint32_t FrontSlack = (Front + 1);
			if (FrontSlack < FrontSlackMax)
			{
				return;
			}

			if constexpr (std::is_nothrow_destructible_v<T>)
			{
				for (uint32_t i = 0; i < FrontSlack; i++)
				{
					Elements[i].~T();
				}
			}
			else if constexpr (std::is_destructible_v<T>)
			{
				static_assert(false, "TQueue<T> T must be nothrow destructible");
			}

			Elements.ShiftLeft(FrontSlack);

			Elements.Count -= FrontSlack;

			Front = 0;

			LogInfo("TQueue<>::Balance() FrontSlack: {}", FrontSlack);
		}

		TVector<T>	Elements;
		uint32_t	Front{ 0 };
	};
}
