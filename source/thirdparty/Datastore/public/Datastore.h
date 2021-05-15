#pragma once

#define _HAS_EXCEPTIONS				0

#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <ostream>
#include <array>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <fvec.h>
#include <cstdio>
#include <cctype>
#include <codecvt>
#include <type_traits>
#include <memory>
#include <utility>
#include <random>
#include <filesystem>

#include <nlohmann/json.hpp>
#define json_lib nlohmann

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define DATACENTER_UE4_PLUGIN FALSE

#if !DATACENTER_UE4_PLUGIN
#define DATACENTER_UI FALSE
#else 
#define DATACENTER_UI FALSE
#endif

#if !DATACENTER_UE4_PLUGIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN TRUE
#endif
#include<Windows.h>

#endif

constexpr size_t			CElementsBlockSize = 65536;
constexpr size_t			CAttributesBlockSize = 65536;
constexpr size_t			CStringsBlockSize = 65536;

namespace Datacenter {
	struct Config {

#if DATACENTER_UI
		int32_t WindowWidth = 0;
		int32_t WindowHeight = 0;

		std::string DefaultKEY;
		std::string DefaultIV;
		std::string DefaultLoadDatacenterName;
		std::string DefaultSaveDatacenterName;
#endif

		std::vector<std::wstring> ElementNameComposeFromAttributesList;

		struct {
			std::string	ServerPath;
			std::string	ClientPath;
		} Datacenter;

		struct {
			std::string	ArbiterServer_XTD_Path;
			std::string	WorldServer_XTD_Path;
		} Type;

		bool Initialize(const char* CWD);

		std::string BuildSubPath(const char* Path) const {
			std::string Temp = PathBase;
			Temp += Path;

			return std::move(Temp);
		}

	private:
		std::string		PathBase;

	};

	extern Config GConfig;

	template<bool bForServer, bool bFor64>
	struct SerialSizesAndTypes {
		using BlockIndexType = uint16_t;
		using NameIndexType = typename std::conditional<bForServer, uint32_t, uint16_t>::type;

		using TBlockIndices = std::pair<BlockIndexType, BlockIndexType>;

		static constexpr size_t GetAttributeSerialSize() {
			if constexpr (!bForServer) {
				if constexpr (!bFor64) {
					return 8;
				}
				else {
					return 12;
				}
			}
			else {
				return
					sizeof(NameIndexType) +		//NameId
					sizeof(uint16_t) +			//TypeInfo
					sizeof(int64_t);			//Int64Value
			}
		}
		static constexpr size_t GetElementSerialSize() {
			if constexpr (!bForServer) {
				if constexpr (!bFor64) {
					return 16;
				}
				else {
					return 24;
				}
			}
			else {
				return
					sizeof(NameIndexType) +		//NameId
					sizeof(uint8_t) +			//IsValueElement
					sizeof(uint8_t) +			//IsCommentElement
					sizeof(TBlockIndices) +		//ValueIndices
					sizeof(uint16_t) +			//AttributesCount
					sizeof(uint16_t) +			//ChildCount
					sizeof(TBlockIndices) +		//AttributesIndices
					sizeof(TBlockIndices) +		//ChildrenIndices
					sizeof(TBlockIndices);		//FileNameStrIndices
			}
		}
	};
}

namespace Datacenter {
	// Initializes the Datacenter system and sets the CWD(current working directory/"bench" directory).
	// By default is seeds the rand function with the current time value.
	bool Initialize(const char* CWD, bool bSeedRand = true);

	enum class ServerAttributeType : uint16_t {
		Unknown,
		Int,
		Float,
		Double,
		Bool,
		String,

		IntList,
		FloatList,
		DoubleList,
		BoolList,
		StringList,
		Int64,
		FVector,
		IntPair
	};

	struct ServerTypeInfo {
		ServerAttributeType		Type;
		std::wstring			DefaultValue;
	};

	struct ServerDCInfo {
		std::unordered_map<std::wstring, ServerTypeInfo> ArbiterServerType;
		std::unordered_map<std::wstring, ServerTypeInfo> WorldServerType;

		bool LoadServerTypes(const char* arbiterServerFileName, const char* worldServerFileName);

	private:
		bool LoadServerTypes(const char* fileName, std::unordered_map<std::wstring, ServerTypeInfo>& outTypes);
	};

	extern ServerDCInfo GServerDCInfo;
}

#include "FIStream.h"
#include "Utils.h"
#include "Structure.h"
#include "Building.h"
