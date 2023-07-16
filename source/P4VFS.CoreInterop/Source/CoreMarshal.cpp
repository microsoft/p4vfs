// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "CoreMarshal.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

template <typename StringArrayType, typename TransformFunction>
static array<System::String^>^ 
FromNativeStringArray(
	const StringArrayType& src,
	TransformFunction transform
	)
{
	array<System::String^>^ dst = gcnew array<System::String^>(System::Int32(src.size()));
	for (size_t i = 0; i < src.size(); ++i)
	{
		dst[System::Int32(i)] = transform(src[i].c_str());
	}
	return dst;
}

array<System::String^>^ 
Marshal::FromNativeAnsi(
	const FileCore::AStringArray& src
	)
{
	return FromNativeStringArray(src, [](const char* s) -> System::String^ { return Marshal::FromNativeAnsi(s); });
}

array<System::String^>^ 
Marshal::FromNativeWide(
	const FileCore::WStringArray& src
	)
{
	return FromNativeStringArray(src, [](const wchar_t* s) -> System::String^ { return Marshal::FromNativeWide(s); });
}

System::String^
Marshal::FromNativeAnsi(
	const char* src
	)
{
	return src ? FromNativeWide(CSTR_ATOW(src)) : nullptr;
}

System::String^
Marshal::FromNativeWide(
	const wchar_t* src
	)
{
	return src ? gcnew System::String(src) : nullptr;
}

FileCore::AStringArray
Marshal::ToNativeAnsi(
	array<System::String^>^ src
	)
{
	FileCore::AStringArray dst;
	if (src != nullptr)
	{
		for each (System::String^ i in src)
		{
			dst.push_back(marshal_as_astring(i));
		}
	}
	return dst;
}

FileCore::WStringArray
Marshal::ToNativeWide(
	array<System::String^>^ src
	)
{
	FileCore::WStringArray dst;
	if (src != nullptr)
	{
		for each (System::String^ i in src)
		{
			dst.push_back(marshal_as_wstring(i));
		}
	}
	return dst;
}

}}}

