// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotSyncAction.h"
#include "FileAssert.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

DepotString DepotSyncType::ToString(DepotSyncType::Enum value)
{
	DepotString result;
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncType, Normal);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncType, Force);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncType, Flush);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncType, Preview);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncType, IgnoreOutput);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncType, Quiet);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncType, Writeable);
	return result;
}

DepotString DepotFlushType::ToString(DepotFlushType::Enum value)
{
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotFlushType, Single);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotFlushType, Atomic);
	return DepotString();
}

bool DepotSyncAction::IsError(DepotSyncAction::Enum value)
{
	switch (value)
	{
		case DepotSyncAction::None:
		case DepotSyncAction::NoFileAtRevision:
		case DepotSyncAction::InvalidPattern:
		case DepotSyncAction::NotInClientView:
		case DepotSyncAction::NoFilesFound:
		case DepotSyncAction::UpToDate:
		case DepotSyncAction::GenericError:
			return true;
	}
	return false;
}

bool DepotSyncAction::IsChanged(DepotSyncAction::Enum value)
{
	switch (value)
	{
		case DepotSyncAction::Added:
		case DepotSyncAction::Deleted:
		case DepotSyncAction::Updated:
		case DepotSyncAction::Refreshed:
		case DepotSyncAction::Replaced:
			return true;
	}
	return false;
}

bool DepotSyncAction::IsLocalChanged(DepotSyncAction::Enum value)
{
	switch (value)
	{
		case DepotSyncAction::Added:
		case DepotSyncAction::Updated:
		case DepotSyncAction::Refreshed:
		case DepotSyncAction::Replaced:
			return true;
	}
	return false;
}

DepotString DepotSyncAction::ToString(DepotSyncAction::Enum value)
{
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncAction, None);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncAction, Added);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncAction, Deleted);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncAction, Updated);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncAction, Refreshed);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncAction, Replaced);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncAction, UpToDate);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncAction, NoFilesFound);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncAction, NoFileAtRevision);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncAction, InvalidPattern);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncAction, NotInClientView);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncAction, OpenedNotChanged);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncAction, CantClobber);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncAction, NeedsResolve);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncAction, GenericError);
	return DepotString();
}

DepotSyncAction::Enum DepotSyncAction::FromString(const DepotString& value)
{
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncAction, None);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncAction, Added);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncAction, Deleted);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncAction, Updated);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncAction, Refreshed);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncAction, Replaced);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncAction, UpToDate);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncAction, NoFilesFound);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncAction, NoFileAtRevision);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncAction, InvalidPattern);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncAction, NotInClientView);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncAction, OpenedNotChanged);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncAction, CantClobber);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncAction, NeedsResolve);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncAction, GenericError);
	return DepotSyncAction::None;
}

DepotString DepotSyncOption::ToString(DepotSyncOption::Enum value)
{
	DepotString result;
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncOption, None);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncOption, FileWrite);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncOption, HaveFileWrite);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncOption, ClientWrite);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncOption, ClientClobber);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncOption, FileSymlink);
	return result;
}

FDepotSyncActionInfo::FDepotSyncActionInfo() :
	m_DepotFile(),
	m_ClientFile(),
	m_FileSize(0),
	m_Revision(),
	m_SyncAction(DepotSyncAction::None),
	m_SyncOptions(DepotSyncOption::None),
	m_SyncType(DepotSyncType::Normal),
	m_FlushType(DepotFlushType::Atomic),
	m_DiskFileSize(0),
	m_VirtualFileSize(0),
	m_IsAlwaysResident(false),
	m_PlaceholderTime(0),
	m_FlushTime(0),
	m_SyncTime(0),
	m_Message(),
	m_SubActions()
{
}

FDepotSyncActionInfo::~FDepotSyncActionInfo()
{
}

int32_t FDepotSyncActionInfo::RevisionNumber() const
{
	const FDepotRevisionNumber* rn = FDepotRevision::Cast<const FDepotRevisionNumber*>(m_Revision.get());
	return rn ? rn->m_Value : 0; 
}

bool FDepotSyncActionInfo::CanModifyWritableFile() const
{
	if (m_SyncType & DepotSyncType::Force)
		return true;
	if (m_SyncOptions & (DepotSyncOption::ClientClobber | DepotSyncOption::ClientWrite | DepotSyncOption::FileWrite | DepotSyncOption::HaveFileWrite | DepotSyncOption::FileSymlink))
		return true;
	return false;
}

bool FDepotSyncActionInfo::CanSetWritableFile() const
{
	if (m_SyncOptions & (DepotSyncOption::ClientWrite | DepotSyncOption::FileWrite))
		return true;
	return false;
}

bool FDepotSyncActionInfo::IsPreview() const
{
	if (m_SyncType & DepotSyncType::Preview)
		return true;
	return false;
}

DepotSyncActionInfo FDepotSyncActionInfo::FromInfoOutput(const DepotString& infoText, FileCore::LogDevice* log)
{
	struct InfoRegex
	{
		std::regex m_FileRev = std::regex("^([^#]*)(#\\w+)? - (.+)");
		std::regex m_ActionOpened = std::regex("^is opened .+");
		std::regex m_ActionDeleted = std::regex("^deleted as (.+)");
		std::regex m_ActionAdded = std::regex("^added as (.+)");
		std::regex m_ActionUpdated = std::regex("^updating (.+)");
		std::regex m_ActionNeedsResolve = std::regex("^(...\\s+)*(.*) - must resolve( (#\\w+))? before submitting");
	};
	static const InfoRegex rx;

	DepotSyncActionInfo info = std::make_shared<FDepotSyncActionInfo>();
	info->m_Message = StringInfo::TrimRight(infoText.c_str());

	std::match_results<const char*> match;
	if (std::regex_search(infoText.c_str(), match, rx.m_FileRev))
	{
		info->m_DepotFile = match[1];
		info->m_Revision = FDepotRevision::FromString(match[2]);
		DepotString fileRevAction = match[3];

		if (std::regex_search(fileRevAction.c_str(), match, rx.m_ActionOpened))
		{
			info->m_SyncAction = DepotSyncAction::OpenedNotChanged;
		}
		else if (std::regex_search(fileRevAction.c_str(), match, rx.m_ActionDeleted))
		{
			info->m_SyncAction = DepotSyncAction::Deleted;
			info->m_ClientFile = match[1];
		}
		else if (std::regex_search(fileRevAction.c_str(), match, rx.m_ActionAdded))
		{
			info->m_SyncAction = DepotSyncAction::Added;
			info->m_ClientFile = match[1];
		}
		else if (std::regex_search(fileRevAction.c_str(), match, rx.m_ActionUpdated))
		{
			info->m_SyncAction = DepotSyncAction::Updated;
			info->m_ClientFile = match[1];
		}
	}
	else if (std::regex_search(infoText.c_str(), match, rx.m_ActionNeedsResolve))
	{
		info->m_DepotFile = match[2];
		info->m_Revision = FDepotRevision::FromString(match[4]);
		info->m_SyncAction = DepotSyncAction::NeedsResolve;
	}

	if (info->m_SyncAction == DepotSyncAction::None)
	{
		if (log != nullptr)
			log->Error(StringInfo::Format("Failed to parse DepotSyncActionInfo InfoOutput '%s'", infoText.c_str()));
		return nullptr;
	}
	return info;
}

DepotSyncActionInfo FDepotSyncActionInfo::FromTaggedOutput(const FDepotResultTag& tag, FileCore::LogDevice* log)
{
	const DepotString& action = tag.GetValue("action");
	DepotSyncAction::Enum syncAction = DepotSyncAction::FromString(action);
	
	if (syncAction == DepotSyncAction::None)
	{
		if (log != nullptr)
			log->Error(StringInfo::Format("Failed to parse action tag '%s'", action.c_str()));
		return nullptr;
	}

	DepotSyncActionInfo info = std::make_shared<FDepotSyncActionInfo>();

	info->m_SyncAction = syncAction;
	info->m_Revision = FDepotRevision::New<FDepotRevisionNumber>(tag.GetValueInt32("rev"));
	info->m_ClientFile = tag.GetValue("clientFile");
	info->m_DepotFile = tag.GetValue("depotFile");
	info->m_FileSize = tag.GetValueInt64("fileSize");
	return info;
}

DepotSyncActionInfo FDepotSyncActionInfo::FromErrorOutput(const DepotString& errorText, FileCore::LogDevice* log)
{
	struct ErrorRegex
	{
		std::regex m_FileRev = std::regex("^([^#]*)(#\\w+)? - (.+)");
		std::regex m_ActionNoFileAtRevision = std::regex("^no file\\(s\\) at that revision");
		std::regex m_ActionInvalidPattern = std::regex("^no such file\\(s\\)");
		std::regex m_ActionNotInClientView = std::regex("^file\\(s\\) not in client view");
		std::regex m_ActionNoFilesFound = std::regex("^no file\\(s\\) at that changelist number");
		std::regex m_ActionUpToDate = std::regex("^file\\(s\\) up-to-date");
		std::regex m_ActionCantClobber = std::regex("^Can't clobber writable file (.+)");
	};
	static const ErrorRegex rx;

	DepotSyncActionInfo info = std::make_shared<FDepotSyncActionInfo>();
	info->m_Message = StringInfo::TrimRight(errorText.c_str());

	std::match_results<const char*> match;
	if (std::regex_search(errorText.c_str(), match, rx.m_FileRev))
	{
		info->m_DepotFile = match[1];
		info->m_Revision = FDepotRevision::FromString(match[2]);
		DepotString fileRevAction = match[3];

		if (std::regex_search(fileRevAction.c_str(), match, rx.m_ActionNoFileAtRevision))
		{
			info->m_SyncAction = DepotSyncAction::NoFileAtRevision;
		}
		else if (std::regex_search(fileRevAction.c_str(), match, rx.m_ActionInvalidPattern))
		{
			info->m_SyncAction = DepotSyncAction::InvalidPattern;
		}
		else if (std::regex_search(fileRevAction.c_str(), match, rx.m_ActionNotInClientView))
		{
			info->m_SyncAction = DepotSyncAction::NotInClientView;
		}
		else if (std::regex_search(fileRevAction.c_str(), match, rx.m_ActionNoFilesFound))
		{
			info->m_SyncAction = DepotSyncAction::NoFilesFound;
		}
		else if (std::regex_search(fileRevAction.c_str(), match, rx.m_ActionUpToDate))
		{
			info->m_SyncAction = DepotSyncAction::UpToDate;
		}
	}
	else if (std::regex_search(errorText.c_str(), match, rx.m_ActionCantClobber))
	{
		info->m_ClientFile = match[1];
		info->m_SyncAction = DepotSyncAction::CantClobber;
	}
	else
	{
		info->m_SyncAction = DepotSyncAction::GenericError;
	}
	return info;
}

DepotString FDepotSyncActionInfo::ToString() const
{
	return StringInfo::Format("DepotFile='%s' ClientFile='%s' DepotRevision='%s' Action='%s'", 
		m_DepotFile.c_str(), 
		m_ClientFile.c_str(), 
		FDepotRevision::ToString(m_Revision).c_str(), 
		DepotSyncAction::ToString(m_SyncAction).c_str());
}

DepotString FDepotSyncActionInfo::ToFileSpecString() const
{
	if (StringInfo::IsNullOrEmpty(m_DepotFile.c_str()) == false)
		return StringInfo::Format("%s%s", m_DepotFile.c_str(), FDepotRevision::ToString(m_Revision).c_str());
	if (StringInfo::IsNullOrEmpty(m_ClientFile.c_str()) == false)
		return StringInfo::Format("%s%s", m_ClientFile.c_str(), FDepotRevision::ToString(m_Revision).c_str());
	return DepotString();
}

DepotString DepotSyncStatus::ToString(DepotSyncStatus::Enum value)
{
	DepotString result;
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncStatus, Success);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncStatus, Warning);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncStatus, Error);
	return result;
}

DepotSyncStatus::Enum DepotSyncStatus::FromLogElement(const LogElement& element)
{
	if (element.m_Channel >= LogChannel::Error)
	{
		return DepotSyncStatus::Error;
	}
	if (element.m_Channel >= LogChannel::Warning)
	{
		return DepotSyncStatus::Warning;
	}
	return DepotSyncStatus::Success;
}

DepotSyncStatus::Enum DepotSyncStatus::FromLog(const LogDeviceMemory& log)
{
	StaticAssert(DepotSyncStatus::Success == 0);
	DepotSyncStatus::Enum status = DepotSyncStatus::Success;
	for (const LogElement& element : log.m_Elements)
	{
		status |= FromLogElement(element);
	}
	return status;
}

FDepotSyncResult::FDepotSyncResult(DepotSyncStatus::Enum status, DepotSyncActionInfoArray modifications) :
	m_Status(status),
	m_Modifications(modifications)
{
}

FDepotSyncResult::~FDepotSyncResult()
{
}

}}}
