#include "../public/Datastore.h"

using namespace Datacenter;

//Init
bool Datacenter::Initialize(const char* CWD, bool bSeedRand)
{
	Utils::HashUtil::Initialize();

	if (!GConfig.Initialize(CWD)) {
		Message("Datacenter:: Failed to load the Config!");
		return false;
	}

	if(!GServerDCInfo.LoadServerTypes(
		GConfig.Type.ArbiterServer_XTD_Path.c_str(),
		GConfig.Type.WorldServer_XTD_Path.c_str())){
		Message("Failed to load server dc types!");
		return false;
	}

	if (bSeedRand) {
		srand((uint32_t)time(NULL));
	}

	return true;
}

//FIStream
FIStream::FIStream()
{
	_raw = 0;
	_size = 0;
	_pos = 0;
}

FIStream::FIStream(std::istream* str)
{
	str->seekg(0, std::istream::end);
	_size = (uint64_t)str->tellg();
	str->seekg(0, std::istream::beg);

	_raw = new uint8_t[_size];
	str->read((char*)_raw, _size);
	_pos = 0;
}

FIStream::FIStream(uint64_t size)
{
	_size = size;
	_raw = (uint8_t*)malloc(size);
	memset((void*)_raw, 0, _size);
	_pos = 0;
}

FIStream::FIStream(uint8_t* data, uint64_t size)
{
	_size = size;
	_pos = 0;
	_raw = new uint8_t[_size];
	if (memcpy_s((void*)_raw, _size, (const void*)data, _size) != 0)
	{
		if (_raw)
		{
			delete[] _raw;
			_raw = 0;

		}
		return;
	}
}

FIStream::FIStream(FIStream& d)
{
	_size = _pos = 0;
	_raw = nullptr;

	Resize(d._size);
	Write(d._raw, d._size);
}

FIStream::~FIStream()
{
	if (_raw)
	{
		delete[] _raw;
		_raw = 0;
	}
	_size = 0;
	_pos = 0;
}

bool FIStream::load_from_file(const char* file, size_t addSlack)
{
	Clear();
	std::ifstream f = std::ifstream(file, std::fstream::binary);
	if (!f.is_open())
		return  false;
	if (!f.good()) {
		f.close();
		return  false;
	}

	f.seekg(0, std::fstream::end);
	_size = (uint64_t)f.tellg();
	f.seekg(0, std::fstream::beg);


	_raw = new uint8_t[_size + addSlack];
	if (!_raw) {
		f.close();
		return false;
	}

	f.read((char*)_raw, _size);
	f.close();

	_size += addSlack;
	return true;
}

bool FIStream::load_from_file(const wchar_t* file)
{
	std::wstring TFile = file;

	char buffer[2048];

	if (_WideCharToMultiByte(file, buffer, 2048)) {
		return load_from_file(buffer);
	}

	return false;
}

bool FIStream::save_to_file(const char* fileName)
{
	std::ofstream file;
	file.open(fileName, std::ofstream::binary);
	if (!file.is_open()) {
		char errorBuffer[1024];
		strerror_s(errorBuffer, errno);
		Message("FIStream::ofstream::open error[%s]", errorBuffer);
		return false;
	}

	file.write((const char*)_raw, _pos);

	file.close();

	return true;
}

bool FIStream::save_to_file(const wchar_t* fileName)
{
	std::wofstream file;
	file.open(fileName, std::wofstream::binary | std::wofstream::out);
	if (!file.is_open()) {
		char errorBuffer[1024];
		strerror_s(errorBuffer, errno);
		Message("FIStream::ofstream::open error[%s]", errorBuffer);
		return false;
	}

	file.write((const wchar_t*)_raw, _pos / 2);

	file.close();

	return true;
}

//WRITE_*******************************************************
void FIStream::Resize(uint64_t size)
{
	if (size <= 0 || _size + size > UINT64_MAX)
		return;

	if (!_raw)
	{
		_raw = new uint8_t[size];
		_size = size;
		memset((void*)_raw, 0, _size);
	}
	else
	{
		uint64_t newSize = _size + size;
		uint8_t* newRaw = (uint8_t*)realloc((void*)_raw, newSize);

		memset((void*)&newRaw[_size], 0, size);
		_size = newSize;
		_raw = newRaw;
	}
}

void FIStream::Write(const uint8_t* data, uint64_t toWriteSize)
{
	if (_size + toWriteSize > UINT64_MAX) //0xffff because the packets header has only 2 bytes [00 00] for the size
		return;

	int64_t allocSzie = toWriteSize - (_size + _pos);

	if (allocSzie > 0)
		Resize(allocSzie);

	if (memcpy_s((void*)&_raw[_pos], toWriteSize, (const void*)data, toWriteSize) != 0) return;


	if (allocSzie > 0)
	{
		_pos = _size;
	}
	else {
		_pos += toWriteSize;
	}
}

void FIStream::WriteUInt8(uint8_t val)
{
	Write(&val, 1);
}

void FIStream::WriteInt16(int16_t data)
{
	uint8_t shortBytes[2];
	shortBytes[0] = (data & 0xff);
	shortBytes[1] = ((data >> 8) & 0xff);

	Write(shortBytes, 2);
}

void FIStream::WriteUInt16(uint16_t val)
{
	uint8_t shortBytes[2];
	shortBytes[0] = (val & 0xff);
	shortBytes[1] = ((val >> 8) & 0xff);
	Write(shortBytes, 2);
}

void FIStream::WriteInt32(int32_t data)
{
	uint8_t intBytes[4];
	intBytes[0] = data & 0x000000ff;
	intBytes[1] = (data & 0x0000ff00) >> 8;
	intBytes[2] = (data & 0x00ff0000) >> 16;
	intBytes[3] = (data & 0xff000000) >> 24;

	Write(intBytes, 4);
}

void FIStream::WriteUInt32(int32_t data)
{
	uint8_t intBytes[4];
	intBytes[0] = data & 0x000000ff;
	intBytes[1] = (data & 0x0000ff00) >> 8;
	intBytes[2] = (data & 0x00ff0000) >> 16;
	intBytes[3] = (data & 0xff000000) >> 24;

	Write(intBytes, 4);
}

void FIStream::Write_int64(int64_t data)
{
	uint8_t intBytes[8];

	intBytes[0] = ((data >> 0) & 0xff);
	intBytes[1] = ((data >> 8) & 0xff);
	intBytes[2] = ((data >> 16) & 0xff);
	intBytes[3] = ((data >> 24) & 0xff);
	intBytes[4] = ((data >> 32) & 0xff);
	intBytes[5] = ((data >> 40) & 0xff);
	intBytes[6] = ((data >> 48) & 0xff);
	intBytes[7] = ((data >> 56) & 0xff);

	Write(intBytes, 8);
}

void FIStream::WriteU_int64(uint64_t data)
{
	uint8_t intBytes[8];

	intBytes[0] = ((data >> 0) & 0xff);
	intBytes[1] = ((data >> 8) & 0xff);
	intBytes[2] = ((data >> 16) & 0xff);
	intBytes[3] = ((data >> 24) & 0xff);
	intBytes[4] = ((data >> 32) & 0xff);
	intBytes[5] = ((data >> 40) & 0xff);
	intBytes[6] = ((data >> 48) & 0xff);
	intBytes[7] = ((data >> 56) & 0xff);

	Write(intBytes, 8);
}

void FIStream::WriteFloat(float val)
{
	Write(reinterpret_cast<uint8_t*>(&val), 4);
}

void FIStream::WriteDouble(double val)
{
	uint8_t* doubleBytes = reinterpret_cast<uint8_t*>(&val);
	Write(doubleBytes, 4);
}

void FIStream::WritePos(uint16_t s, int16_t offset)
{
	uint64_t temp = _pos;
	_pos = s;
	WriteInt16((int16_t)(temp + offset));
	_pos = temp;
}

//READ_********************************************************
void FIStream::Read(uint8_t* out_buffer, uint64_t size)
{
	if (size > _size - _pos)
	{
		int64_t temp = (_size - _pos);

		memcpy_s((void*)out_buffer, size, (const void*)&_raw[_pos], temp);
		_pos = temp;

		return;
	}
	memcpy_s((void*)out_buffer, size, (const void*)&_raw[_pos], size);
	_pos += size;
}

char FIStream::ReadInt8()
{
	return _raw[_pos++];
}

uint8_t FIStream::ReadUInt8()
{
	return _raw[_pos++];
}

int16_t FIStream::ReadInt16()
{
	if (_pos > _size - 2)
		return -1;

	int16_t out = (_raw[_pos + 1] << 8) | _raw[_pos];
	_pos += 2;
	return out;
}

uint16_t FIStream::ReadUInt16()
{
	if (_pos > _size - 2)
		return -1;

	uint16_t out = (_raw[_pos + 1] << 8) | _raw[_pos];
	_pos += 2;
	return out;
}

int32_t FIStream::ReadInt32()
{
	if (_size - 4 < _pos)
		return INT_MAX;

	int32_t out = ((_raw[_pos + 3] << 24) | (_raw[_pos + 2] << 16) | (_raw[_pos + 1] << 8) | _raw[_pos]);
	_pos += 4;

	return out;
}

uint32_t FIStream::ReadUInt32()
{
	if (_size - 4 < _pos)
		return INT_MAX;

	uint32_t out = ((_raw[_pos + 3] << 24) | (_raw[_pos + 2] << 16) | (_raw[_pos + 1] << 8) | _raw[_pos]);
	_pos += 4;

	return out;
}

int64_t FIStream::ReadInt64()
{
	if (_size < _pos + 8)
		return INT64_MAX;

	int64_t l = 0;
	for (uint8_t i = 0; i < 8; i++) {
		l = l | ((unsigned long long)_raw[_pos + i] << (8 * i));
	}
	_pos += 8;
	return l;
}

uint64_t FIStream::ReadUInt64()
{
	if (_size < _pos + 8)
		return INT64_MAX;

	uint64_t l = 0;
	for (uint8_t i = 0; i < 8; i++) {
		l = l | ((unsigned long long)_raw[_pos + i] << (8 * i));
	}
	_pos += 8;
	return l;
}

float FIStream::ReadFloat()
{
	if (_size < _pos + 4)
		return FLT_MAX;

	float out = *(float*)(&_raw[_pos]);
	_pos += 4;
	return out;
}

double FIStream::ReadDouble()
{
	if (_size < _pos + 4)
		return FLT_MAX;

	double out = *(double*)(&_raw[_pos]);
	_pos += 4;
	return out;
}

//MISC_**************************************************
void FIStream::Clear()
{
	if (_raw)
	{
		delete[] _raw;
		_raw = 0;
	}
	_size = 0;
	_pos = 0;
}

void FIStream::Zero() {
	memset(_raw, 0, sizeof(uint8_t) * _size);
}

uint64_t FIStream::SetEnd()
{
	_pos = _size;
	return _pos;
}

uint64_t FIStream::SetFront()
{
	_pos = 0;
	return 0;
}

uint8_t* FIStream::GetPtr()
{
	if (!_raw) {
		return nullptr;
	}

	return &_raw[_pos];
}

//HashUtil
uint32_t Utils::HashUtil::GCRCTable[256];

//SGUID
const Utils::SGUID Utils::SGUID::None;

Utils::SGUID Utils::SGUID::New() {
	SGUID SGuid;

	SGuid.B1 = (uint8_t)(rand() % UINT8_MAX);
	SGuid.B2 = (uint8_t)(rand() % UINT8_MAX);
	SGuid.B3 = (uint8_t)(rand() % UINT8_MAX);
	SGuid.B4 = (uint8_t)(rand() % UINT8_MAX);

	return SGuid;
}
Utils::SGUID Utils::SGUID::NewSimple() {
	return (uint32_t)rand() % UINT32_MAX;
}

//Config
bool Config::Initialize(const char* CWD)
{
	PathBase = CWD;

	std::fstream file = std::fstream(BuildSubPath("Config.json").c_str());
	if (!file.is_open()) {
		Message("Failed to open config.json");
		return false;
	}

	json_lib::json j;
	file >> j;
	file.close();

#if DATACENTER_UI
	WindowWidth = j["Window"]["width"];
	WindowHeight = j["Window"]["height"];

	DefaultKEY = j["Settings"]["DefaultKEY"];
	DefaultIV = j["Settings"]["DefaultIV"];
	DefaultLoadDatacenterName = j["Settings"]["DefaultLoadDatacenterName"];
	DefaultSaveDatacenterName = j["Settings"]["DefaultSaveDatacenterName"];
#endif

	std::vector<std::string> list = j["ElementNameComposeFromAttributesList"];

	for (auto& item : list)
	{
		ElementNameComposeFromAttributesList.emplace_back(item.begin(), item.end());
	}

	Datacenter.ServerPath = j["Datacenter"]["ServerPath"];
	Datacenter.ClientPath = j["Datacenter"]["ClientPath"];

	Datacenter.ServerPath = BuildSubPath(Datacenter.ServerPath.c_str());
	Datacenter.ClientPath = BuildSubPath(Datacenter.ClientPath.c_str());

	Type.ArbiterServer_XTD_Path = j["Type"]["ArbiterServer_XTD_Path"];
	Type.WorldServer_XTD_Path = j["Type"]["WorldServer_XTD_Path"];

	Type.ArbiterServer_XTD_Path = BuildSubPath(Type.ArbiterServer_XTD_Path.c_str());
	Type.WorldServer_XTD_Path = BuildSubPath(Type.WorldServer_XTD_Path.c_str());

	return true;
}

bool Datacenter::ServerDCInfo::LoadServerTypes(const char* arbiterServerFileName, const char* worldServerFileName)
{
	if(!LoadServerTypes(arbiterServerFileName, ArbiterServerType)){
		return false;
	}

	if (!LoadServerTypes(worldServerFileName, WorldServerType)) {
		return false;
	}

	Message("Loaded %lld ArbiterServer DC types and %lld WorldServer DC types", ArbiterServerType.size(), WorldServerType.size());

	return true;
}

bool Datacenter::ServerDCInfo::LoadServerTypes(const char* fileName, std::unordered_map<std::wstring, ServerTypeInfo>& outTypes)
{
	static wchar_t Utf8ToUtf16Buffer[4096 * 2];

	std::ifstream file = std::ifstream(fileName);
	if (!file.is_open()) {
		return false;
	}

	std::string line;
	while (std::getline(file, line)) {
		if (line.find_first_of('#') != std::string::npos) {
			//skipp
			continue;
		}

		if (!_MultiByteToWideChar(line.c_str(), Utf8ToUtf16Buffer, 4096 * 2)) {
			Message("ServerDCInfo::LoadServerTypes failed to _MultiByteToWideChar!");
			continue;
		}

		auto parts = Utils::SplitString<std::wstring>(Utf8ToUtf16Buffer, L"@", true, false);
		if (parts.size() < 2) {
			continue;
		}

		auto& elementName = parts[0];

		parts = Utils::SplitString<std::wstring>(parts[1], L":", false, false);
		if (parts.size() < 1) {
			continue;
		}

		Utils::Trim<std::wstring>(parts[0]);
		Utils::Trim<std::wstring>(parts[1]);
		Utils::Trim<std::wstring>(parts[2]);

		auto attributeName = std::wstring(parts[0].begin(), parts[0].end());

		elementName += L"@";
		elementName += attributeName;

		auto typeName = parts[1];
		auto defaultValue = std::wstring(parts[2].begin(), parts[2].end());

		ServerAttributeType attributeType = ServerAttributeType::Unknown;

		if (!_wcsicmp(typeName.c_str(), L"int")) {
			attributeType = ServerAttributeType::Int;
		}
		else if (!_wcsicmp(typeName.c_str(), L"float")) {
			attributeType = ServerAttributeType::Float;
		}
		else if (!_wcsicmp(typeName.c_str(), L"double")) {
			attributeType = ServerAttributeType::Double;
		}
		else if (!_wcsicmp(typeName.c_str(), L"bool")) {
			attributeType = ServerAttributeType::Bool;
		}
		else if (!_wcsicmp(typeName.c_str(), L"string")) {
			attributeType = ServerAttributeType::String;
		}
		else if (!_wcsicmp(typeName.c_str(), L"int-list")) {
			attributeType = ServerAttributeType::IntList;
		}
		else if (!_wcsicmp(typeName.c_str(), L"float-list")) {
			attributeType = ServerAttributeType::FloatList;
		}
		else if (!_wcsicmp(typeName.c_str(), L"double-list")) {
			attributeType = ServerAttributeType::DoubleList;
		}
		else if (!_wcsicmp(typeName.c_str(), L"bool-list")) {
			attributeType = ServerAttributeType::BoolList;
		}
		else if (!_wcsicmp(typeName.c_str(), L"string-list")) {
			attributeType = ServerAttributeType::StringList;
		}
		else if (!_wcsicmp(typeName.c_str(), L"fvector")) {
			attributeType = ServerAttributeType::FVector;
		}
		else if (!_wcsicmp(typeName.c_str(), L"__int64")) {
			attributeType = ServerAttributeType::Int64;
		}
		else if (!_wcsicmp(typeName.c_str(), L"int-pair")) {
			attributeType = ServerAttributeType::IntPair;
		}
		else{
			Message("Unknwon type [%s] in the Server type file!", typeName.c_str());
		}

		ServerTypeInfo TypeInfo;
		TypeInfo.DefaultValue = std::move(defaultValue);
		TypeInfo.Type = attributeType;

		outTypes.insert({
			std::move(elementName),
			TypeInfo
			});
	}

	file.close();

	return true;
}

Datacenter::ServerDCInfo Datacenter::GServerDCInfo;
Datacenter::Config Datacenter::GConfig;

