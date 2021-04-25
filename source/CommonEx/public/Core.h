#pragma once
/**
 * @file Core.h
 *
 * @brief CommonEx core abstractions
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

#ifndef ALIGNMENT
#define ALIGNMENT alignof(size_t)
#endif

#ifndef FORCEINLINE
#define FORCEINLINE __forceinline
#endif

//Empty macro used to keep Visual Studio from indenting
#define PTR

namespace CommonEx {
	// Void * pointer type.
	using ptr_t = void*;

	using ulong_t = unsigned long;

	using long_t = long;

	//Read macros
#define r_8	(_raw)		( *reinterpret_cast<int8_t*>	(_raw) )
#define r_u8(_raw)		( *reinterpret_cast<uint8_t*>	(_raw) )
#define r_16(_raw)		( *reinterpret_cast<int16_t*>	(_raw) )
#define r_u16(_raw)		( *reinterpret_cast<uint16_t*>	(_raw) )
#define r_32(_raw)		( *reinterpret_cast<int32_t*>	(_raw) )
#define r_u32(_raw)		( *reinterpret_cast<uint32_t*>	(_raw) )
#define r_64(_raw)		( *reinterpret_cast<int64_t*>	(_raw) )
#define r_u64(_raw)		( *reinterpret_cast<uint64_t*>	(_raw) )
#define r_single(_raw)	( *reinterpret_cast<float*>		(_raw) )
#define r_double(_raw)	( *reinterpret_cast<double*>	(_raw) )

	//Write macros
#define w_8	(_raw, data)		( *(reinterpret_cast<int8_t*>	(_raw))	= (data) )
#define w_u8(_raw, data)		( *(reinterpret_cast<uint8_t*>	(_raw))	= (data) )
#define w_16(_raw, data)		( *(reinterpret_cast<int16_t*>	(_raw))	= (data) )
#define w_u16(_raw, data)		( *(reinterpret_cast<uint16_t*>	(_raw))	= (data) )
#define w_32(_raw, data)		( *(reinterpret_cast<int32_t*>	(_raw))	= (data) )
#define w_u32(_raw, data)		( *(reinterpret_cast<uint32_t*>	(_raw))	= (data) )
#define w_64(_raw, data)		( *(reinterpret_cast<int64_t*>	(_raw))	= (data) )
#define w_u64(_raw, data)		( *(reinterpret_cast<uint64_t*>	(_raw))	= (data) )
#define w_single(_raw, data)	( *(reinterpret_cast<float*>	(_raw))	= (data) )
#define w_double(_raw, data)	( *(reinterpret_cast<double*>	(_raw))	= (data) )

	struct SGUID {
		union {
			uint32_t Value;
			struct {
				uint8_t  B1;
				uint8_t  B2;
				uint8_t  B3;
				uint8_t  B4;
			};
		};

		SGUID() noexcept : Value{ 0 } {}
		SGUID(const SGUID& Other) noexcept : Value{ Other.Value } {}
		SGUID(uint32_t Value) noexcept : Value{ Value } {}

		void operator=(const SGUID& Other) { Value = Other.Value; }

		inline static SGUID New() noexcept {
			SGUID SGuid;

			SGuid.B1 = (uint8_t)(std::rand() % UINT8_MAX);
			SGuid.B2 = (uint8_t)(std::rand() % UINT8_MAX);
			SGuid.B3 = (uint8_t)(std::rand() % UINT8_MAX);
			SGuid.B4 = (uint8_t)(std::rand() % UINT8_MAX);

			return SGuid;
		}
		inline static SGUID NewSimple() noexcept{
			return (uint32_t)(std::rand() % LONG_MAX);
		}

		const bool IsNone() const {
			return Value == None.Value;
		}
		const bool operator==(const SGUID Other) const noexcept {
			return Value == Other.Value;
		}
		const bool operator!=(const SGUID Other) const noexcept {
			return Value != Other.Value;
		}

		inline uint32_t GetRaw() const noexcept {
			return Value;
		}

		static const SGUID None;
	};

	//Efficient implentation of a spin lock
	class SpinLock {
	public:
		FORCEINLINE void Lock() noexcept
		{
			for (;;) {
				if (!bLock.exchange(true, std::memory_order_acquire)) {
					break;
				}
				while (bLock.load(std::memory_order_relaxed)) {
					std::atomic_signal_fence(std::memory_order_seq_cst);
					_mm_pause();
				}
			}
		}

		FORCEINLINE void Unlock() noexcept
		{
			bLock.store(false, std::memory_order_release);
		}

	private:
		std::atomic<bool> bLock = { false };
	};

	//RAII-based spin lock guard (scope guard)
	class SpinLockScopeGuard {
	public:
		FORCEINLINE explicit SpinLockScopeGuard(SpinLock* Lock) noexcept
			:Lock(Lock)
		{
			Lock->Lock();
		}

		FORCEINLINE ~SpinLockScopeGuard() noexcept
		{
			Lock->Unlock();
		}

	private:
		SpinLock* Lock;
	};
}
