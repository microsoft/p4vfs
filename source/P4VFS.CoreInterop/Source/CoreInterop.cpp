// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "CoreInterop.h"
#include "FileCore.h"
#include "FileContext.h"
#include "CoreMarshal.h"
#include "FileOperations.h"
#include "FileInterop.h"

using namespace msclr::interop;

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

FileCore::UserContext
UserContext::ToNative(
	)
{
	FileCore::UserContext context;
	context.m_SessionId = SessionId;
	context.m_ProcessId = ProcessId;
	context.m_ThreadId = ThreadId;
	return context;
}
	
UserContext^
UserContext::FromNative(
	const FileCore::UserContext& src
	)
{
	UserContext^ context = gcnew UserContext();
	context->SessionId = src.m_SessionId;
	context->ProcessId = src.m_ProcessId;
	context->ThreadId = src.m_ThreadId;
	return context;
}

UserContext^
UserContext::CurrentProcess(
	)
{
	UserContext^ context = gcnew UserContext();
	context->ProcessId = GetCurrentProcessId();
	context->ThreadId = GetCurrentThreadId();
	return context;
}

FileCore::LogElement
LogElement::ToNative(
	)
{
	FileCore::LogElement dst;
	dst.m_Channel = static_cast<FileCore::LogChannel::Enum>(m_Channel);
	dst.m_Text = marshal_as_wstring(m_Text);
	dst.m_Time = m_Time;
	return dst;
}
	
LogElement^ 
LogElement::FromNative(
	const FileCore::LogElement& src
	)
{
	LogElement^ dst = gcnew LogElement();
	dst->m_Channel = safe_cast<LogChannel>(src.m_Channel);
	dst->m_Text = gcnew System::String(src.m_Text.c_str());
	dst->m_Time = src.m_Time;
	return dst;
}

System::Int64
LogElement::Now(
	)
{
	return FileCore::TimeInfo::GetTime();
}

void 
LogDevice::Write(
	LogElement^ element
	)
{
}

bool 
LogDevice::IsFaulted(
	)
{
	return false;
}

void
LogSystem::Write(
	LogElement^ element
	)
{
	if (element != nullptr)
	{
		FileCore::LogSystem::StaticInstance().Write(element->ToNative());
	}
}

bool
LogSystem::IsPending(
	)
{
	return FileCore::LogSystem::StaticInstance().IsPending();
}

void
LogSystem::Flush(
	)
{
	return FileCore::LogSystem::StaticInstance().Flush();
}

void
LogSystem::Suspend(
	)
{
	return FileCore::LogSystem::StaticInstance().Suspend();
}

void
LogSystem::Resume(
	)
{
	return FileCore::LogSystem::StaticInstance().Resume();
}

void
LogSystem::Initialize(
	UserContext^ context
	)
{
	FileCore::LogSystem::StaticInstance().Initialize(marshal_as_user_context(context));
}

void
LogSystem::Shutdown(
	)
{
	FileCore::LogSystem::StaticInstance().Shutdown();
}

SettingNode::SettingNode(
	)
{
}

SettingNode::SettingNode(
	System::String^ data
	)
{
	m_Data = data;
}

SettingNode::SettingNode(
	System::String^ data,
	System::String^ node
	)
{
	m_Data = data;
	m_Nodes = gcnew array<SettingNode^>(1){ gcnew SettingNode(node) };
}

SettingNode::SettingNode(
	System::String^ data,
	array<SettingNode^>^ nodes
	)
{
	m_Data = data;
	m_Nodes = nodes;
}

System::String^
SettingNode::ToString(
	)
{
	return m_Data != nullptr ? m_Data : nullptr;
}

System::String^
SettingNode::ToString(
	System::String^ defaultValue
	)
{
	return m_Data != nullptr ? m_Data : defaultValue;
}

System::Int32
SettingNode::ToInt32(
	)
{
	return ToInt32(0);
}

System::Int32
SettingNode::ToInt32(
	System::Int32 defaultValue
	)
{
	return ToNative().GetInt32(defaultValue);
}

System::Boolean
SettingNode::ToBool(
	)
{
	return ToBool(false);
}

System::Boolean
SettingNode::ToBool(
	System::Boolean defaultValue
	)
{
	return ToNative().GetBool(defaultValue);
}

FileCore::SettingNode
SettingNode::ToNative(
	)
{
	FileCore::SettingNode node;
	node.SetString(marshal_as_wstring(m_Data));
	if (m_Nodes != nullptr)
	{
		node.Nodes().resize(m_Nodes->Length);
		for (int32_t i = 0; i < m_Nodes->Length; ++i)
		{
			node.Nodes()[i] = m_Nodes[i] != nullptr ? m_Nodes[i]->ToNative() : FileCore::SettingNode();
		}
	}
	return node;
}
	
SettingNode^
SettingNode::FromString(
	System::String^ value
	)
{
	FileCore::SettingNode node;
	node.SetString(marshal_as_wstring(value));
	return FromNative(node);
}

SettingNode^
SettingNode::FromInt32(
	System::Int32 value
	)
{
	FileCore::SettingNode node;
	node.SetInt32(value);
	return FromNative(node);
}

SettingNode^
SettingNode::FromBool(
	bool value
	)
{
	FileCore::SettingNode node;
	node.SetBool(value);
	return FromNative(node);
}

SettingNode^ 
SettingNode::FromNative(
	const FileCore::SettingNode& srcNode
	)
{
	SettingNode^ node = gcnew SettingNode();
	node->m_Data = gcnew System::String(srcNode.GetString().c_str());
	node->m_Nodes = gcnew array<SettingNode^>(int32_t(srcNode.Nodes().size()));
	for (uint32_t i = 0; i < srcNode.Nodes().size(); ++i)
	{
		node->m_Nodes[i] = FromNative(srcNode.Nodes()[i]);
	}
	return node;
}

void
SettingManager::Reset(
	)
{
	FileCore::SettingManager::StaticInstance().Reset();
}

SettingNode^ 
SettingManager::GetProperty(
	System::String^ name
	)
{
	FileCore::SettingNode node;
	if (FileCore::SettingManager::StaticInstance().GetProperty(marshal_as_wstring(name), node))
	{
		return SettingNode::FromNative(node);
	}
	return nullptr;
}

bool
SettingManager::SetProperty(
	System::String^ name,
	SettingNode^ value
	)
{
	const marshal_as_wstring cname(name);
	if (FileCore::SettingManager::StaticInstance().HasProperty(cname))
	{
		FileCore::SettingManager::StaticInstance().SetProperty(cname, value ? value->ToNative() : FileCore::SettingNode());
		return true;
	}
	return false;
}

FilePopulateInfo^
NativeMethods::GetFilePopulateInfo(
	System::String^		path
	)
{
	FileCore::GAllocPtr<P4VFS_REPARSE_DATA_2> populateData;
	HRESULT hr = FileOperations::GetFileReparseData(marshal_as_wchar(path), populateData);
	if (FAILED(hr) || populateData.get() == nullptr)
	{
		return nullptr;
	}
	
	FilePopulateInfo^ info	= gcnew FilePopulateInfo();
	info->ResidencyPolicy	= safe_cast<FileResidencyPolicy>(populateData->residencyPolicy);
	info->PopulatePolicy	= safe_cast<FilePopulatePolicy>(populateData->populatePolicy);	
	info->FileRevision		= populateData->fileRevision;	 		
	info->DepotPath			= gcnew System::String(populateData->depotPath.c_str());	
	info->DepotServer		= gcnew System::String(populateData->depotServer.c_str());	
	info->DepotClient		= gcnew System::String(populateData->depotClient.c_str());	
	info->DepotUser			= gcnew System::String(populateData->depotUser.c_str());	
	return info;
}

System::Int32 
NativeMethods::InstallReparsePointOnFile(
		System::UInt16		majorVersion,
		System::UInt16		minorVersion,
		System::UInt16		buildVersion,
		System::String^		filePath,
		System::Byte		residencyPolicy,
		System::UInt32		fileRevision,
		System::Int64		fileSize,
		System::UInt32		fileAttributes,
		System::String^		depotPath,
		System::String^		depotServer,
		System::String^		depotClient,
		System::String^		depotUser
		)
{
	return FileOperations::InstallReparsePointOnFile(
								majorVersion, 
								minorVersion, 
								buildVersion, 
								marshal_as_wchar(filePath), 
								residencyPolicy, 
								fileRevision,
								fileSize,
								fileAttributes,
								marshal_as_wchar(depotPath),
								marshal_as_wchar(depotServer),
								marshal_as_wchar(depotClient),
								marshal_as_wchar(depotUser)
								);
}

System::Int32
NativeMethods::CreateSymlinkFile(
	System::String^		symlinkFilePath,
	System::String^		targetFilePath
)
{
	return FileOperations::CreateSymlinkFile(
		marshal_as_wchar(symlinkFilePath),
		marshal_as_wchar(targetFilePath)
	);
}

System::Int32 
NativeMethods::RemoveReparsePoint(
	System::String^		path
	)
{
	return FileOperations::RemoveReparsePoint(marshal_as_wchar(path));
}

System::Int32 
NativeMethods::PopulateFile(
	System::String^		dstFile,
	System::String^		srcFile,
	FilePopulateMethod	populateMethod
	)
{
	return FileOperations::PopulateFile(marshal_as_wchar(dstFile), marshal_as_wchar(srcFile), safe_cast<BYTE>(populateMethod));
}

System::Boolean
NativeMethods::ImpersonateFileAppend(
	System::String^		fileName,
	System::String^		text,
	UserContext^		context
	)
{
	return SUCCEEDED(FileOperations::ImpersonateFileAppend(
			marshal_as_wchar(fileName), 
			marshal_as_wchar(text),
			marshal_as_user_context(context)
			));
}

System::Boolean
NativeMethods::ImpersonateLoggedOnUser(
	UserContext^		context
	)
{
	return SUCCEEDED(FileOperations::ImpersonateLoggedOnUser(marshal_as_user_context(context)));
}

System::Boolean
NativeMethods::RevertToSelf(
	)
{
	return ::RevertToSelf();
}

System::String^
NativeMethods::GetImpersonatedUserName(
	UserContext^		context
	)
{
	FileCore::String userName = FileOperations::GetImpersonatedUserName(marshal_as_user_context(context));
	if (userName.empty() == false)
		return gcnew System::String(userName.c_str());
	return nullptr;
}

System::String^
NativeMethods::GetImpersonatedEnvironmentStrings(
	System::String^		srcText,
	UserContext^		context
	)
{
	wchar_t dstText[2048] = {0};
	DWORD dstTextSize = _countof(dstText);
	HRESULT hr = FileOperations::GetImpersonatedEnvironmentStrings(marshal_as_wchar(srcText), dstText, dstTextSize, marshal_as_user_context(context));
	if (SUCCEEDED(hr))
		return gcnew System::String(dstText);
	return nullptr;
}

System::IntPtr^
NativeMethods::GetLoggedOnUserToken(
	UserContext^	context
	)
{
	HANDLE hToken = FileOperations::GetLoggedOnUserToken(marshal_as_user_context(context));
	if (hToken == INVALID_HANDLE_VALUE)
		return System::IntPtr::Zero;
	return gcnew System::IntPtr(hToken);
}

System::IntPtr^
NativeMethods::GetPreferredLoggedOnUserToken(
	)
{
	HANDLE hToken = FileOperations::GetPreferredLoggedOnUserToken();
	if (hToken == INVALID_HANDLE_VALUE)
		return System::IntPtr::Zero;
	return gcnew System::IntPtr(hToken);
}

System::Boolean
NativeMethods::CreateProcessImpersonated(
	System::String^					commandLine,
	System::String^					currentDirectory,
	System::Boolean					waitForExit,
	System::Text::StringBuilder^	stdOutput,
	UserContext^					context
	)
{
	FileCore::String stdOutputResult;
	HRESULT status = FileOperations::CreateProcessImpersonated(
										marshal_as_wchar(commandLine), 
										marshal_as_wchar(currentDirectory), 
										waitForExit,
										stdOutput != nullptr ? &stdOutputResult : nullptr,
										marshal_as_user_context(context)
										);
	if (stdOutput != nullptr)
		stdOutput->Append(gcnew System::String(stdOutputResult.c_str()));
	return SUCCEEDED(status);
}

System::Boolean
NativeMethods::SetDriverTraceEnabled(
	System::UInt32		channels
	)
{
	return SUCCEEDED(FileOperations::SetDriverTraceEnabled(channels));
}

System::Boolean
NativeMethods::SetDriverFlag(
	System::String^		flagName,
	System::UInt32		flagValue
	)
{
	return SUCCEEDED(FileOperations::SetDriverFlag(marshal_as_wchar(flagName), flagValue));
}

System::Boolean
NativeMethods::GetDriverIsConnected(
	System::Boolean%	connected
	)
{
	bool driverConnected = false;
	if (SUCCEEDED(FileOperations::GetDriverIsConnected(driverConnected)) == false)
		return false;
	connected = driverConnected;
	return true;
}

System::Boolean
NativeMethods::GetDriverVersion(
	System::UInt16%		major,
	System::UInt16%		minor,
	System::UInt16%		build,
	System::UInt16%		revision
	)
{
	USHORT driverMajor(0), driverMinor(0), driverBuild(0), driverRevision(0);
	if (SUCCEEDED(FileOperations::GetDriverVersion(driverMajor, driverMinor, driverBuild, driverRevision)) == false)
		return false;
	major = driverMajor;
	minor = driverMinor;
	build = driverBuild;
	revision = driverRevision;
	return true;
}

System::Boolean
NativeMethods::SetupInstallHinfSection(
	System::String^		sectionName,
	System::String^		filePath
	)
{
	return SUCCEEDED(FileSystem::SetupInstallHinfSection(marshal_as_wchar(sectionName), marshal_as_wchar(filePath)));
}

System::Boolean
NativeMethods::IsFileExists(
	System::String^		filePath
	)
{
	return FileCore::FileInfo::Exists(marshal_as_wchar(filePath));
}

System::Boolean
NativeMethods::IsFileRegular(
	System::String^		filePath
	)
{
	return FileCore::FileInfo::IsRegular(marshal_as_wchar(filePath));
}

System::Boolean
NativeMethods::IsFileSymlink(
	System::String^		filePath
	)
{
	return FileCore::FileInfo::IsSymlink(marshal_as_wchar(filePath));
}

System::Boolean
NativeMethods::IsFileDirectory(
	System::String^		filePath
	)
{
	return FileCore::FileInfo::IsDirectory(marshal_as_wchar(filePath));
}

System::Boolean
NativeMethods::IsFileReadOnly(
	System::String^		filePath
	)
{
	return FileCore::FileInfo::IsReadOnly(marshal_as_wchar(filePath));
}

System::Boolean
NativeMethods::SetFileReadOnly(
	System::String^		filePath,
	System::Boolean		readOnly
	)
{
	return FileCore::FileInfo::SetReadOnly(marshal_as_wchar(filePath), readOnly);
}

System::IO::FileAttributes
NativeMethods::GetFileAttributes(
	System::String^		filePath
	)
{
	DWORD attributes = FileCore::FileInfo::FileAttributes(marshal_as_wchar(filePath));
	if (attributes == INVALID_FILE_ATTRIBUTES)
		throw gcnew System::Exception("failed to get attributes");
	return static_cast<System::IO::FileAttributes>(attributes);
}

System::Int64
NativeMethods::GetFileSize(
	System::String^		filePath
	)
{
	return FileCore::FileInfo::FileSize(marshal_as_wchar(filePath));
}

System::Int64
NativeMethods::GetFileUncompressedSize(
	System::String^		filePath
	)
{
	return FileCore::FileInfo::FileUncompressedSize(marshal_as_wchar(filePath));
}

System::Int64
NativeMethods::GetFileDiskSize(
	System::String^		filePath
	)
{
	return FileCore::FileInfo::FileDiskSize(marshal_as_wchar(filePath));
}

System::String^
NativeMethods::GetFileSymlinkTarget(
	System::String^		filePath
	)
{
	return gcnew System::String(FileCore::FileInfo::SymlinkTarget(marshal_as_wchar(filePath)).c_str());
}

System::Boolean
NativeMethods::IsFileExtendedPath(
	System::String^		filePath
	)
{
	return FileCore::FileInfo::IsExtendedPath(marshal_as_wchar(filePath));
}

System::String^
NativeMethods::GetFileExtendedPath(
	System::String^		filePath
	)
{
	return gcnew System::String(FileCore::FileInfo::ExtendedPath(marshal_as_wchar(filePath)).c_str());
}

System::String^
NativeMethods::GetFileUnextendedPath(
	System::String^		filePath
	)
{
	return gcnew System::String(FileCore::FileInfo::UnextendedPath(marshal_as_wchar(filePath)).c_str());
}

UserContext^
NativeMethods::GetCurrentProcessUserContext(
	)
{
	UserContext^ context = gcnew UserContext();
	context->ProcessId = GetCurrentProcessId();
	return context;
}

}}}

