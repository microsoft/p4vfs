// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "LogDevice.h"
#include "FileContext.h"
#include "FileOperations.h"
#include "SettingManager.h"
#include "DepotDateTime.h"

namespace Microsoft {
namespace P4VFS {
namespace FileCore {

AString LogChannel::ToString(Enum value)
{
	P4VFS_ENUM_TO_STRING_RETURN(value, LogChannel, Verbose);
	P4VFS_ENUM_TO_STRING_RETURN(value, LogChannel, Debug);
	P4VFS_ENUM_TO_STRING_RETURN(value, LogChannel, Info);
	P4VFS_ENUM_TO_STRING_RETURN(value, LogChannel, Warning);
	P4VFS_ENUM_TO_STRING_RETURN(value, LogChannel, Error);
	return AString();
}

LogChannel::Enum LogChannel::FromString(const AString& value)
{
	P4VFS_STRING_TO_ENUM_RETURN(value, LogChannel, Verbose);
	P4VFS_STRING_TO_ENUM_RETURN(value, LogChannel, Debug);
	P4VFS_STRING_TO_ENUM_RETURN(value, LogChannel, Info);
	P4VFS_STRING_TO_ENUM_RETURN(value, LogChannel, Warning);
	P4VFS_STRING_TO_ENUM_RETURN(value, LogChannel, Error);
	return LogChannel::Info;
}

LogDevice::LogDevice()
{
}

LogDevice::~LogDevice()
{
}

bool LogDevice::IsFaulted()
{
	return false;
}

void LogDeviceConsole::Write(const LogElement& element)
{
	if (element.m_Channel == LogChannel::Error)
	{
		Write(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED|FOREGROUND_INTENSITY, element.m_Text);
	}
	else if (element.m_Channel == LogChannel::Warning)
	{
		Write(GetStdHandle(STD_ERROR_HANDLE), FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY, element.m_Text);
	}
	else
	{
		Write(GetStdHandle(STD_OUTPUT_HANDLE), 0, element.m_Text);
	}
}

void LogDeviceConsole::Write(HANDLE hConsole, WORD textAttribute, const String& text)
{
	WORD prevTextAttribute = textAttribute;
	if (textAttribute != 0)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (GetConsoleScreenBufferInfo(hConsole, &csbi))
		{
			prevTextAttribute = csbi.wAttributes;
			SetConsoleTextAttribute(hConsole, textAttribute);
		}
	}

	DWORD dwWritten = 0;
	AString atext = StringInfo::ToAnsi(text);
	WriteFile(hConsole, atext.c_str(), DWORD(atext.length()), &dwWritten, NULL);
	if (prevTextAttribute != textAttribute)
	{
		SetConsoleTextAttribute(hConsole, prevTextAttribute);
	}
}

void LogDeviceDebugger::Write(const LogElement& element)
{
	if (IsDebuggerPresent())
	{
		OutputDebugStringW(element.m_Text.c_str());
	}
}

void LogDeviceNull::Write(const LogElement& element)
{
}

LogDeviceFile::LogDeviceFile(const UserContext* impersonate)
{
	m_Impersonate = impersonate ? std::make_unique<UserContext>(*impersonate) : std::make_unique<UserContext>();
	m_HeaderWritten = false;

	const time_t now = FileCore::TimeInfo::GetTime();
	String relativeFileName = StringInfo::Format(L"%s\\%s\\%s_%s.log", 
		StringInfo::WCStr(FileInfo::FileTitle(FileInfo::ApplicationFilePath().c_str())), 
		StringInfo::FormatLocalTime(now, L"%Y_%m_%d").c_str(),
		StringInfo::WCStr(VariableUserName),
		StringInfo::FormatLocalTime(now, L"%Y_%m_%d_%H_%M_%S").c_str());

	String logLocalDirectory = SettingManager::StaticInstance().FileLoggerLocalDirectory.GetValue();
	if (logLocalDirectory.empty() == false)
	{
		String localDirectory = FileOperations::GetImpersonatedEnvironmentStrings(logLocalDirectory.c_str(), m_Impersonate.get());
		m_LocalFilePath = FileInfo::FullPath(StringInfo::Format(L"%s\\%s", localDirectory.c_str(), relativeFileName.c_str()).c_str());
	}

	String logRemoteDirectory = SettingManager::StaticInstance().FileLoggerRemoteDirectory.GetValue();
	if (logRemoteDirectory.empty() == false)
	{
		m_RemoteFilePath = FileInfo::FullPath(StringInfo::Format(L"%s\\%s\\%s", logRemoteDirectory.c_str(), StringInfo::WCStr(VariableUserName), relativeFileName.c_str()).c_str());
	}
}

void LogDeviceFile::Write(const LogElement& element)
{
	WriteInternal(element.m_Time, element.m_Channel, element.m_Text);
}

void LogDeviceFile::WriteInternal(time_t time, LogChannel::Enum channel, const String& text)
{
	String line = StringInfo::Format(L"-%s::<%s> - %s\n", 
		CSTR_ATOW(P4::DepotDateTime(time).ToDisplayString()), 
		CSTR_ATOW(LogChannel::ToString(channel)), 
		StringInfo::TrimRight(text.c_str(), L"\n").c_str());

	if (m_RemoteFilePath.empty() == false && SettingManager::StaticInstance().RemoteLogging.GetValue())
	{
		if (m_HeaderWritten == false)
		{
			String headerText = GetLogHeaderText();
			line = StringInfo::Format(L"%s\n%s\n", StringInfo::TrimRight(headerText.c_str(), L"\n").c_str(), StringInfo::TrimRight(line.c_str(), L"\n").c_str());
			m_HeaderWritten = true;
		}

		if (m_Impersonate.get())
		{
			if (FileOperations::ImpersonateFileAppend(ExpandVariables(m_RemoteFilePath).c_str(), line.c_str(), m_Impersonate.get()))
				return;
		}
		else
		{
			if (FileOperations::FileAppend(ExpandVariables(m_RemoteFilePath).c_str(), line.c_str()))
				return;
		}
	}

	if (m_LocalFilePath.empty() == false)
	{
		FileOperations::FileAppend(ExpandVariables(m_LocalFilePath).c_str(), line.c_str());
	}
}

String LogDeviceFile::GetDesiredUserName() const
{
	String alias = m_Impersonate.get() ? FileOperations::GetImpersonatedUserName(m_Impersonate.get()) : FileOperations::GetUserName();
	if (alias.empty())
	{
		alias = FileOperations::GetExpandedEnvironmentStrings(L"%COMPUTERNAME%");
	}
	return alias;
}

String LogDeviceFile::GetLogHeaderText() const
{
	String text;
	String separator(130, L'=');

	text += StringInfo::Format(L"%s\n", separator.c_str());
	text += StringInfo::Format(L"UserName: %s\n", GetDesiredUserName().c_str());
	text += StringInfo::Format(L"MachineName: %s\n", FileOperations::GetExpandedEnvironmentStrings(L"%COMPUTERNAME%").c_str());
	text += StringInfo::Format(L"UserDomainName: %s\n", FileOperations::GetExpandedEnvironmentStrings(L"%USERDOMAIN%").c_str());
			
	HKEY hVersionKey = NULL;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hVersionKey) == ERROR_SUCCESS)
	{
		text += StringInfo::Format(L"OS Name: %s\n", RegistryInfo::GetValue(hVersionKey, L"ProductName").ToString().c_str());
		text += StringInfo::Format(L"OS Architecture: %s\n", RegistryInfo::GetValue(hVersionKey, L"BuildLabEx").ToString().c_str());
		text += StringInfo::Format(L"OS Version: %s.%s.%s\n", RegistryInfo::GetValue(hVersionKey, L"CurrentMajorVersionNumber").ToString().c_str(), RegistryInfo::GetValue(hVersionKey, L"CurrentMinorVersionNumber").ToString().c_str(), RegistryInfo::GetValue(hVersionKey, L"CurrentBuildNumber").ToString().c_str());
		RegCloseKey(hVersionKey);
	}

	text += StringInfo::Format(L"%s\n", separator.c_str());
	return text;
}

String LogDeviceFile::ExpandVariables(const String& text) const
{
	return StringInfo::Replace(text.c_str(), VariableUserName, StringInfo::ToLower(GetDesiredUserName().c_str()).c_str(), StringInfo::SearchCase::Insensitive);
}

LogDeviceMemory::LogDeviceMemory() :
	m_ElementsMutex(CreateMutex(NULL, FALSE, NULL))
{
}

void LogDeviceMemory::Write(const LogElement& element)
{
	AutoMutex elementsScope(m_ElementsMutex);
	m_Elements.push_back(element);
}

const List<LogElement>& LogDeviceMemory::GetElements() const
{
	return m_Elements;
}

void LogDeviceAggregate::Write(const LogElement& element)
{
	for (LogDevice* device : m_Devices)
	{
		if (device != nullptr)
		{
			device->Write(element);
		}
	}
}

bool LogDeviceAggregate::IsFaulted()
{
	for (LogDevice* device : m_Devices)
	{
		if (device != nullptr && device->IsFaulted())
		{
			return true;
		}
	}
	return false;
}

void LogDeviceAggregate::AddDevice(LogDevice* device)
{
	m_Devices.push_back(device);
}

LogDeviceFilter::LogDeviceFilter(LogDevice* device, LogChannel::Enum level) :
	m_InnerDevice(device),
	m_Level(level)
{
}

void LogDeviceFilter::Write(const LogElement& element)
{
	if (m_InnerDevice != nullptr && element.m_Channel >= m_Level)
	{
		m_InnerDevice->Write(element);
	}
}

bool LogDeviceFilter::IsFaulted()
{
	return m_InnerDevice != nullptr ? m_InnerDevice->IsFaulted() : false;
}

LogSystem::LogSystem() :
	m_WriteThread(NULL),
	m_WriteMutex(NULL),
	m_WriteNotifySemaphore(NULL),
	m_CancelationEvent(NULL),
	m_LastWriteTime(FileCore::MinFileTime)
{
}

LogSystem::~LogSystem()
{
}

void LogSystem::Write(const LogElement& element)
{
	PushLogElement(element);
	if (SettingManager::StaticInstance().ImmediateLogging.GetValue())
	{
		Flush();
	}
}

bool LogSystem::IsFaulted()
{
	for (LogDevice* device : m_Devices)
	{
		if (device->IsFaulted())
		{
			return true;
		}
	}
	return false;
}

LogSystem& LogSystem::StaticInstance()
{
	static LogSystem instance;
	return instance;
}

void LogSystem::Initialize(const UserContext* impersonate)
{
	Shutdown();

	if (impersonate != nullptr)
	{
		m_Impersonate = std::make_unique<UserContext>(*impersonate);
	}
	else if (StringInfo::Stricmp(FileOperations::GetUserName().c_str(), TEXT("SYSTEM")) == 0)
	{
		m_Impersonate = std::make_unique<UserContext>();
	}

	m_WriteMutex = CreateMutex(NULL, FALSE, NULL);
	m_WriteNotifySemaphore = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
	m_CancelationEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_Devices.push_back(new LogDeviceConsole());
	m_Devices.push_back(new LogDeviceDebugger());
	m_Devices.push_back(new LogDeviceFile(impersonate));

	RestartWriteThread();
}

void LogSystem::Shutdown(DWORD timeoutMs)
{
	Flush(timeoutMs);

	if (m_CancelationEvent != NULL)
	{
		SetEvent(m_CancelationEvent);
	}
	if (m_WriteThread != NULL)
	{
		WaitForSingleObject(m_WriteThread, INFINITE);
	}

	Algo::ClearDelete(m_Devices);
	m_Impersonate.reset();
	m_Elements.clear();

	SafeCloseHandle(m_WriteThread);
	SafeCloseHandle(m_WriteMutex);
	SafeCloseHandle(m_WriteNotifySemaphore);
	SafeCloseHandle(m_CancelationEvent);
}

void LogSystem::Flush(DWORD timeoutMs)
{
	P4::DepotStopwatch waitTimer(P4::DepotStopwatch::Init::Start);
	while (IsPending() && (timeoutMs == INFINITE || waitTimer.TotalMilliseconds() < timeoutMs))
	{
		Sleep(0);
	}
}

void LogSystem::Suspend()
{
	WaitForSingleObject(m_WriteMutex, INFINITE);
}

void LogSystem::Resume()
{
	ReleaseMutex(m_WriteMutex);
}

bool LogSystem::IsPending() const
{
	bool pending = false;
	if (WaitForSingleObject(m_WriteMutex, INFINITE) == WAIT_OBJECT_0)
	{
		pending = m_Elements.empty() == false;
		ReleaseMutex(m_WriteMutex);
	}
	return pending;
}

bool LogSystem::IsCancelRequested() const
{
	return WaitForSingleObject(m_CancelationEvent, 0) == WAIT_OBJECT_0;
}

FILETIME LogSystem::GetLastWriteTime() const
{
	return m_LastWriteTime;
}

void LogSystem::RestartWriteThread()
{
	SetEvent(m_CancelationEvent);
	WaitForSingleObject(m_WriteThread, INFINITE);
	ResetEvent(m_CancelationEvent);

	SafeCloseHandle(m_WriteThread);
	m_WriteThread = CreateThread(NULL, 0, &WriteThreadEntry, this, 0, NULL);
}

bool LogSystem::PopLogElement(LogElement* element)
{
	bool success = false;
	if (WaitForSingleObject(m_WriteMutex, INFINITE) == WAIT_OBJECT_0)
	{
		if (m_Elements.empty() == false)
		{
			*element = m_Elements.front();
			m_Elements.pop_front();
			success = true;
		}
		ReleaseMutex(m_WriteMutex);
	}
	return success;
}

void LogSystem::PushLogElement(const LogElement& element)
{
	if (WaitForSingleObject(m_WriteMutex, INFINITE) == WAIT_OBJECT_0)
	{
		m_Elements.push_back(element);
		ReleaseMutex(m_WriteMutex);
		ReleaseSemaphore(m_WriteNotifySemaphore, 1, NULL);
	}
	if (IsChannelEnabled(element.m_Channel))
	{
		m_LastWriteTime = TimeInfo::GetUtcFileTime();
	}
}

void LogSystem::WriteLogElement(const LogElement& element)
{
	LogElement outElement = element;
	if (outElement.m_Text.empty() || outElement.m_Text.back() != L'\n')
	{
		outElement.m_Text += L"\n";
	}
	for (LogDevice* device : m_Devices)
	{
		device->Write(outElement);
	}
}

bool LogSystem::IsChannelEnabled(LogChannel::Enum channel)
{
	return channel >= LogChannel::FromString(StringInfo::ToAnsi(SettingManager::StaticInstance().Verbosity.GetValue()));
}

DWORD LogSystem::WriteThreadEntry(void* data)
{
	LogSystem* system = reinterpret_cast<LogSystem*>(data);
	while (system->IsCancelRequested() == false)
	{
		const HANDLE handles[] = { system->m_WriteNotifySemaphore, system->m_CancelationEvent };
		if (WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE) != WAIT_OBJECT_0)
			continue;

		while (system->IsCancelRequested() == false)
		{
			LogElement element;
			if (system->PopLogElement(&element) == false)
				break;

			if (IsChannelEnabled(element.m_Channel))
			{
				system->WriteLogElement(element);
			}
		}
	}
	return 0;
}

}}}
