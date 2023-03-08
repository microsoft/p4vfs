// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotRevision.h"
#include "DepotClient.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

FDepotRevision::FDepotRevision()
{
}

FDepotRevision::~FDepotRevision()
{
}

DepotRevision FDepotRevision::FromString(const DepotString& text)
{
	if (StringInfo::IsNullOrWhiteSpace(text.c_str()) == false)
	{
		typedef DepotRevision (*FromStringDelegate)(const DepotString&);
		constexpr FromStringDelegate fromStringDelegates[] = 
		{
			&FDepotRevisionNumber::FromString,
			&FDepotRevisionChangelist::FromString,
			&FDepotRevisionNow::FromString,
			&FDepotRevisionNone::FromString,
			&FDepotRevisionHave::FromString,
			&FDepotRevisionHead::FromString,
			&FDepotRevisionLabel::FromString,
			&FDepotRevisionRange::FromString,
			&FDepotRevisionDate::FromString,
		};

		for (const FromStringDelegate& fromString : fromStringDelegates)
		{
			DepotRevision revision = (*fromString)(text);
			if (revision.get() != nullptr)
			{
				return revision;
			}
		}
	}
	return nullptr;
}

template <typename RevType, typename... ArgTypes>
DepotRevision FDepotRevision::New(ArgTypes&&... args)
{
	return std::make_shared<RevType>(std::forward<ArgTypes>(args)...);
}

template P4VFS_CORE_API DepotRevision FDepotRevision::New<FDepotRevisionRange>();
template P4VFS_CORE_API DepotRevision FDepotRevision::New<FDepotRevisionRange>(const DepotRevision&, const DepotRevision&);
template P4VFS_CORE_API DepotRevision FDepotRevision::New<FDepotRevisionDate>(int&&);
template P4VFS_CORE_API DepotRevision FDepotRevision::New<FDepotRevisionNumber>(int&&);
template P4VFS_CORE_API DepotRevision FDepotRevision::New<FDepotRevisionNow>();
template P4VFS_CORE_API DepotRevision FDepotRevision::New<FDepotRevisionNone>();
template P4VFS_CORE_API DepotRevision FDepotRevision::New<FDepotRevisionChangelist>(int&&);
template P4VFS_CORE_API DepotRevision FDepotRevision::New<FDepotRevisionHave>();
template P4VFS_CORE_API DepotRevision FDepotRevision::New<FDepotRevisionHead>();
template P4VFS_CORE_API DepotRevision FDepotRevision::New<FDepotRevisionLabel>();
template P4VFS_CORE_API DepotRevision FDepotRevision::New<FDepotRevisionLabel>(const DepotString&);

DepotString FDepotRevision::ToString() const
{
	return DepotString();
}

DepotString FDepotRevision::ToString(const DepotRevision& revision)
{
	return revision.get() ? revision->ToString() : DepotString();
}

bool FDepotRevision::IsHeadRevision() const
{
	return IsRevisionString("#head");
}

bool FDepotRevision::IsHaveRevision() const
{
	return IsRevisionString("#have");
}

bool FDepotRevision::IsNoneRevision() const
{
	return IsRevisionString("#none");
}

bool FDepotRevision::IsRevisionString(const DepotString& text) const
{
	DepotRevision rev = FromString(text);
	DepotString revText = rev.get() ? rev->ToString() : DepotString();
	return StringInfo::Stricmp(revText.c_str(), ToString().c_str()) == 0;
}

DepotRevision FDepotRevisionNone::FromString(const DepotString& text)
{
	if (StringInfo::Stricmp(text.c_str(), "#none") == 0 || StringInfo::Stricmp(text.c_str(), "#0") == 0)
	{
		return New<FDepotRevisionNone>();
	}
	return nullptr;
}

DepotString FDepotRevisionNone::ToString() const
{
	return "#none";
}

DepotRevision FDepotRevisionHave::FromString(const DepotString& text)
{
	if (StringInfo::Stricmp(text.c_str(), "#have") == 0)
	{
		return New<FDepotRevisionHave>();
	}
	return nullptr;
}

DepotString FDepotRevisionHave::ToString() const
{
	return "#have";
}

DepotRevision FDepotRevisionHead::FromString(const DepotString& text)
{
	if (StringInfo::Stricmp(text.c_str(), "#head") == 0)
	{
		return New<FDepotRevisionHead>();
	}
	return nullptr;
}

DepotString FDepotRevisionHead::ToString() const
{
	return "#head";
}

DepotRevision FDepotRevisionNumber::FromString(const DepotString& text)
{
	std::match_results<const char*> match;
	if (std::regex_match(text.c_str(), match, std::regex("\\s*#(\\d+)\\s*")))
	{
		DepotString strValue = match.str(1);
		int32_t revision = atoi(strValue.c_str());
		return New<FDepotRevisionNumber>(revision);
	}
	return nullptr;
}

DepotString FDepotRevisionNumber::ToString() const
{
	return StringInfo::Format("#%d", m_Value);
}

DepotRevision FDepotRevisionLabel::FromString(const DepotString& text)
{
	std::match_results<const char*> match;
	if (std::regex_match(text.c_str(), match, std::regex("\\s*@([a-zA-Z]\\S+)\\s*")))
	{
		DepotString strValue = match.str(1);
		return New<FDepotRevisionLabel>(strValue);
	}
	return nullptr;
}

DepotString FDepotRevisionLabel::ToString() const
{
	return StringInfo::Format("@%s", m_Value.c_str());
}

DepotRevision FDepotRevisionRange::FromString(const DepotString& text)
{
	std::match_results<const char*> match;
	if (std::regex_match(text.c_str(), match, std::regex("\\s*([@#][^,\\s]+),([@#]?[^,\\s]+)\\s*")))
	{
		DepotString startStr = match.str(1);
		DepotString endStr = match.str(2);
		if (startStr.length() && endStr.length())
		{
			if (StringInfo::Strchr("@#", endStr[0]) == nullptr)
			{	
				endStr.insert(endStr.begin(), startStr[0]);
			}

			DepotRevision start = FDepotRevision::FromString(startStr);
			DepotRevision end = FDepotRevision::FromString(endStr);
			if (start.get() != nullptr && end.get() != nullptr)
			{
				return New<FDepotRevisionRange>(start, end);
			}
		}
	}
	else if (std::regex_match(text.c_str(), match, std::regex("\\s*@=(\\d+)\\s*")))
	{
		DepotString startEndStr = match.str(1);
		int32_t startEndCL = atoi(startEndStr.c_str());
		DepotRevision startEnd = New<FDepotRevisionChangelist>(startEndCL);
		return New<FDepotRevisionRange>(startEnd, startEnd);
	}
	return nullptr;
}

DepotString FDepotRevisionRange::ToString() const
{
	if (m_Start.get() != nullptr && m_End.get() != nullptr)
	{
		return StringInfo::Format("%s,%s", m_Start->ToString().c_str(), m_End->ToString().c_str());
	}
	return DepotString();
}

DepotRevision FDepotRevisionChangelist::FromString(const DepotString& text)
{
	std::match_results<const char*> match;
	if (std::regex_match(text.c_str(), match, std::regex("\\s*@(\\d+)\\s*")))
	{
		DepotString strValue = match.str(1);
		int32_t changelist = atoi(strValue.c_str());
		return New<FDepotRevisionChangelist>(changelist);
	}
	return nullptr;
}

DepotString FDepotRevisionChangelist::ToString() const
{
	return StringInfo::Format("@%d", m_Value);
}

DepotRevision FDepotRevisionNow::FromString(const DepotString& text)
{
	if (StringInfo::Stricmp(text.c_str(), "@now") == 0)
	{
		return New<FDepotRevisionNow>();
	}
	return nullptr;
}

DepotString FDepotRevisionNow::ToString() const
{
	return "@now";
}

DepotRevision FDepotRevisionDate::FromString(const DepotString& text)
{
	// "@yyyy/mm/dd" or "@yyyy/mm/dd:hh:mm:ss"
	std::match_results<const char*> match;
	if (std::regex_match(text.c_str(), match, std::regex("\\s*@((\\d{4})/(\\d{2})/(\\d{2})(:(\\d{2}):(\\d{2}):(\\d{2}))?)\\s*")))
	{
		time_t value = DepotInfo::StringToTime(match.str(1));
		if (value > 0)
		{
			return New<FDepotRevisionDate>(value);
		}
	}
	return nullptr;
}

DepotString FDepotRevisionDate::ToString() const
{
	return StringInfo::Format("@%s", DepotInfo::TimeToString(m_Value).c_str());
}

}}}
