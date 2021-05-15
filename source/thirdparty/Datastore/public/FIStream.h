#pragma once

namespace Datacenter {
	struct FIStream
	{
		FIStream();
		FIStream(uint64_t size);
		FIStream(uint8_t* data, uint64_t size);
		FIStream(std::istream* str);

		FIStream(FIStream&);
		~FIStream();

		bool				load_from_file(const char*, size_t addSlack = 0);
		bool				load_from_file(const wchar_t* file);
		bool				save_to_file(const char*);
		bool				save_to_file(const wchar_t* fileName);

		//WRITE_*******************************************************
		void				Resize(uint64_t size);

		void				Write(const uint8_t* data, uint64_t size);

		void				WriteUInt8(uint8_t);

		void				WriteInt16(int16_t);
		void				WriteUInt16(uint16_t);

		void				WriteInt32(int32_t);
		void				WriteUInt32(int32_t);

		void				Write_int64(_int64);
		void				WriteU_int64(uint64_t);

		void				WriteFloat(float);
		void				WriteDouble(double);

		void				WritePos(uint16_t, int16_t offset = 0);

		//READ_********************************************************
		void				Read(uint8_t* out_buffer, uint64_t size);

		char				ReadInt8();
		uint8_t				ReadUInt8();

		int16_t				ReadInt16();
		uint16_t			ReadUInt16();

		int32_t				ReadInt32();
		uint32_t			ReadUInt32();

		_int64				ReadInt64();
		uint64_t			ReadUInt64();

		float				ReadFloat();
		double				ReadDouble();

		bool IsLoading()const {
			return _isLoading;
		}

		template<typename T>
		inline T ReadT() noexcept {
			_pos += sizeof(T);
			return *(T*)(GetPtr() - sizeof(T));
		}

		template<typename T>
		inline void WriteT(const T& Value) noexcept {
			*((T*)GetPtr()) = Value;
			_pos += sizeof(T);
		}

		//MISC_**************************************************
		void				Clear();
		void				Zero();
		uint64_t			SetEnd();
		uint64_t			SetFront();

		uint8_t* GetPtr();

		//MEMBER_VAR_**************************************************
		uint8_t* _raw;
		uint64_t				_size;
		uint64_t				_pos;
		bool					_isLoading = true;
	};
}