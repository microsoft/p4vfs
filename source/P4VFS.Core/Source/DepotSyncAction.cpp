// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotSyncAction.h"
#include "FileAssert.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

DepotString DepotSyncFlags::ToString(DepotSyncFlags::Enum value)
{
	DepotString result;
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncFlags, Normal);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncFlags, Force);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncFlags, Flush);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncFlags, Preview);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncFlags, IgnoreOutput);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncFlags, Quiet);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncFlags, Writeable);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncFlags, ClientSize);
	return result;
}

DepotString DepotFlushType::ToString(DepotFlushType::Enum value)
{
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotFlushType, Single);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotFlushType, Atomic);
	return DepotString();
}

bool DepotSyncActionType::IsError(DepotSyncActionType::Enum value)
{
	switch (value)
	{
		case DepotSyncActionType::None:
		case DepotSyncActionType::NoFileAtRevision:
		case DepotSyncActionType::InvalidPattern:
		case DepotSyncActionType::NotInClientView:
		case DepotSyncActionType::NoFilesFound:
		case DepotSyncActionType::UpToDate:
		case DepotSyncActionType::GenericError:
			return true;
	}
	return false;
}

bool DepotSyncActionType::IsChanged(DepotSyncActionType::Enum value)
{
	switch (value)
	{
		case DepotSyncActionType::Added:
		case DepotSyncActionType::Deleted:
		case DepotSyncActionType::Updated:
		case DepotSyncActionType::Refreshed:
		case DepotSyncActionType::Replaced:
			return true;
	}
	return false;
}

bool DepotSyncActionType::IsLocalChanged(DepotSyncActionType::Enum value)
{
	switch (value)
	{
		case DepotSyncActionType::Added:
		case DepotSyncActionType::Updated:
		case DepotSyncActionType::Refreshed:
		case DepotSyncActionType::Replaced:
			return true;
	}
	return false;
}

DepotString DepotSyncActionType::ToString(DepotSyncActionType::Enum value)
{
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncActionType, None);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncActionType, Added);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncActionType, Deleted);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncActionType, Updated);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncActionType, Refreshed);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncActionType, Replaced);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncActionType, UpToDate);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncActionType, NoFilesFound);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncActionType, NoFileAtRevision);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncActionType, InvalidPattern);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncActionType, NotInClientView);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncActionType, OpenedNotChanged);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncActionType, CantClobber);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncActionType, NeedsResolve);
	P4VFS_ENUM_TO_STRING_RETURN(value, DepotSyncActionType, GenericError);
	return DepotString();
}

DepotSyncActionType::Enum DepotSyncActionType::FromString(const DepotString& value)
{
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncActionType, None);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncActionType, Added);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncActionType, Deleted);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncActionType, Updated);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncActionType, Refreshed);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncActionType, Replaced);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncActionType, UpToDate);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncActionType, NoFilesFound);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncActionType, NoFileAtRevision);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncActionType, InvalidPattern);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncActionType, NotInClientView);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncActionType, OpenedNotChanged);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncActionType, CantClobber);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncActionType, NeedsResolve);
	P4VFS_STRING_TO_ENUM_RETURN(value, DepotSyncActionType, GenericError);
	return DepotSyncActionType::None;
}

DepotString DepotSyncActionFlags::ToString(DepotSyncActionFlags::Enum value)
{
	DepotString result;
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncActionFlags, None);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncActionFlags, FileWrite);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncActionFlags, HaveFileWrite);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncActionFlags, ClientWrite);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncActionFlags, ClientClobber);
	P4VFS_ENUM_TO_STRING_APPEND_FLAG(result, value, DepotSyncActionFlags, FileSymlink);
	return result;
}

FDepotSyncActionInfo::FDepotSyncActionInfo() :
	m_DepotFile(),
	m_ClientFile(),
	m_FileSize(0),
	m_Revision(),
	m_SyncActionType(DepotSyncActionType::None),
	m_SyncActionFlags(DepotSyncActionFlags::None),
	m_SyncFlags(DepotSyncFlags::Normal),
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
	if (m_SyncFlags & DepotSyncFlags::Force)
		return true;
	if (m_SyncActionFlags & (DepotSyncActionFlags::ClientClobber | DepotSyncActionFlags::ClientWrite | DepotSyncActionFlags::FileWrite | DepotSyncActionFlags::HaveFileWrite | DepotSyncActionFlags::FileSymlink))
		return true;
	return false;
}

bool FDepotSyncActionInfo::CanSetWritableFile() const
{
	if (m_SyncActionFlags & (DepotSyncActionFlags::ClientWrite | DepotSyncActionFlags::FileWrite))
		return true;
	return false;
}

bool FDepotSyncActionInfo::IsPreview() const
{
	if (m_SyncFlags & DepotSyncFlags::Preview)
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
			info->m_SyncActionType = DepotSyncActionType::OpenedNotChanged;
		}
		else if (std::regex_search(fileRevAction.c_str(), match, rx.m_ActionDeleted))
		{
			info->m_SyncActionType = DepotSyncActionType::Deleted;
			info->m_ClientFile = match[1];
		}
		else if (std::regex_search(fileRevAction.c_str(), match, rx.m_ActionAdded))
		{
			info->m_SyncActionType = DepotSyncActionType::Added;
			info->m_ClientFile = match[1];
		}
		else if (std::regex_search(fileRevAction.c_str(), match, rx.m_ActionUpdated))
		{
			info->m_SyncActionType = DepotSyncActionType::Updated;
			info->m_ClientFile = match[1];
		}
	}
	else if (std::regex_search(infoText.c_str(), match, rx.m_ActionNeedsResolve))
	{
		info->m_DepotFile = match[2];
		info->m_Revision = FDepotRevision::FromString(match[4]);
		info->m_SyncActionType = DepotSyncActionType::NeedsResolve;
	}

	if (info->m_SyncActionType == DepotSyncActionType::None)
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
	DepotSyncActionType::Enum syncAction = DepotSyncActionType::FromString(action);
	
	if (syncAction == DepotSyncActionType::None)
	{
		if (log != nullptr)
			log->Error(StringInfo::Format("Failed to parse action tag '%s'", action.c_str()));
		return nullptr;
	}

	DepotSyncActionInfo info = std::make_shared<FDepotSyncActionInfo>();

	info->m_SyncActionType = syncAction;
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
			info->m_SyncActionType = DepotSyncActionType::NoFileAtRevision;
		}
		else if (std::regex_search(fileRevAction.c_str(), match, rx.m_ActionInvalidPattern))
		{
			info->m_SyncActionType = DepotSyncActionType::InvalidPattern;
		}
		else if (std::regex_search(fileRevAction.c_str(), match, rx.m_ActionNotInClientView))
		{
			info->m_SyncActionType = DepotSyncActionType::NotInClientView;
		}
		else if (std::regex_search(fileRevAction.c_str(), match, rx.m_ActionNoFilesFound))
		{
			info->m_SyncActionType = DepotSyncActionType::NoFilesFound;
		}
		else if (std::regex_search(fileRevAction.c_str(), match, rx.m_ActionUpToDate))
		{
			info->m_SyncActionType = DepotSyncActionType::UpToDate;
		}
	}
	else if (std::regex_search(errorText.c_str(), match, rx.m_ActionCantClobber))
	{
		info->m_ClientFile = match[1];
		info->m_SyncActionType = DepotSyncActionType::CantClobber;
	}
	else
	{
		info->m_SyncActionType = DepotSyncActionType::GenericError;
	}
	return info;
}

DepotString FDepotSyncActionInfo::ToString() const
{
	return StringInfo::Format("DepotFile='%s' ClientFile='%s' DepotRevision='%s' Action='%s'", 
		m_DepotFile.c_str(), 
		m_ClientFile.c_str(), 
		FDepotRevision::ToString(m_Revision).c_str(), 
		DepotSyncActionType::ToString(m_SyncActionType).c_str());
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
