// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "CoreInterop.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

class marshal_as_wchar
{
public:
	marshal_as_wchar(System::String^ s)
	{
		m_hGlobal = System::Runtime::InteropServices::Marshal::StringToHGlobalUni(s);
	}

	~marshal_as_wchar()
	{
		System::Runtime::InteropServices::Marshal::FreeHGlobal(m_hGlobal);
	}

	operator const wchar_t*() const
	{
		return m_hGlobal != System::IntPtr::Zero ? static_cast<const wchar_t*>(m_hGlobal.ToPointer()) : nullptr;
	}

private:
	mutable System::IntPtr m_hGlobal;
};

class marshal_as_char
{
public:
	marshal_as_char(System::String^ s)
	{
		m_hGlobal = System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(s);
	}

	~marshal_as_char()
	{
		System::Runtime::InteropServices::Marshal::FreeHGlobal(m_hGlobal);
	}

	operator const char*() const
	{
		return m_hGlobal != System::IntPtr::Zero ? static_cast<const char*>(m_hGlobal.ToPointer()) : nullptr;
	}

private:
	mutable System::IntPtr m_hGlobal;
};

class marshal_as_wstring
{
public:
	marshal_as_wstring(System::String^ s) : 
		m_String(s)
	{
	}

	operator FileCore::WString() const
	{
		const wchar_t* str = m_String;
		return str ? FileCore::WString(str) : FileCore::WString();
	}

private:
	marshal_as_wchar m_String;
};

class marshal_as_astring
{
public:
	marshal_as_astring(System::String^ s) : 
		m_String(s)
	{
	}

	operator FileCore::AString() const
	{
		const char* str = m_String;
		return str ? FileCore::AString(str) : FileCore::AString();
	}

private:
	marshal_as_char m_String;
};

class marshal_as_user_context
{
public:
	marshal_as_user_context(CoreInterop::UserContext^ c) :
		m_Valid(false),
		m_Context()
	{
		if (c != nullptr)
		{
			m_Valid = true;
			m_Context = c->ToNative();
		}
	}

	operator const FileCore::UserContext*() const
	{
		return m_Valid ? &m_Context : nullptr;
	}

	const FileCore::UserContext& Get() const
	{
		return m_Context;
	}

	FileCore::UserContext& Get()
	{
		return m_Context;
	}

private:
	bool m_Valid;
	FileCore::UserContext m_Context;
};

class marshal_as_gchandle
{
public:
	marshal_as_gchandle(System::Object^ obj)
	{
		m_hGlobal = System::Runtime::InteropServices::GCHandle::Alloc(obj);
	}

	~marshal_as_gchandle()
	{
		m_hGlobal.Free();
	}

	operator void*()
	{
		return System::Runtime::InteropServices::GCHandle::ToIntPtr(m_hGlobal).ToPointer();
	}

	System::Object^ Target()
	{
		return m_hGlobal.Target;
	}

private:
	System::Runtime::InteropServices::GCHandle m_hGlobal;
};

class marshal_as_logdevice : public FileCore::LogDevice
{
public:
	marshal_as_logdevice(CoreInterop::LogDevice^ log)
	{
		m_hGlobal = System::Runtime::InteropServices::GCHandle::Alloc(log);
	}

	~marshal_as_logdevice()
	{
		m_hGlobal.Free();
	}

	void Write(const FileCore::LogElement& element) override
	{
		CoreInterop::LogDevice^ log = ManagedDevice();
		if (log != nullptr)
		{
			log->Write(LogElement::FromNative(element)); 
		}
	}

	bool IsFaulted() override
	{
		CoreInterop::LogDevice^ log = ManagedDevice();
		return log != nullptr ? log->IsFaulted() : false;
	}

	CoreInterop::LogDevice^ ManagedDevice()
	{
		return safe_cast<CoreInterop::LogDevice^>(m_hGlobal.Target);
	}

private:
	System::Runtime::InteropServices::GCHandle m_hGlobal;
};

template <typename ManagedStructType, typename UnmanagedStructType>
inline ManagedStructType marshal_as_struct(const UnmanagedStructType& src)
{
	int dstSize = System::Runtime::InteropServices::Marshal::SizeOf(ManagedStructType::typeid);
	if (int(sizeof(UnmanagedStructType)) < dstSize)
		throw gcnew System::Exception("Invalid target size");

	return safe_cast<ManagedStructType>(
		System::Runtime::InteropServices::Marshal::PtrToStructure(System::IntPtr((void*)&src), ManagedStructType::typeid)
	);
}

inline System::DateTime
marshal_as_datetime(const FILETIME& fileTime)
{
	return System::DateTime::FromFileTime(FileCore::TimeInfo::FileTimeToUInt64(fileTime));

}

inline FILETIME
marshal_as_filetime(System::DateTime^ dateTime)
{
	return FileCore::TimeInfo::UInt64ToFileTime(static_cast<uint64_t>(dateTime->ToFileTime()));
}

struct Marshal
{
	template <typename StringArrayType>
	static array<System::String^>^ 
	FromNative(
		const StringArrayType& src
		);

	static FileCore::AStringArray
	ToNativeAnsi(
		array<System::String^>^ src
		);

	static FileCore::WStringArray
	ToNativeWide(
		array<System::String^>^ src
		);
};

}}}
