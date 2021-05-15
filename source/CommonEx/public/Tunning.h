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

//64 bytes
#ifndef Tiny1MemBlockSize
#define Tiny1MemBlockSize		  64
#endif 

//128 bytes
#ifndef Tiny2MemBlockSize
#define Tiny2MemBlockSize		  128
#endif 

//512 bytes
#ifndef Tiny3MemBlockSize
#define Tiny3MemBlockSize		  512
#endif 

//1 kb
#ifndef SmallMemBlockSize
#define SmallMemBlockSize		  1024
#endif 

//512 kb
#ifndef MediumMemBlockSize
#define MediumMemBlockSize		  1024 * 512
#endif 

//4 mb
#ifndef LargeMemBlockSize
#define LargeMemBlockSize		  (1024 * 1024) * 4
#endif 

//16 mb
#ifndef ExtraLargeMemBlockSize
#define ExtraLargeMemBlockSize	  (1024 * 1024) * 16
#endif 

#ifndef Tiny1MemBlockCount
#define Tiny1MemBlockCount		  32768
#endif 

#ifndef Tiny2MemBlockCount
#define Tiny2MemBlockCount		  32768
#endif 

#ifndef Tiny3MemBlockCount
#define Tiny3MemBlockCount		  32768
#endif 

#ifndef SmallMemBlockCount
#define SmallMemBlockCount		  16384
#endif 

#ifndef MediumMemBlockCount
#define MediumMemBlockCount		  8192
#endif 

#ifndef LargeMemBlockCount
#define LargeMemBlockCount		  128
#endif 

#ifndef ExtraLargeMemBlockCount
#define ExtraLargeMemBlockCount   128
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
	constexpr size_t CRecvBufferCount = 1024;

	/*------------------------------------------------------------
		TaskEx
	  ------------------------------------------------------------*/
	constexpr size_t CTaskExBodySize = 32;
#pragma endregion
}