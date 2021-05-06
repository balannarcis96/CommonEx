#pragma once

namespace CommonEx
{
	/**
	 * Compile-time function to get the sum of sizeof each type in the TPack.
	 */
	template<typename ...TPack>
	consteval size_t CCalculatePackSize()
	{
		if (sizeof...(TPack) == 0)
		{
			return (size_t)0;
		}

		size_t result = 0;
		for (auto s : { sizeof(TPack)... }) result += s;
		return result;
	}

	/**
	 * Compile-time function to get the sum of sizeof each type in the TPack.
	 */
	template<typename SumType, typename ...TPack>
	consteval SumType CCalculatePackSizeEx();

	template<typename SumType, typename ...TPack>
	consteval SumType CCalculatePackSizeEx()
	{
		SumType result = 0;
		for (const SumType s : { sizeof(TPack)... }) result += (SumType)s;
		return result;
	}

	/**
	 * Compile-time function to get maximum of First and Second
	 */
	template<typename T>
	consteval T CMax(T First, T Second)
	{
		return First > Second ? First : Second;
	}

	/**
	 * Compile-time function to get minimum of First and Second
	 */
	template<typename T>
	consteval T CMin(T First, T Second)
	{
		return First < Second ? First : Second;
	}

}
