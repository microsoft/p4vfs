// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "FileCore.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace FileCore {

	struct UserContext;

	struct LogChannel 
	{ 
		enum Enum
		{
			Verbose,
			Debug,
			Info,
			Warning,
			Error
		};

		static AString ToString(Enum value);
		static LogChannel::Enum FromString(const AString& value);
	};

	struct LogElement
	{
		LogElement() : 
			m_Channel(LogChannel::Info),
			m_Text(),
			m_Time(0)
		{}

		LogElement(LogChannel::Enum channel, const String& text, time_t time = 0) : 
			m_Channel(channel),
			m_Text(text),
			m_Time(time)
		{}

		LogChannel::Enum m_Channel;
		String m_Text;
		time_t m_Time;
	};

	struct LogDevice
	{
		P4VFS_CORE_API LogDevice();
		P4VFS_CORE_API virtual ~LogDevice();
		P4VFS_CORE_API virtual void Write(const LogElement& element) = 0;
		P4VFS_CORE_API virtual bool IsFaulted();

		void Write(LogChannel::Enum channel, const WString& text)
		{
			Write(LogElement(channel, text, FileCore::TimeInfo::GetTime()));
		}

		void Write(LogChannel::Enum channel, const AString& text)
		{
			Write(channel, StringInfo::ToWide(text));
		}

		void WriteLine(LogChannel::Enum channel, const WString& text)
		{
			Write(channel, text + L"\n");
		}

		void WriteLine(LogChannel::Enum channel, const AString& text)
		{
			Write(channel, StringInfo::ToWide(text + "\n"));
		}

		template <typename TString>
		void Verbose(const TString& text) 
		{ 
			WriteLine(LogChannel::Verbose, text); 
		}

		template <typename TString>
		void Info(const TString& text) 
		{ 
			WriteLine(LogChannel::Info, text); 
		}

		template <typename TString>
		void Warning(const TString& text) 
		{ 
			WriteLine(LogChannel::Warning, text); 
		}
		
		template <typename TString>
		void Error(const TString& text) 
		{ 
			WriteLine(LogChannel::Error, text); 
		}

		template <typename TString>
		static void WriteLine(LogDevice* log, LogChannel::Enum channel, const TString& text)
		{
			if (log != nullptr)
				log->WriteLine(channel, text);
		}
	};

	struct LogDeviceConsole : LogDevice
	{
		virtual void Write(const LogElement& element) override;
	
	private:
		void Write(HANDLE hConsole, WORD textAttribute, const String& text);
	};

	struct LogDeviceDebugger : LogDevice
	{
		virtual void Write(const LogElement& element) override;
	};

	struct LogDeviceNull : LogDevice
	{
		virtual void Write(const LogElement& element) override;
	};

	struct LogDeviceFile : LogDevice
	{
		LogDeviceFile(const UserContext* impersonate = nullptr);
		virtual void Write(const LogElement& element) override;

	private:
		void WriteInternal(time_t time, LogChannel::Enum channel, const String& text);
		bool FileAppend(const String& filePath, const String& text) const;
		String GetDesiredUserName() const;
		String GetLogHeaderText() const;
		String ExpandVariables(const String& text) const;

	private:
		static constexpr const wchar_t* VariableUserName = L"$(USERNAME)";
		std::unique_ptr<UserContext> m_Impersonate;
		String m_RemoteFilePath;
		String m_LocalFilePath;
		bool m_HeaderWritten;
	};

	struct LogDeviceMemory : LogDevice
	{
		virtual void Write(const LogElement& element) override;

		List<LogElement> m_Elements;
	};

	struct LogDeviceAggregate : LogDevice
	{
		virtual void Write(const LogElement& element) override;
		virtual bool IsFaulted() override;

		Array<LogDevice*> m_Devices;
	};

	class LogSystem : public LogDevice
	{
	public:
		LogSystem();
		virtual ~LogSystem();

		virtual void Write(const LogElement& element) override;
		virtual bool IsFaulted() override;

		P4VFS_CORE_API static LogSystem& StaticInstance();
		P4VFS_CORE_API void Initialize(const UserContext* impersonate = nullptr);
		P4VFS_CORE_API void Shutdown(DWORD timeoutMs = INFINITE);

		P4VFS_CORE_API void Flush(DWORD timeoutMs = INFINITE);
		P4VFS_CORE_API void Suspend();
		P4VFS_CORE_API void Resume();
		P4VFS_CORE_API bool IsPending() const;
		P4VFS_CORE_API bool IsCancelRequested() const;
		P4VFS_CORE_API FILETIME GetLastWriteTime() const;

	private:
		void RestartWriteThread();
		bool PopLogElement(LogElement* element);
		void PushLogElement(const LogElement& element);
		void WriteLogElement(const LogElement& element);
		static bool IsChannelEnabled(LogChannel::Enum channel);

		static DWORD WriteThreadEntry(void* data);

	private:
		Array<LogDevice*> m_Devices;
		List<LogElement> m_Elements;
		std::unique_ptr<UserContext> m_Impersonate;
		HANDLE m_WriteThread;
		HANDLE m_WriteMutex;
		HANDLE m_WriteNotifySemaphore;
		HANDLE m_CancelationEvent;
		FILETIME m_LastWriteTime;
	};

}}}
#pragma managed(pop)
