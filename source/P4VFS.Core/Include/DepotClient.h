// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "FileCore.h"
#include "FileContext.h"
#include "DepotResult.h"
#include "DepotResultClient.h"
#include "DepotResultInfo.h"
#include "DepotCommand.h"
#include "DepotConfig.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	typedef std::shared_ptr<struct FDepotClient> DepotClient;
	typedef std::function<void(LogChannel::Enum channel, const char* severity, const char* text)> FDepotClientLogCallback;
	typedef std::shared_ptr<FDepotClientLogCallback> DepotClientLogCallback;

	struct FDepotClient
	{
		P4VFS_CORE_API FDepotClient(FileContext* context = nullptr);
		P4VFS_CORE_API virtual ~FDepotClient();

		template <typename... ArgTypes>
		P4VFS_CORE_API static DepotClient New(ArgTypes&&... args);
	
		P4VFS_CORE_API bool Connect(const char* user = nullptr, const char* client = nullptr, const char* port = nullptr, const char* passwd = nullptr);
		P4VFS_CORE_API bool Connect(const DepotConfig& config);
		P4VFS_CORE_API void Disconnect();
		P4VFS_CORE_API bool IsConnected();
		P4VFS_CORE_API bool IsConnectedClient();

		P4VFS_CORE_API bool IsLoginRequired();
		P4VFS_CORE_API bool Login(DepotClientPromptCallback prompt);
		P4VFS_CORE_API bool Login();

		P4VFS_CORE_API DepotResult Trust();
		P4VFS_CORE_API DepotResultClient Connection();
		P4VFS_CORE_API DepotResultClient Client();
		P4VFS_CORE_API DepotResultInfo Info();

		P4VFS_CORE_API void Reset();
		P4VFS_CORE_API bool HasError();
		P4VFS_CORE_API DepotString GetErrorText();
		P4VFS_CORE_API int32_t GetServerProtocol();

		P4VFS_CORE_API DepotString GetProgramName() const;
		P4VFS_CORE_API DepotString GetTicketsFilePath() const;
		P4VFS_CORE_API DepotString GetTrustFilePath() const;
		P4VFS_CORE_API DepotString GetClientOwnerUserName(const DepotString& clientName, const DepotString& portName);
		P4VFS_CORE_API DepotString GetHostName() const;
		P4VFS_CORE_API const DepotConfig& Config() const;

		P4VFS_CORE_API void Run(const DepotCommand& cmd, FDepotResult& result);

		template <typename Result = DepotResult>
		Result Run(const DepotCommand& cmd);

		template <typename Result = DepotResult>
		Result Run(const DepotString& name, const DepotStringArray& args = DepotStringArray());
	
		virtual void OnErrorPause(const char* message);
		virtual void OnErrorCallback(LogChannel::Enum channel, const char* severity, const char* text);
		virtual void OnMessageCallback(LogChannel::Enum channel, const char* severity, const char* text);
		virtual DepotString OnPromptCallback(const DepotCommand* command, const char* message);

		P4VFS_CORE_API LogDevice* Log();
		P4VFS_CORE_API void Log(LogChannel::Enum channel, const DepotString& text);
		P4VFS_CORE_API bool IsFaulted() const;

		struct Flags { enum Enum
		{
			None			= 0,
			DisableLogin	= 1<<0,
			Unattended		= 1<<1,
			Unimpersonated	= 1<<2,
		};};

		P4VFS_CORE_API Flags::Enum GetFlags() const;
		P4VFS_CORE_API void SetFlags(Flags::Enum flags);
		P4VFS_CORE_API bool HasFlag(Flags::Enum flag) const;
		P4VFS_CORE_API void ApplyFlag(Flags::Enum flag, bool set);

		P4VFS_CORE_API void SetContext(FileContext* fileContext);
		P4VFS_CORE_API FileContext* GetContext() const;
		P4VFS_CORE_API UserContext* GetUserContext() const;
		P4VFS_CORE_API time_t GetAccessTimeSpan() const;

		P4VFS_CORE_API bool SetEnv(const char* name, const char* value);
		P4VFS_CORE_API DepotString GetEnv(const char* name) const;
		P4VFS_CORE_API DepotString GetEnvImpersonated(const char* name) const;
		P4VFS_CORE_API WString GetEnvImpersonatedW(const char* name) const;

		P4VFS_CORE_API void SetErrorCallback(DepotClientLogCallback callback);
		P4VFS_CORE_API void SetMessageCallback(DepotClientLogCallback callback);

	protected:
		bool LoginUsingConfig();
		bool LoginUsingClientOwner();
		bool LoginUsingInteractiveSession();
		bool RequestInteractivePassword(DepotString& passwd);

	private:
		struct Api;
		Api* m_P4;
	};


	template <typename Result>
	Result FDepotClient::Run(const DepotCommand& cmd)
	{
		Result result = std::make_shared<Result::element_type>();
		Run(cmd, *result.get());
		return result;
	}

	template <typename Result>
	Result FDepotClient::Run(const DepotString& name, const DepotStringArray& args)
	{
		return Run<Result>(DepotCommand(name, args));
	}

	DEFINE_ENUM_FLAG_OPERATORS(FDepotClient::Flags::Enum);

	struct DepotInfo
	{
		P4VFS_CORE_API static time_t StringToTime(const DepotString& text);
		P4VFS_CORE_API static DepotString TimeToString(const time_t& time);

		P4VFS_CORE_API static bool IsWritableFileType(const DepotString& fileType);
		P4VFS_CORE_API static bool IsSymlinkFileType(const DepotString& fileType);

		P4VFS_CORE_API static DepotConfig DepotConfigFromPath(const DepotString& folderPath);
	};

	struct DepotTunable
	{
		P4VFS_CORE_API static int32_t Get(const DepotString& name, int32_t defaultValue = -1);
		P4VFS_CORE_API static void	Set(const DepotString& name, int32_t value);
		P4VFS_CORE_API static void	Unset(const DepotString& name);
		P4VFS_CORE_API static bool IsSet(const DepotString& name);
		P4VFS_CORE_API static bool IsKnown(const DepotString& name);
	};

}}}

#pragma managed(pop)
