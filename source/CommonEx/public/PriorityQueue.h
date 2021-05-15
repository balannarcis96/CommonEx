#pragma once
/**
 * @file PriorityQueue.h
 *
 * @brief Priority Queue structure (std::priority_queue using custom container).
 * * Memory management is done through the MemoryManager *
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 */

namespace CommonEx
{
	template <typename T, typename Comparator = std::greater<T>>
	using TPriorityQueue = std::priority_queue<T, TStructureBase<T, false>, Comparator>;
}
