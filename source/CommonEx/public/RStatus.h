#pragma once
/**
 * @file RStatus.h
 *
 * @brief Simple Result Status abstraction
 *
 * @author Balan Narcis
 * Contact: balannarcis96@gmail.com
 *
 */

namespace CommonEx {
	enum class RStatus : int32_t {
		WorkRemains = -1,
		Success = 0,
		Fail = 1,
		Timedout = 2,
		AlreadyPerformed = 3,
		NotImplemented = 4,
		AcquireFailed = 5,
		ConnectionLost = 6,
		Aborted = 7,

		//Custom

		MAX
	};

	constexpr RStatus RSuccess{ RStatus::Success };
	constexpr RStatus RFail{ RStatus::Fail };
	constexpr RStatus RTimedout{ RStatus::Timedout };
	constexpr RStatus RNotImplemented{ RStatus::NotImplemented };
	constexpr RStatus RWorkRemains{ RStatus::WorkRemains };
	constexpr RStatus RAcquireFailed{ RStatus::AcquireFailed };
	constexpr RStatus RConnectionLost{ RStatus::ConnectionLost };
	constexpr RStatus RAborted{ RStatus::Aborted };

	inline bool operator!(const RStatus& Status) noexcept {
		return Status != RSuccess;
	}
}