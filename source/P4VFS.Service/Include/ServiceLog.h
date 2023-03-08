// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "LogDevice.h"

namespace Microsoft {
namespace P4VFS {

struct ServiceLog
{
	static void 
	WriteLine(
		FileCore::LogChannel::Enum level, 
		const WCHAR* text
		);

	static void 
	ServiceLog::Verbose(
		const WCHAR* text
		);

	static void 
	Info(
		const WCHAR* text
		);

	static void 
	Warning(
		const WCHAR* text
		);

	static void 
	Error(
		const WCHAR* text
		);
};

class ServiceLogDevice : public FileCore::LogDevice
{
public:
	ServiceLogDevice(
		HANDLE hCancelationEvent
		);

	virtual 
	~ServiceLogDevice(
		);

	virtual void 
	Write(
		const FileCore::LogElement& element
		) override;
	
	virtual bool 
	IsFaulted(
		) override;

private:
	HANDLE m_CancelationEvent;
};

}}
