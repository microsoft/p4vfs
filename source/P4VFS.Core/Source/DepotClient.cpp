// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotClient.h"
#include "DepotDateTime.h"
#include "DepotConstants.h"
#include "FileOperations.h"
#include "SettingManager.h"
#include "DriverVersion.h"
#pragma warning(push)
#pragma warning(disable: 4267)
#pragma warning(disable: 4244)
#undef Verify
#undef SetPort
#define OS_NT
#include "p4/clientapi.h"
#include "p4/datetime.h"
#include "p4/i18napi.h"
#include "p4/charcvt.h"
#include "p4/enviro.h"
#include "p4/strarray.h"
#include "p4/debug.h"
#pragma warning(pop)

namespace Microsoft {
namespace P4VFS {
namespace P4 {

struct FDepotTextEncoder
{
	virtual ~FDepotTextEncoder() {}
	virtual void Encode(const char** pdata, int32_t* plength) {}
};

struct FDepotTextEncoderCharSet : FDepotTextEncoder
{
	FDepotTextEncoderCharSet(CharSetApi::CharSet toCharSet)
	{
		m_Cvt = CharSetCvt::FindCvt(CharSetCvt::UTF_8, toCharSet);
	}

	virtual ~FDepotTextEncoderCharSet()
	{
		delete m_Cvt;
		m_Cvt = nullptr;
	}

	void Encode(const char** pdata, int32_t* plength) override
	{
		if (m_Cvt != nullptr && pdata != nullptr && *pdata != nullptr && plength != nullptr && *plength > 0)
		{
			int32_t newLength = 0;
			if (const char* newData = m_Cvt->FastCvt(*pdata, *plength, &newLength))
			{
				*pdata = newData;
				*plength = newLength;
			}
		}
	}

private:
	CharSetCvt* m_Cvt;
};

struct FDepotTextEncoderPlatformBase : FDepotTextEncoder
{
	struct Flags { enum Enum
	{
		None		= 0,
		CrLf		= 1<<0,
		Bom			= 1<<1,
		Utf8		= 1<<2,
		Utf16		= 1<<3,
		TextUtf8	= Utf8|Bom,
		TextUtf16	= Utf16|Bom,
	};};
};

DEFINE_ENUM_FLAG_OPERATORS(FDepotTextEncoderPlatformBase::Flags::Enum);

template <typename CharType>
struct FDepotTextEncoderPlatform : FDepotTextEncoderPlatformBase
{
	FDepotTextEncoderPlatform(Flags::Enum flags) :
		m_Flags(flags)
	{
	}

	void Encode(const char** pdata, int32_t* plength) override
	{
		if (pdata != nullptr && *pdata != nullptr && plength != nullptr && *plength > 0)
		{
			m_Buffer.clear();
			if (m_Flags & Flags::Bom)
			{
				m_Flags &= ~Flags::Bom;
				if (m_Flags & Flags::Utf8)
					Write({ 0xEF, 0xBB, 0xBF });
				else if (m_Flags & Flags::Utf16)
					Write({ 0xFF, 0xFE });
			}
			
			if (m_Buffer.size() == 0 && (m_Flags & Flags::CrLf) == 0)
				return;

			const size_t srcNum = size_t(*plength) / sizeof(CharType);
			const CharType* src = reinterpret_cast<const CharType*>(*pdata);
			m_Buffer.reserve(std::max(m_Buffer.size(), ((srcNum*4)/3)*sizeof(CharType)));

			for (size_t srcIndex = 0; srcIndex < srcNum; ++srcIndex)
			{
				const CharType c = src[srcIndex];
				if (c == LITERAL(CharType, '\n') && (m_Flags & Flags::CrLf) != 0)
					Write(LITERAL(CharType, '\r'));
				Write(c);
			}

			if (m_Buffer.size() > 0)
			{
				*pdata = reinterpret_cast<const char*>(m_Buffer.data());
				*plength = int32_t(m_Buffer.size());
			}
		}
	}

private:
	void Write(const std::initializer_list<uint8_t>& data)
	{
		FileCore::Algo::Append(m_Buffer, data);
	}

	void Write(char data)
	{
		m_Buffer.push_back(uint8_t(data));
	}

	void Write(wchar_t data)
	{
		m_Buffer.push_back(((uint8_t*)&data)[0]);
		m_Buffer.push_back(((uint8_t*)&data)[1]);
	}

private:
	int32_t m_Flags;
	Array<uint8_t> m_Buffer;
};

class DepotClientCommand : public ClientUser, IDepotClientCommand
{
public:
	DepotClientCommand(FDepotClient* client, const DepotCommand* cmd, FDepotResult* result) :
		m_TagLevel('0'),
		m_Client(client),
		m_Command(cmd),
		m_Result(result)
	{
	}

	virtual ~DepotClientCommand()
	{
	}

	Array<DepotResultTag>& TagList()
	{
		return m_Result->m_TagList;
	}

	Array<DepotResultText>& TextList()
	{
		return m_Result->m_TextList;
	}

	virtual void HandleError(Error* err) override
	{
		if (err && err->GetSeverity() != E_EMPTY)
		{
			StrBuf buf;
			err->Fmt(buf, EF_NEWLINE);
			TextList().push_back(std::make_shared<FDepotResultText>(DepotResultChannel::StdErr, buf.Text()));

			m_Client->OnErrorCallback(GetErrorLogChannel(err), err->FmtSeverity(), buf.Text());
		}
	}

	virtual void Message(Error* err) override
	{
		if (err && err->GetSeverity() != E_EMPTY)
		{
			StrBuf buf;
			err->Fmt(buf, EF_NEWLINE);
			if (err->GetSeverity() >= E_WARN)
			{
				TextList().push_back(std::make_shared<FDepotResultText>(DepotResultChannel::StdErr, buf.Text()));
			}
			else if (err->GetSeverity() >= E_INFO)
			{
				TextList().push_back(std::make_shared<FDepotResultText>(DepotResultChannel::StdOut, buf.Text(), err->GetGeneric()));
			}

			m_Client->OnMessageCallback(GetErrorLogChannel(err), err->FmtSeverity(), buf.Text());
		}
	}

	virtual void OutputError(const char* errBuf) override
	{
		TextList().push_back(std::make_shared<FDepotResultText>(DepotResultChannel::StdErr, errBuf ? errBuf : ""));
	}

	virtual void OutputText(const char* data, int length) override
	{
		for (std::shared_ptr<FDepotTextEncoder>& encoder : m_Encoders)
			encoder->Encode(&data, &length);
		
		if (m_Result->OnStreamOutput(this, data, length) == DepotResultReply::Handled)
			return;

		if (data && length < 0)
			TextList().push_back(std::make_shared<FDepotResultText>(DepotResultChannel::StdOut, data));
		else if (data && length > 0)
			TextList().push_back(std::make_shared<FDepotResultText>(DepotResultChannel::StdOut, DepotString(data, size_t(length))));
		else
			TextList().push_back(std::make_shared<FDepotResultText>(DepotResultChannel::StdOut, ""));
	}

	virtual void OutputInfo(char level, const char* data) override
	{
		if (m_Result->OnStreamInfo(this, level, data) == DepotResultReply::Handled)
			return;

		if (level > 0 && StringInfo::IsNullOrEmpty(data) == false)
		{
			if (m_TagLevel == '0' || TagList().size() == 0)
				TagList().push_back(std::make_shared<FDepotResultTag>());
			if (const char* delim = strchr(data, ' '))
				TagList().back()->m_Fields.insert(Map<DepotString, DepotString>::value_type(DepotString(data, delim-data), DepotString(delim+1)));
			else
				TagList().back()->m_Fields.insert(Map<DepotString, DepotString>::value_type(data, ""));
		}
		m_TagLevel = level;
	}

	virtual void OutputBinary(const char* data, int length) override
	{
		for (std::shared_ptr<FDepotTextEncoder>& encoder : m_Encoders)
			encoder->Encode(&data, &length);

		m_Result->OnStreamOutput(this, data, length > 0 ? size_t(length) : 0);
	}

	virtual void InputData(StrBuf* strbuf, Error* e) override
	{
		if (strbuf != nullptr)
		{
			DepotString data;
			if (m_Command->m_Input.empty() == false)
				data = m_Command->m_Input.c_str();

			m_Result->OnStreamInput(this, data);
			strbuf->Set(data.c_str());
		}
	}
	
	virtual void OutputStat(StrDict* varList) override
	{
		if (varList == nullptr)
			return;

		TagList().push_back(std::make_shared<FDepotResultTag>());
		StrRef var, val;
		for (int32_t varIndex = 0; varList->GetVar(varIndex, var, val); ++varIndex)
		{
			if (var != P4Tag::v_specdef && var != P4Tag::v_specFormatted && var != P4Tag::v_func)
				TagList().back()->m_Fields.insert(Map<DepotString, DepotString>::value_type(var.Text(), val.Text()));
		}

		m_Result->OnStreamStat(this, TagList().back());
	}

	virtual void Prompt(const StrPtr& msg, StrBuf& rsp, int noEcho, Error* e)
	{
		rsp = m_Client->OnPromptCallback(m_Command, msg.Text()).c_str();
	}

	virtual void ErrorPause(char* errBuf, Error* e) override
	{
		m_Client->OnErrorPause(errBuf);
	}

	virtual void HandleUrl(const StrPtr* url) override
	{
		if (url != nullptr && FileCore::StringInfo::IsNullOrEmpty(url->Text()) == false)
		{
			UserContext* context = m_Client->GetUserContext();
			WString cmd = StringInfo::Format(L"\"%s\\p4vfs.exe\" %s login -t 60 -u \"%s\"", FileInfo::FolderPath(FileInfo::ApplicationFilePath().c_str()).c_str(), CSTR_ATOW(m_Client->Config().ToCommandString()), CSTR_ATOW(url->Text()));
			FileOperations::CreateProcessImpersonated(cmd.c_str(), nullptr, FALSE, nullptr, context);
		}
	}

	virtual void Diff(FileSys* f1, FileSys* f2, FileSys* fout, int doPage, char* diffFlags, Error* e) override
	{
		if (e == nullptr || fout != nullptr)
			return;

		FileSys* tmpOut = FileSys::CreateGlobalTemp(FST_BINARY);
		if (tmpOut == nullptr)
			return;

		m_Client->SetEnv("DIFF", nullptr);
		m_Client->SetEnv("P4DIFF", nullptr);
		m_Client->SetEnv("P4DIFFUNICODE", nullptr);
		ClientUser::Diff(f1, f2, tmpOut, 0, diffFlags, e);

		const StrPtr* tmpPathPtr = tmpOut->FileSys::Path();
		const WString tmpPath = StringInfo::AtoW(tmpPathPtr ? tmpPathPtr->Text() : "");

		FileInfo::ReadFileLines(tmpPath.c_str(), [this](const DepotString& line) -> bool
		{
			TextList().push_back(std::make_shared<FDepotResultText>(DepotResultChannel::StdOut, line));
			return true;
		});

		SafeDeletePointer(tmpOut);
	}

	virtual void SetOutputEncoding(const DepotString& fileType) override
	{
		m_Encoders.clear();

		CharSetApi::CharSet charSet = CharSetApi::CSLOOKUP_ERROR;
		if (fileType.empty() == false)
		{
			if (strstr(fileType.c_str(), "text"))
				charSet = CharSetApi::NOCONV;
			else if (strstr(fileType.c_str(), "utf8"))
				charSet = CharSetApi::UTF_8;
			else if (strstr(fileType.c_str(), "utf16"))
				charSet = CharSetApi::UTF_16_LE;
			else if (strstr(fileType.c_str(), "unicode"))
				charSet = CharSetApi::UTF_16_LE;
		}

		if (charSet == CharSetApi::CSLOOKUP_ERROR)
			return;
		
		if (charSet != CharSetApi::NOCONV)
			m_Encoders.push_back(std::make_shared<FDepotTextEncoderCharSet>(charSet));

		FDepotTextEncoderPlatformBase::Flags::Enum platform = FDepotTextEncoderPlatformBase::Flags::None;
		
		DepotResultClient clientSpec = m_Client->Connection();
		if (clientSpec.get() && StringInfo::Stricmp(clientSpec->LineEnd().c_str(), "unix") == 0)
			platform = FDepotTextEncoderPlatformBase::Flags::None;
		else
			platform = FDepotTextEncoderPlatformBase::Flags::CrLf;

		if (charSet == CharSetApi::NOCONV && platform != FDepotTextEncoderPlatformBase::Flags::None)
			m_Encoders.push_back(std::make_shared<FDepotTextEncoderPlatform<char>>(platform));
		else if (charSet == CharSetApi::UTF_8)
			m_Encoders.push_back(std::make_shared<FDepotTextEncoderPlatform<char>>(platform|FDepotTextEncoderPlatformBase::Flags::TextUtf8));
		else if (charSet == CharSetApi::UTF_16_LE)
			m_Encoders.push_back(std::make_shared<FDepotTextEncoderPlatform<wchar_t>>(platform|FDepotTextEncoderPlatformBase::Flags::TextUtf16));
	}

private:
	static LogChannel::Enum GetErrorLogChannel(Error* err)
	{
		if (err->IsInfo())
			return LogChannel::Info;
		if (err->IsWarning())
			return LogChannel::Warning;
		return LogChannel::Error;
	}

public:
	char m_TagLevel;
	FDepotClient* m_Client;
	const DepotCommand* m_Command;
	FDepotResult* m_Result;
	Array<std::shared_ptr<FDepotTextEncoder>> m_Encoders;
};

struct FDepotClient::Api
{
	DepotConfig m_Config;
	std::shared_ptr<ClientApi> m_ClientApi;
	std::shared_ptr<Error> m_Error;
	std::shared_ptr<FDepotResultClient> m_Connection;
	DepotTimer m_AccessTime;
	FileContext* m_FileContext;
	Flags::Enum m_Flags;
	bool m_IsServerUnicode;
	int32_t m_ServerApiLevel;
	DepotClientLogCallback m_OnErrorCallback;
	DepotClientLogCallback m_OnMessageCallback;

	Api() :
		m_FileContext(nullptr),
		m_Flags(Flags::None),
		m_IsServerUnicode(false),
		m_ServerApiLevel(0)
	{}
};

FDepotClient::FDepotClient(FileContext* context)
{
	m_P4 = new FDepotClient::Api();
	m_P4->m_FileContext = context;
}

FDepotClient::~FDepotClient()
{
	Reset();

	delete m_P4;
	m_P4 = nullptr;
}

template <typename... ArgTypes>
DepotClient FDepotClient::New(ArgTypes&&... args)
{
	return std::make_shared<FDepotClient>(std::forward<ArgTypes>(args)...);
}

template P4VFS_CORE_API DepotClient FDepotClient::New<FileContext*>(FileContext*&&);
template P4VFS_CORE_API DepotClient FDepotClient::New<FileContext*const&>(FileContext*const&);

bool FDepotClient::Connect(const char* user, const char* client, const char* port, const char* passwd)
{
	DepotConfig config;
	DepotConfig::SetNonEmpty(config.m_User, user);
	DepotConfig::SetNonEmpty(config.m_Client, client);
	DepotConfig::SetNonEmpty(config.m_Port, port);
	DepotConfig::SetNonEmpty(config.m_Passwd, passwd);
	return Connect(config);
}

bool FDepotClient::Connect(const DepotConfig& config)
{
	Reset();
	m_P4->m_Config.Apply(config);
	m_P4->m_ClientApi = std::make_shared<ClientApi>();
	m_P4->m_ClientApi->SetUser(m_P4->m_Config.m_User.c_str());
	m_P4->m_ClientApi->SetPort(m_P4->m_Config.m_Port.c_str());
	m_P4->m_ClientApi->SetClient(m_P4->m_Config.m_Client.c_str());
	m_P4->m_ClientApi->SetHost(GetHostName().c_str());
	m_P4->m_ClientApi->SetCwd(m_P4->m_Config.m_Directory.c_str());
	m_P4->m_ClientApi->SetPassword(m_P4->m_Config.m_Passwd.c_str());
	m_P4->m_ClientApi->SetProg(GetProgramName().c_str());
	m_P4->m_ClientApi->SetTicketFile(GetTicketsFilePath().c_str());
	m_P4->m_ClientApi->SetTrustFile(GetTrustFilePath().c_str());
	m_P4->m_ClientApi->SetProtocol(P4Tag::v_api, DepotProtocol::CLIENT_2016_1_STRING);
	m_P4->m_ClientApi->SetProtocol(P4Tag::v_specstring, ""); 
	m_P4->m_ClientApi->SetProtocol(P4Tag::v_enableStreams, "");

	m_P4->m_Error = std::make_shared<Error>();
	m_P4->m_ClientApi->Init(m_P4->m_Error.get());
	if (HasError())
	{
		m_P4->m_ClientApi.reset();
		return false;
	}

	if (HasFlag(Flags::DisableLogin) == false && IsLoginRequired())
	{
		if (Login() == false)
		{
			return false;
		}
	}
	
	if (m_P4->m_Connection.get() == nullptr)
	{
		m_P4->m_Connection = Client();
	}

	m_P4->m_Config.SetClient(m_P4->m_ClientApi->GetClient().Text());
	m_P4->m_Config.SetHost(m_P4->m_ClientApi->GetHost().Text());
	m_P4->m_Config.SetUser(m_P4->m_ClientApi->GetUser().Text());
	m_P4->m_Config.SetPort(m_P4->m_ClientApi->GetPort().Text());
	m_P4->m_Config.SetDirectory(m_P4->m_ClientApi->GetCwd().Text());
	m_P4->m_IsServerUnicode = !!GetProtocol(P4Tag::v_unicode);
	m_P4->m_ServerApiLevel = GetProtocol(P4Tag::v_server2);
	return IsConnected();
}

void FDepotClient::Disconnect()
{
	Reset();
}

void FDepotClient::Reset()
{
	m_P4->m_Config.Reset();
	m_P4->m_Error.reset();
	m_P4->m_Connection.reset();
	m_P4->m_AccessTime.Reset();
	if (m_P4->m_ClientApi.get())
	{
		m_P4->m_Error = std::make_shared<Error>();
		m_P4->m_ClientApi->Final(m_P4->m_Error.get());
		m_P4->m_ClientApi.reset();
	}
}

bool FDepotClient::HasError() const
{
	return m_P4->m_Error.get() ? m_P4->m_Error->Test() : false;
}

bool FDepotClient::IsConnected()
{
	return m_P4->m_ClientApi.get() ? !m_P4->m_ClientApi->Dropped() : false;
}

bool FDepotClient::IsConnectedClient()
{
	return IsConnected() && m_P4->m_Connection->Access().empty() == false;
}

bool FDepotClient::IsLoginRequired()
{
	if (!IsConnected())
	{
		return false;
	}

	m_P4->m_Connection = Client();
	if (m_P4->m_Connection->HasErrorRegex("use the 'p4 trust' command"))
	{
		Trust();
		m_P4->m_Connection = Client();
	}

	m_P4->m_Config.Apply(m_P4->m_Connection->Config());
	if (m_P4->m_Connection->Access().empty() == false)
	{
		return false;
	}

	if (Run("login", DepotStringArray{"-s"})->HasError() == false)
	{
		return false;
	}
	return true;
}

bool FDepotClient::Login(DepotClientPromptCallback prompt)
{
	DepotCommand cmd;
	cmd.m_Name = "login";
	cmd.m_Prompt = prompt;
	return Run(cmd)->HasError() == false;
}

DepotResult FDepotClient::Trust()
{
	return Run("trust", DepotStringArray{"-y", "-f"});
}

DepotResultClient FDepotClient::Connection()
{
	return m_P4->m_Connection;
}

DepotResultClient FDepotClient::Client()
{
	return Run<DepotResultClient>("client", DepotStringArray{"-o"});
}

DepotResultInfo FDepotClient::Info()
{
	return Run<DepotResultInfo>("info");
}

DepotString FDepotClient::GetErrorText() const
{
	DepotString text;
	if (m_P4->m_Error.get())
	{
		StrBuf msg;
		m_P4->m_Error->Fmt(&msg);
		text = FileCore::StringInfo::ToString(msg.Text());
	}
	return text;
}

int32_t FDepotClient::GetProtocol(const char* tag, int32_t defaultValue) const
{
	if (m_P4->m_ClientApi.get() && StringInfo::IsNullOrEmpty(tag) == false)
	{
		const StrPtr* protocol = m_P4->m_ClientApi->GetProtocol(tag);
		if (protocol != nullptr && protocol->Length())
		{
			return protocol->Atoi();
		}
	}
	return defaultValue;
}

int32_t FDepotClient::GetServerApiLevel() const
{
	return m_P4->m_ServerApiLevel;
}

bool FDepotClient::IsServerUnicode() const
{
	return m_P4->m_IsServerUnicode;
}

DepotString FDepotClient::GetProgramName() const
{
	static const DepotString name = StringInfo::ToAnsi(StringInfo::Format(L"%s/" P4VFS_VER_VERSION_STRING, FileInfo::FileTitle(FileInfo::ApplicationFilePath().c_str()).c_str()));
	return name;
}

DepotString FDepotClient::GetTicketsFilePath() const
{
	const WString envTicketsPath = GetEnvImpersonatedW(DepotConstants::P4TICKETS);
	if (envTicketsPath.empty() == false)
	{
		if (FileInfo::CreateWritableFile(envTicketsPath.c_str()))
		{
			return StringInfo::ToAnsi(FileInfo::FullPath(envTicketsPath.c_str()));
		}
	}

	const DepotString configTicketsPath = GetConfigEnv(DepotConstants::P4TICKETS);
	if (configTicketsPath.empty() == false)
	{
		const WString configTicketsPathW = StringInfo::ToWide(configTicketsPath);
		if (FileInfo::CreateWritableFile(configTicketsPathW.c_str()))
		{
			return StringInfo::ToAnsi(FileInfo::FullPath(configTicketsPathW.c_str()));
		}
	}

	const WString profileFolder = GetEnvImpersonatedW(DepotConstants::USERPROFILE);
	if (FileInfo::IsDirectory(profileFolder.c_str()))
	{
		WString profileTicketsPath = FileInfo::FullPath(StringInfo::Format(L"%s\\p4tickets.txt", profileFolder.c_str()).c_str());
		if (FileInfo::CreateWritableFile(profileTicketsPath.c_str()))
		{
			return StringInfo::ToAnsi(profileTicketsPath);
		}
	}

	wchar_t userName[512] = {0};
	DWORD userNameSize = _countof(userName)-1;
	if (GetUserName(userName, &userNameSize))
	{
		WString userTicketsPath = StringInfo::Format(L"C:\\Users\\%s\\p4tickets.txt", userName);
		if (FileInfo::CreateWritableFile(userTicketsPath.c_str()))
		{
			return StringInfo::ToAnsi(userTicketsPath);
		}
	}

	// Absolute last resort... this may be the service profile folder
	wchar_t currentProfileFolder[512] = {0};
	if (ExpandEnvironmentStrings(L"%USERPROFILE%\\p4tickets.txt", currentProfileFolder, _countof(currentProfileFolder)) != 0)
	{
		if (FileInfo::CreateWritableFile(currentProfileFolder))
		{
			return StringInfo::ToAnsi(currentProfileFolder);
		}
	}
	return DepotString();
}

DepotString FDepotClient::GetTrustFilePath() const
{
	const WString envTrustPath = GetEnvImpersonatedW(DepotConstants::P4TRUST);
	if (envTrustPath.empty() == false)
	{
		if (FileInfo::CreateWritableFile(envTrustPath.c_str()))
		{
			return StringInfo::ToAnsi(FileInfo::FullPath(envTrustPath.c_str()));
		}
	}

	const WString profileFolder = GetEnvImpersonatedW(DepotConstants::USERPROFILE);
	if (FileInfo::IsDirectory(profileFolder.c_str()))
	{
		WString profileTicketsPath = FileInfo::FullPath(StringInfo::Format(L"%s\\p4trust.txt", profileFolder.c_str()).c_str());
		if (FileInfo::CreateWritableFile(profileTicketsPath.c_str()))
		{
			return StringInfo::ToAnsi(profileTicketsPath);
		}
	}

	wchar_t userName[512] = {0};
	DWORD userNameSize = _countof(userName)-1;
	if (GetUserName(userName, &userNameSize))
	{
		WString userTicketsPath = StringInfo::Format(L"C:\\Users\\%s\\p4trust.txt", userName);
		if (FileInfo::CreateWritableFile(userTicketsPath.c_str()))
		{
			return StringInfo::ToAnsi(userTicketsPath);
		}
	}

	// Absolute last resort... this may be the service profile folder
	wchar_t currentProfileFolder[512] = {0};
	if (ExpandEnvironmentStrings(L"%USERPROFILE%\\p4trust.txt", currentProfileFolder, _countof(currentProfileFolder)) != 0)
	{
		if (FileInfo::CreateWritableFile(currentProfileFolder))
		{
			return StringInfo::ToAnsi(currentProfileFolder);
		}
	}
	return DepotString();
}

DepotString FDepotClient::GetClientOwnerUserName(const DepotString& clientName, const DepotString& portName)
{
	if (StringInfo::IsNullOrEmpty(clientName.c_str()))
	{
		return DepotString();
	}

	Array<DepotString> tickets;
	if (FileInfo::ReadFileLines(StringInfo::AtoW(GetTicketsFilePath()), tickets) == false)
	{
		return DepotString();
	}

	std::set<DepotString> depotUsers;
	for (const DepotString& line : tickets)
	{
		std::match_results<const char*> match;
		if (std::regex_search(line.c_str(), match, std::regex("=\\s*(.+?)\\s*:")))
		{
			depotUsers.insert(match[1]);
		}
	}

	for (const DepotString& depotUser : depotUsers)
	{
		FDepotClient localClient;
		localClient.m_P4->m_FileContext = m_P4->m_FileContext;
		localClient.m_P4->m_Flags = Flags::DisableLogin;
		
		DepotConfig localConfig;
		localConfig.m_Client = clientName;
		localConfig.m_Port = portName;
		localConfig.m_User = depotUser;
		if (localClient.Connect(localConfig) == false)
		{
			continue;
		}
		DepotString ownerName = localClient.Client()->Owner();
		if (ownerName.empty() == false)
		{
			return ownerName;
		}
	}
	return DepotString(); 
}

DepotString FDepotClient::GetHostName() const
{
	if (m_P4->m_Config.m_Host.empty() == false)
	{
		return m_P4->m_Config.m_Host;
	}

	DepotString envHostName = GetEnvImpersonated(DepotConstants::P4HOST);
	if (envHostName.empty() == false)
	{
		return envHostName;
	}

	char strHostName[1024] = {0};
	DWORD dwHostNameLen = _countof(strHostName)-1;
 	if (GetComputerNameA(strHostName, &dwHostNameLen))
	{
		return strHostName;
	}

	return GetEnv(DepotConstants::COMPUTERNAME);
}

DepotString FDepotClient::GetConfigEnv(const char* envVarName) const
{
	if (m_P4->m_ClientApi.get() != nullptr)
	{
		const StrArray* configFiles = m_P4->m_ClientApi->GetConfigs();
		if (configFiles != nullptr && configFiles->Count() > 0)
		{
			if (Enviro* apiEnv = m_P4->m_ClientApi->GetEnviro())
			{
				const char* envVarValue = apiEnv->Get(envVarName);
				if (StringInfo::IsNullOrEmpty(envVarValue) == false)
				{
					return DepotString(envVarValue);
				}
			}
		}
	}
	return DepotString();
}

bool FDepotClient::Login()
{
	if (LoginUsingConfig())
	{
		return true;
	}
	if (LoginUsingClientOwner())
	{
		return true;
	}
	if (LoginUsingInteractiveSession())
	{
		return true;
	}
	return false;
}

bool FDepotClient::LoginUsingConfig()
{
	if (m_P4->m_Config.m_Passwd.empty())
	{
		m_P4->m_Config.m_Passwd = GetEnvImpersonated(DepotConstants::P4PASSWD);
	}

	DepotClientPromptCallback prompt = std::make_shared<FDepotClientPromptCallback>([this](const DepotString& message) -> DepotString
	{
		if (StringInfo::Contains(message.c_str(), "password", StringInfo::SearchCase::Insensitive))
		{
			return m_P4->m_Config.m_Passwd;
		}
		return DepotString();
	});

	if (Login(prompt) == false)
	{
		return false;
	}

	if (IsLoginRequired())
	{
		return false;
	}
	return true;
}

bool FDepotClient::LoginUsingClientOwner()
{
	if (IsConnected() == false)
	{
		return false;
	}

	DepotString ownerUserName = GetClientOwnerUserName(m_P4->m_Config.m_Client, m_P4->m_Config.m_Port);
	if (ownerUserName.empty() || ownerUserName == m_P4->m_Config.m_User)
	{
		return false;
	}

	m_P4->m_ClientApi->Final(m_P4->m_Error.get());
	if (HasError())
	{
		return false;
	}

	m_P4->m_Config.m_User = ownerUserName;
	m_P4->m_Config.m_Passwd.clear();
	m_P4->m_ClientApi->SetUser(m_P4->m_Config.m_User.c_str());
	m_P4->m_ClientApi->SetPassword(m_P4->m_Config.m_Passwd.c_str());
	m_P4->m_ClientApi->Init(m_P4->m_Error.get());
	if (HasError())
	{
		return false;
	}

	if (IsLoginRequired())
	{
		return false;
	}
	return true;
}

bool FDepotClient::LoginUsingInteractiveSession()
{
	if (HasFlag(Flags::Unattended))
	{
		return false;
	}

	if (FileCore::SettingManager::StaticInstance().Unattended.GetValue())
	{
		return false;
	}

	const UserContext* context = GetUserContext();
	if (FileOperations::IsSystemUserContext(context))
	{
		return false;
	}

	DepotResult ticket = Run("login", DepotStringArray{"-s"});
	if (ticket->HasError() == false)
	{
		return true;
	}

	if (ticket->HasErrorRegex("user .+ doesn't exist"))
	{
		return false;
	}

	DepotClientPromptCallback prompt = std::make_shared<FDepotClientPromptCallback>([this](const DepotString& message) -> DepotString
	{
		if (StringInfo::Contains(message.c_str(), "password", StringInfo::SearchCase::Insensitive))
		{
			DepotString passwd;
			if (RequestInteractivePassword(passwd))
			{
				return passwd;
			}
		}
		return DepotString();
	});

	if (Login(prompt) == false)
	{
		return false;
	}

	if (IsLoginRequired())
	{
		return false;
	}
	return true;
}

const DepotConfig& FDepotClient::Config() const
{
	return m_P4->m_Config;
}

void FDepotClient::Run(const DepotCommand& cmd, FDepotResult& result)
{
	if (m_P4->m_ClientApi.get() == nullptr)
	{
		result.SetError("DepotClient invalid ClientApi");
		return;
	}
	if (IsConnected() == false)
	{
		result.SetError("DepotClient not connected");
		return;
	}

	Array<const char*> argv;
	if (cmd.m_Args.size())
	{
		argv.reserve(cmd.m_Args.size());
		for (const DepotString& arg : cmd.m_Args)
			argv.push_back(arg.c_str());
	}

	DepotClientCommand clientCmd(this, &cmd, &result);
	m_P4->m_ClientApi->SetArgv(int32_t(argv.size()), argv.size() ? const_cast<char* const*>(argv.data()) : nullptr);
	m_P4->m_ClientApi->SetVar(P4Tag::v_tag, cmd.m_Flags & DepotCommand::Flags::UnTagged ? 0 : "yes");
	m_P4->m_ClientApi->Run(cmd.m_Name.c_str(), &clientCmd);
	m_P4->m_AccessTime.Reset();

	result.OnComplete();
}

bool FDepotClient::RequestInteractivePassword(DepotString& passwd)
{
	WString output;
	UserContext* context = m_P4->m_FileContext ? m_P4->m_FileContext->m_UserContext : nullptr;
	WString cmd = StringInfo::Format(L"\"%s\\p4vfs.exe\" %s login -i -w", FileInfo::FolderPath(FileInfo::ApplicationFilePath().c_str()).c_str(), CSTR_ATOW(m_P4->m_Config.ToCommandString()));
	if (SUCCEEDED(FileOperations::CreateProcessImpersonated(cmd.c_str(), nullptr, TRUE, &output, context)))
	{
		WStringArray lines = StringInfo::Split(output.c_str(), L"\n\r", StringInfo::SplitFlags::RemoveEmptyEntries);
		for (const WString& line : lines)
		{
			std::match_results<const wchar_t*> match;
			if (std::regex_search(line.c_str(), match, std::wregex(L"P4PASSWD=(.*)")))
			{
				passwd = StringInfo::ToAnsi(match[1]);
				return true;
			}
		}
	}
	return false;
}

void FDepotClient::OnErrorPause(const char* message)
{
}

void FDepotClient::OnErrorCallback(LogChannel::Enum channel, const char* severity, const char* text)
{
	if (m_P4->m_OnErrorCallback.get() && *m_P4->m_OnErrorCallback.get())
	{
		(*m_P4->m_OnErrorCallback.get())(channel, severity, text);
	}
}

void FDepotClient::OnMessageCallback(LogChannel::Enum channel, const char* severity, const char* text)
{
	if (m_P4->m_OnMessageCallback.get() && *m_P4->m_OnMessageCallback.get())
	{
		(*m_P4->m_OnMessageCallback.get())(channel, severity, text);
	}
}

DepotString FDepotClient::OnPromptCallback(const DepotCommand* command, const char* message)
{
	if (command && command->m_Prompt.get() && *command->m_Prompt.get())
	{
		return (*command->m_Prompt.get())(message ? message : "");
	}
	return DepotString();
}

LogDevice* FDepotClient::Log()
{ 
	return m_P4->m_FileContext ? m_P4->m_FileContext->m_LogDevice : nullptr; 
}

void FDepotClient::Log(LogChannel::Enum channel, const DepotString& text)
{
	LogDevice::WriteLine(Log(), channel, text);
}

bool FDepotClient::IsFaulted()
{
	return LogDevice::IsFaulted(Log());
}

FDepotClient::Flags::Enum FDepotClient::GetFlags() const 
{ 
	return m_P4->m_Flags; 
}

void FDepotClient::SetFlags(Flags::Enum flags) 
{ 
	m_P4->m_Flags = flags; 
}

bool FDepotClient::HasFlag(Flags::Enum flag) const 
{ 
	return !!(m_P4->m_Flags & flag); 
}

void FDepotClient::ApplyFlag(Flags::Enum flag, bool set)
{
	if (set)
	{
		m_P4->m_Flags |= flag;
	}
	else
	{
		m_P4->m_Flags &= ~flag;
	}
}

void FDepotClient::SetContext(FileContext* fileContext) 
{ 
	m_P4->m_FileContext = fileContext; 
}

FileContext* FDepotClient::GetContext() const
{
	return m_P4->m_FileContext;
}

UserContext* FDepotClient::GetUserContext() const
{
	return m_P4->m_FileContext ? m_P4->m_FileContext->m_UserContext : nullptr;
}

time_t FDepotClient::GetAccessTimeSpan() const
{
	return m_P4->m_AccessTime.TotalSeconds();
}

bool FDepotClient::SetEnv(const char* name, const char* value)
{
	if (StringInfo::IsNullOrEmpty(name) == false)
	{
		if (Enviro* env = m_P4->m_ClientApi->GetEnviro())
		{
			env->Update(name, value ? value : "");
			return true;
		}
	}
	return false;
}

DepotString FDepotClient::GetEnv(const char* name) const
{
	if (StringInfo::IsNullOrEmpty(name) == false)
	{
		if (Enviro* env = m_P4->m_ClientApi->GetEnviro())
		{
			if (const char* value = env->Get(name))
			{
				return DepotString(value);
			}
		}
	}
	return DepotString();
}

DepotString FDepotClient::GetEnvImpersonated(const char* name) const
{
	return StringInfo::ToAnsi(GetEnvImpersonatedW(name));
}

WString FDepotClient::GetEnvImpersonatedW(const char* name) const
{
	if (StringInfo::IsNullOrEmpty(name) == false)
	{
		WString envStr;
		envStr += L"%";
		envStr += StringInfo::ToWide(name);
		envStr += L"%";

		wchar_t envValue[1024] = {0};
		if (SUCCEEDED(FileOperations::GetImpersonatedEnvironmentStrings(envStr.c_str(), envValue, _countof(envValue), GetUserContext())))
		{
			return envValue;
		}
	}
	return WString();
}

void FDepotClient::SetErrorCallback(DepotClientLogCallback callback)
{
	m_P4->m_OnErrorCallback = callback;
}

void FDepotClient::SetMessageCallback(DepotClientLogCallback callback)
{
	m_P4->m_OnMessageCallback = callback;
}

time_t DepotInfo::StringToTime(const DepotString& text)
{
	Error e;
	DateTime dt;
	dt.Set(text.c_str(), &e);
	return e.Test() ? 0 : dt.Value();
}

DepotString DepotInfo::TimeToString(const time_t& tm)
{
	char timestr[256] = {0};
	DateTime dt;
	dt.Set(int32_t(tm));
	dt.Fmt(timestr);
	DepotString ts = StringInfo::Replace(timestr, ' ', ':');
	return ts;
}

bool DepotInfo::IsWritableFileType(const DepotString& fileType)
{
	if (const char* p = FileCore::StringInfo::Strchr(fileType.c_str(), '+'))
	{
		return !!FileCore::StringInfo::Strichr(p+1, 'w');
	}
	return false;
}

bool DepotInfo::IsSymlinkFileType(const DepotString& fileType)
{
	return !!FileCore::StringInfo::Stristr(fileType.c_str(), "symlink");
}

DepotConfig DepotInfo::DepotConfigFromPath(const DepotString& folderPath)
{
	ClientApi clientApi;
	clientApi.SetCwd(folderPath.c_str());

	DepotConfig config;
	config.m_Directory = folderPath;
	config.m_Port = FileCore::StringInfo::ToString(clientApi.GetPort().Text());
	config.m_User = FileCore::StringInfo::ToString(clientApi.GetUser().Text());
	config.m_Passwd = FileCore::StringInfo::ToString(clientApi.GetPassword().Text());
	config.m_Client = FileCore::StringInfo::ToString(clientApi.GetClient().Text());
	return config;
}

int32_t DepotTunable::Get(const DepotString& name, int32_t defaultValue)
{
	int32_t index = p4tunable.GetIndex(name.c_str());
	return index >= 0 ? p4tunable.Get(index) : defaultValue;
}

void DepotTunable::Set(const DepotString& name, int32_t value)
{
	return p4tunable.Set(FileCore::StringInfo::Format("%s=%d", name.c_str(), value).c_str());
}

void DepotTunable::Unset(const DepotString& name)
{
	return p4tunable.Unset(name.c_str());
}

bool DepotTunable::IsSet(const DepotString& name)
{
	return !!p4tunable.IsSet(name.c_str());
}

bool DepotTunable::IsKnown(const DepotString& name)
{
	return !!p4tunable.IsKnown(name.c_str());
}

}}}
