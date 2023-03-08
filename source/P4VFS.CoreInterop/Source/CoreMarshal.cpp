// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "CoreMarshal.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

template <typename StringArrayType>
array<System::String^>^ 
Marshal::FromNative(
	const StringArrayType& src
	)
{
	array<System::String^>^ dst = gcnew array<System::String^>(System::Int32(src.size()));
	for (size_t i = 0; i < src.size(); ++i)
	{
		dst[System::Int32(i)] = gcnew System::String(src[i].c_str());
	}
	return dst;
}

template array<System::String^>^ Marshal::FromNative<>(const FileCore::AStringArray& src);
template array<System::String^>^ Marshal::FromNative<>(const FileCore::WStringArray& src);

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

