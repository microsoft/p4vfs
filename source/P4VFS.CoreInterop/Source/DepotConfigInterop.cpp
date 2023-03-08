// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotConfigInterop.h"
#include "CoreMarshal.h"

using namespace System::Collections::Generic;

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

System::String^ DepotConfig::ToString()
{
	List<System::String^> args = gcnew List<System::String^>();
	if (System::String::IsNullOrEmpty(Host) == false)
	{
		args.Add(System::String::Format("-H \"{0}\"", Host));
	}
	if (System::String::IsNullOrEmpty(Port) == false)
	{
		args.Add(System::String::Format("-p \"{0}\"", Port));
	}
	if (System::String::IsNullOrEmpty(Client) == false)
	{
		args.Add(System::String::Format("-c \"{0}\"", Client));
	}
	if (System::String::IsNullOrEmpty(User) == false)
	{
		args.Add(System::String::Format("-u \"{0}\"", User));
	}
	if (System::String::IsNullOrEmpty(Passwd) == false)
	{
		args.Add(System::String::Format("-P \"{0}\"", Passwd));
	}
	if (System::String::IsNullOrEmpty(Directory) == false)
	{
		args.Add(System::String::Format("-d \"{0}\"", Directory));
	}
	return System::String::Join(" ", args.ToArray());
}

P4::DepotConfig DepotConfig::ToNative()
{
	P4::DepotConfig dst;
	dst.m_Host = marshal_as_astring(Host);
	dst.m_Port = marshal_as_astring(Port);
	dst.m_Client = marshal_as_astring(Client);
	dst.m_User = marshal_as_astring(User);
	dst.m_Passwd = marshal_as_astring(Passwd);
	dst.m_Ignore = marshal_as_astring(Ignore);
	dst.m_Directory = marshal_as_astring(Directory);
	return dst;
}

DepotConfig^ DepotConfig::FromNative(const P4::DepotConfig& src)
{
	DepotConfig^ dst = gcnew DepotConfig();
	dst->Host = gcnew System::String(src.m_Host.c_str());
	dst->Port = gcnew System::String(src.m_Port.c_str());
	dst->Client = gcnew System::String(src.m_Client.c_str());
	dst->User = gcnew System::String(src.m_User.c_str());
	dst->Passwd = gcnew System::String(src.m_Passwd.c_str());
	dst->Ignore = gcnew System::String(src.m_Ignore.c_str());
	dst->Directory = gcnew System::String(src.m_Directory.c_str());
	return dst;
}

}}}

