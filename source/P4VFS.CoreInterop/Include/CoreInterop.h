// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "FileInterop.h"
#include "DriverVersion.h"
#include "DriverTrace.h"
#include "LogDevice.h"
#ifdef GetFileAttributes
#undef GetFileAttributes
#endif

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

public ref class UserContext
{
public:
	System::Int32 SessionId;
	System::Int32 ProcessId;
	System::Int32 ThreadId;

	UserContext() :
		SessionId(0),
		ProcessId(0),
		ThreadId(0)
	{}

	FileCore::UserContext
	ToNative(
		);
	
	static UserContext^
	FromNative(
		const FileCore::UserContext& src
		);

	static UserContext^
	CurrentProcess(
		);
};

public enum class LogChannel : System::Int32
{
	Verbose		= FileCore::LogChannel::Verbose,
	Debug		= FileCore::LogChannel::Debug,
	Info		= FileCore::LogChannel::Info,
	Warning		= FileCore::LogChannel::Warning,
	Error		= FileCore::LogChannel::Error,
};

public ref class LogElement
{
public:
	LogChannel m_Channel;
	System::String^ m_Text;
	System::Int64 m_Time;

	FileCore::LogElement
	ToNative(
		);
	
	static LogElement^
	FromNative(
		const FileCore::LogElement& src
		);

	static System::Int64
	Now(
		);
};

public ref class LogDevice
{
public:

	virtual void 
	Write(
		LogElement^ element
		);

	virtual bool 
	IsFaulted(
		);
};

public ref class FileContext
{
public:
	LogDevice^ LogDevice;
	UserContext^ UserContext;

	FileContext() :
		LogDevice(nullptr),
		UserContext(nullptr)
	{}
};

public ref class LogSystem abstract sealed
{
public:

	static void
	Write(
		LogElement^ element
		);

	static bool
	IsPending(
		);

	static void
	Flush(
		);

	static void
	Suspend(
		);

	static void
	Resume(
		);

	static void
	Initialize(
		UserContext^ context
		);

	static void
	Shutdown(
		);
};

public ref class NativeMethods abstract sealed
{
public:

	static FilePopulateInfo^
	GetFilePopulateInfo(
		System::String^		path
		);

	static System::Int32 
	InstallReparsePointOnFile(
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
		);

	static System::Int32
	CreateSymlinkFile(
		System::String^		symlinkFilePath,
		System::String^		targetFilePath
	);

	static System::Int32 
	RemoveReparsePoint(
		System::String^		filePath
		);

	static System::Int32 
	PopulateFile(
		System::String^		dstFile,
		System::String^		srcFile,
		FilePopulateMethod	populateMethod
		);

	static System::Int32 
	HydrateFile(
		System::String^		filePath
		);

	static System::Boolean
	ImpersonateFileAppend(
		System::String^		filePath,
		System::String^		text,
		UserContext^		context
		);

	static System::Boolean
	ImpersonateLoggedOnUser(
		UserContext^	context
		);

	static System::Boolean
	RevertToSelf(
		);

	static System::String^
	GetImpersonatedUserName(
		UserContext^	context
		);

	static System::String^
	GetImpersonatedEnvironmentStrings(
		System::String^	srcText,
		UserContext^	context
		);

	static System::IntPtr^
	GetLoggedOnUserToken(
		UserContext^	context
		);

	static System::IntPtr^
	GetPreferredLoggedOnUserToken(
		);

	static System::Boolean
	CreateProcessImpersonated(
		System::String^					commandLine,
		System::String^					currentDirectory,
		System::Boolean					waitForExit,
		System::Text::StringBuilder^	stdOutput,
		UserContext^					context
		);

	static System::Boolean
	SetDriverTraceEnabled(
		System::UInt32		channels
		);

	static System::Boolean
	SetDriverFlag(
		System::String^		flagName,
		System::UInt32		flagValue
		);

	static System::Boolean
	GetDriverIsConnected(
		System::Boolean%	connected
		);

	static System::Boolean
	GetDriverVersion(
		System::UInt16%		major,
		System::UInt16%		minor,
		System::UInt16%		build,
		System::UInt16%		revision
		);

	static System::Boolean
	SetupInstallHinfSection(
		System::String^		sectionName,
		System::String^		filePath
		);

	static System::Boolean
	IsFileExists(
		System::String^		filePath
		);

	static System::Boolean
	IsFileRegular(
		System::String^		filePath
		);

	static System::Boolean
	IsFileSymlink(
		System::String^		filePath
		);

	static System::Boolean
	IsFileDirectory(
		System::String^		filePath
		);

	static System::Boolean
	IsFileReadOnly(
		System::String^		filePath
		);

	static System::Boolean
	SetFileReadOnly(
		System::String^		filePath,
		System::Boolean		readOnly
		);

	static System::IO::FileAttributes
	GetFileAttributes(
		System::String^		filePath
		);

	static System::Int64
	GetFileSize(
		System::String^		filePath
		);

	static System::Int64
	GetFileUncompressedSize(
		System::String^		filePath
		);

	static System::Int64
	GetFileDiskSize(
		System::String^		filePath
		);

	static System::String^
	GetFileSymlinkTarget(
		System::String^		filePath
		);

	static System::Boolean
	IsFileExtendedPath(
		System::String^		filePath
		);

	static System::String^
	GetFileExtendedPath(
		System::String^		filePath
		);
	
	static System::String^
	GetFileUnextendedPath(
		System::String^		filePath
		);

	static UserContext^
	GetCurrentProcessUserContext(
		);
};

public ref class NativeConstants abstract sealed
{
public:

	literal System::String^ Version			= P4VFS_VER_VERSION_STRING;
	literal System::Int32 VersionMajor		= P4VFS_VER_MAJOR;
	literal System::Int32 VersionMinor		= P4VFS_VER_MINOR;
	literal System::Int32 VersionBuild		= P4VFS_VER_BUILD;
	literal System::Int32 VersionRevision	= P4VFS_VER_REVISION;

	literal System::String^ WppGuid			= P4VFS_VER_STRINGIZE(P4VFS_WPP_CONTROL_GUID);

	literal System::String^ DriverTitle		= P4VFS_DRIVER_TITLE;
	literal System::String^ ServiceTitle	= P4VFS_SERVICE_TITLE;
	literal System::String^ MonitorTitle	= P4VFS_MONITOR_TITLE;
};

}}}

