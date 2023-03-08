// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotClientInterop.h"
#include "DepotClient.h"
#include "CoreMarshal.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

DepotCommand::DepotCommand(
	)
{
}

P4::DepotCommand 
DepotCommand::ToNative(
	)
{
	P4::DepotCommand dst;
	dst.m_Name = marshal_as_astring(Name);
	dst.m_Input = marshal_as_astring(Input);
	dst.m_Prompt = marshal_as_astring(Prompt);
	dst.m_Flags = safe_cast<P4::DepotCommand::Flags::Enum>(Flags);
	dst.m_Args = Marshal::ToNativeAnsi(Args);
	return dst;
}

struct FDepotClientData
{
	P4::DepotClient m_DepotClient;
	FileCore::FileContext m_FileContext;
	std::unique_ptr<marshal_as_logdevice> m_ManagedLogDevice;
	std::unique_ptr<marshal_as_user_context> m_ManagedUserContext;
};

DepotClient::DepotClient(
	) : 
	DepotClient(nullptr)
{
}

DepotClient::DepotClient(
	FileContext^ fileContext
	)
{
	m_Data = new FDepotClientData;
	if (fileContext != nullptr)
	{
		if (fileContext->LogDevice != nullptr)
		{
			m_Data->m_ManagedLogDevice.reset(new marshal_as_logdevice(fileContext->LogDevice));
			m_Data->m_FileContext.m_LogDevice = m_Data->m_ManagedLogDevice.get();
		}
		if (fileContext->UserContext != nullptr)
		{
			m_Data->m_ManagedUserContext.reset(new marshal_as_user_context(fileContext->UserContext));
			m_Data->m_FileContext.m_UserContext = &m_Data->m_ManagedUserContext.get()->Get();
		}
	}
	else
	{
		m_Data->m_ManagedUserContext.reset(new marshal_as_user_context(UserContext::CurrentProcess()));
		m_Data->m_FileContext.m_UserContext = &m_Data->m_ManagedUserContext.get()->Get();
		m_Data->m_FileContext.m_LogDevice = &FileCore::LogSystem::StaticInstance();
	}
	m_Data->m_DepotClient = P4::FDepotClient::New(&m_Data->m_FileContext);
}

DepotClient::~DepotClient(
	) 
{ 
	this->!DepotClient(); 
}

DepotClient::!DepotClient(
	)
{
	if (m_Data != nullptr)
	{
		delete m_Data;
		m_Data = nullptr;
	}
}

bool 
DepotClient::Connect(
	DepotConfig^ config
	)
{
	ThrowIfObjectDisposed();
	return m_Data->m_DepotClient->Connect(config->ToNative());
}

void 
DepotClient::Disconnect(
	)
{
	ThrowIfObjectDisposed();
	m_Data->m_DepotClient->Disconnect();
}

bool 
DepotClient::IsConnected(
	)
{
	ThrowIfObjectDisposed();
	return m_Data->m_DepotClient->IsConnected();
}

bool 
DepotClient::IsLoginRequired(
	)
{
	ThrowIfObjectDisposed();
	return m_Data->m_DepotClient->IsLoginRequired();
}

bool 
DepotClient::Login(
	System::String^ passwd
	)
{
	ThrowIfObjectDisposed();
	return m_Data->m_DepotClient->Login(marshal_as_astring(passwd));
}

bool 
DepotClient::Login(
	)
{
	ThrowIfObjectDisposed();
	return m_Data->m_DepotClient->Login();
}

bool 
DepotClient::IsFaulted(
	)
{
	ThrowIfObjectDisposed();
	return m_Data->m_DepotClient->IsFaulted();
}

bool 
DepotClient::HasError(
	)
{
	ThrowIfObjectDisposed();
	return m_Data->m_DepotClient->HasError();
}

bool 
DepotClient::DisableLogin::get(
	)
{
	ThrowIfObjectDisposed();
	return m_Data->m_DepotClient->HasFlag(P4::FDepotClient::Flags::DisableLogin);
}

void 
DepotClient::DisableLogin::set(
	bool value
	)
{
	ThrowIfObjectDisposed();
	return m_Data->m_DepotClient->ApplyFlag(P4::FDepotClient::Flags::DisableLogin, value);
}

bool 
DepotClient::Unattended::get(
	)
{
	ThrowIfObjectDisposed();
	return m_Data->m_DepotClient->HasFlag(P4::FDepotClient::Flags::Unattended);
}

void 
DepotClient::Unattended::set(
	bool value
	)
{
	ThrowIfObjectDisposed();
	return m_Data->m_DepotClient->ApplyFlag(P4::FDepotClient::Flags::Unattended, value);
}

bool 
DepotClient::Unimpersonated::get(
	)
{
	ThrowIfObjectDisposed();
	return m_Data->m_DepotClient->HasFlag(P4::FDepotClient::Flags::Unimpersonated);
}

void 
DepotClient::Unimpersonated::set(
	bool value
	)
{
	ThrowIfObjectDisposed();
	return m_Data->m_DepotClient->ApplyFlag(P4::FDepotClient::Flags::Unimpersonated, value);
}

bool 
DepotClient::SetEnv(
	System::String^ name, 
	System::String^ value
	)
{
	ThrowIfObjectDisposed();
	return m_Data->m_DepotClient->SetEnv(marshal_as_char(name), marshal_as_char(value));
}

System::String^ 
DepotClient::GetEnv(
	System::String^ name
	)
{
	ThrowIfObjectDisposed();
	return gcnew System::String(m_Data->m_DepotClient->GetEnv(marshal_as_char(name)).c_str());
}

System::String^ 
DepotClient::GetErrorText(
	)
{
	ThrowIfObjectDisposed();
	return gcnew System::String(m_Data->m_DepotClient->GetErrorText().c_str());
}

DepotResult^ 
DepotClient::Connection(
	)
{
	ThrowIfObjectDisposed();
	return DepotResult::FromNative(m_Data->m_DepotClient->Connection());
}

DepotConfig^ 
DepotClient::Config(
	)
{
	ThrowIfObjectDisposed();
	return DepotConfig::FromNative(m_Data->m_DepotClient->Config());
}

DepotResult^ 
DepotClient::Run(
	DepotCommand^ cmd
	)
{
	ThrowIfObjectDisposed();
	return DepotResult::FromNative(m_Data->m_DepotClient->Run(cmd->ToNative()));
}

P4::DepotClient 
DepotClient::ToNative(
	)
{
	ThrowIfObjectDisposed();
	return m_Data->m_DepotClient;
}

void 
DepotClient::ThrowIfObjectDisposed(
	)
{
	if (m_Data == nullptr)
	{
		throw gcnew System::ObjectDisposedException("Microsoft::P4VFS::P4::DepotClient");
	}
}

System::Int64 DepotInfo::StringToTime(System::String^ text)
{
	return P4::DepotInfo::StringToTime(marshal_as_astring(text));
}

System::String^ 
DepotInfo::TimeToString(
	System::Int64 time
	)
{
	return gcnew System::String(P4::DepotInfo::TimeToString(time).c_str());
}

bool 
DepotInfo::IsWritableFileType(
	System::String^ fileType
	)
{
	return P4::DepotInfo::IsWritableFileType(marshal_as_astring(fileType));
}

bool 
DepotInfo::IsSymlinkFileType(
	System::String^ fileType
	)
{
	return P4::DepotInfo::IsSymlinkFileType(marshal_as_astring(fileType));
}

DepotConfig^ 
DepotInfo::DepotConfigFromPath(
	System::String^ folderPath
	)
{
	return DepotConfig::FromNative(P4::DepotInfo::DepotConfigFromPath(marshal_as_astring(folderPath)));
}

System::Int32 
DepotTunable::Get(
	System::String^ name
	)
{
	return P4::DepotTunable::Get(marshal_as_astring(name));
}

System::Int32 
DepotTunable::Get(
	System::String^ name, 
	System::Int32 defaultValue
	)
{
	return P4::DepotTunable::Get(marshal_as_astring(name), defaultValue);
}

void 
DepotTunable::Set(
	System::String^ name, 
	System::Int32 value
	)
{
	P4::DepotTunable::Set(marshal_as_astring(name), value);
}

void 
DepotTunable::Unset(
	System::String^ name
	)
{
	P4::DepotTunable::Unset(marshal_as_astring(name));
}

bool 
DepotTunable::IsSet(
	System::String^ name
	)
{
	return P4::DepotTunable::IsSet(marshal_as_astring(name));
}

bool 
DepotTunable::IsKnown(
	System::String^ name
	)
{
	return P4::DepotTunable::IsKnown(marshal_as_astring(name));
}

}}}

