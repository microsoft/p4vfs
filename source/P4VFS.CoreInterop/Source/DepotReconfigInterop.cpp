// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotReconfigInterop.h"
#include "CoreMarshal.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

DepotReconfigOptions::DepotReconfigOptions() :
	Files(nullptr),
	Flags(DepotReconfigFlags::None)
{
}

P4::FDepotReconfigOptions DepotReconfigOptions::ToNative()
{
	P4::FDepotReconfigOptions dst;
	dst.m_Flags = safe_cast<P4::DepotReconfigFlags::Enum>(Flags);

	if (Files != nullptr)
	{
		dst.m_Files.reserve(Files->Length);
		for each (System::String^ file in Files)
		{
			dst.m_Files.push_back(marshal_as_astring(file));
		}
	}
	return dst;
}

}}}

