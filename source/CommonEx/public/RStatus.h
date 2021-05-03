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

namespace CommonEx
{
	enum class RStatus : int32_t
	{
		Success = 0
		, Fail
		, WorkRemains
		, Timedout
		, AlreadyPerformed
		, NotImplemented
		, AcquireFailed
		, ConnectionLost
		, Aborted
		, InvalidParameters
		, OperationOverflows
		, ThreadIsOwnedByTheGroup

		//Custom

		, MAX
	};

	constexpr RStatus RSuccess{ RStatus::Success };
	constexpr RStatus RFail{ RStatus::Fail };
	constexpr RStatus RTimedout{ RStatus::Timedout };
	constexpr RStatus RNotImplemented{ RStatus::NotImplemented };
	constexpr RStatus RWorkRemains{ RStatus::WorkRemains };
	constexpr RStatus RAcquireFailed{ RStatus::AcquireFailed };
	constexpr RStatus RConnectionLost{ RStatus::ConnectionLost };
	constexpr RStatus RAborted{ RStatus::Aborted };
	constexpr RStatus RInvalidParameters{ RStatus::InvalidParameters };
	constexpr RStatus ROperationOverflows{ RStatus::OperationOverflows };

	inline bool operator!(const RStatus& Status) noexcept
	{
		return Status != RSuccess;
	}
}