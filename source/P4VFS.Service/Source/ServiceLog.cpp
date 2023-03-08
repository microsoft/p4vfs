// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "ServiceLog.h"

namespace Microsoft {
namespace P4VFS {

void 
ServiceLog::WriteLine(
	FileCore::LogChannel::Enum level, 
	const WCHAR* text
	)
{
	FileCore::LogSystem::StaticInstance().WriteLine(level, text);
}

void 
ServiceLog::Verbose(
	const WCHAR* text
	)
{
	WriteLine(FileCore::LogChannel::Verbose, text);
}

void 
ServiceLog::Info(
	const WCHAR* text
	)
{
	WriteLine(FileCore::LogChannel::Info, text);
}

void 
ServiceLog::Warning(
	const WCHAR* text
	)
{
	WriteLine(FileCore::LogChannel::Warning, text);
}

void 
ServiceLog::Error(
	const WCHAR* text
	)
{
	WriteLine(FileCore::LogChannel::Error, text);
}

ServiceLogDevice::ServiceLogDevice(
	HANDLE hCancelationEvent
	)
{
	m_CancelationEvent = hCancelationEvent;
}

ServiceLogDevice::~ServiceLogDevice(
	)
{
}

void 
ServiceLogDevice::Write(
	const FileCore::LogElement& element
	)
{
	FileCore::LogSystem::StaticInstance().Write(element);
}
	
bool 
ServiceLogDevice::IsFaulted(
	)
{
	if (m_CancelationEvent != NULL)
	{
		return WaitForSingleObject(m_CancelationEvent, 0) == WAIT_OBJECT_0;
	}
	return false;
}

}}
