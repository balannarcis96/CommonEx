#pragma once
/**
 * @file Tunning.h
 *
 * @brief CommonEx Tunning values, redefine these for your needs.
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

namespace CommonEx {

#ifndef SmallMemBlockSize
#define SmallMemBlockSize		  512
#endif 

#ifndef MediumMemBlockSize
#define MediumMemBlockSize		  1024
#endif 
#ifndef LargeMemBlockSize
#define LargeMemBlockSize		  4096
#endif 
#ifndef ExtraLargeMemBlockSize
#define ExtraLargeMemBlockSize	  (24 * 1024)
#endif 

#ifndef SmallMemBlockCount
#define SmallMemBlockCount		  4096
#endif 
#ifndef MediumMemBlockCount
#define MediumMemBlockCount		  4096
#endif 
#ifndef LargeMemBlockCount
#define LargeMemBlockCount		  4096
#endif 
#ifndef ExtraLargeMemBlockCount
#define ExtraLargeMemBlockCount   4096
#endif 

#pragma region Buffers

	/*------------------------------------------------------------
		There are 5 size types of send buffers (tune as needed)
	  ------------------------------------------------------------*/
	constexpr size_t CSize1ServerSendBufferSize = 128;
	constexpr size_t CSize1ServerSendBufferCount = 4096;

	constexpr size_t CSize2ServerSendBufferSize = 1024;
	constexpr size_t CSize2ServerSendBufferCount = 4096;

	constexpr size_t CSize3ServerSendBufferSize = 4096;
	constexpr size_t CSize3ServerSendBufferCount = 4096;

	constexpr size_t CSize4ServerSendBufferSize = 8192;
	constexpr size_t CSize4ServerSendBufferCount = 4096;

	constexpr size_t CSize5ServerSendBufferSize = UINT16_MAX;
	constexpr size_t CSize5ServerSendBufferCount = 1024;

	/*------------------------------------------------------------
		Recv Buffer
	  ------------------------------------------------------------*/
	constexpr size_t CReceiveBufferSizeMax = UINT16_MAX;
	constexpr size_t CRecvBufferCount = 1024;

#pragma endregion
}