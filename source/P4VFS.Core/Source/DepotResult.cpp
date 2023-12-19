// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotResult.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

FDepotResultText::FDepotResultText(DepotResultChannel channel, const DepotString& value, int32_t level) :
	m_Channel(channel),
	m_Value(value),
	m_Level(level)
{
}

bool FDepotResultTag::ContainsKey(const FDepotName& tagKey) const
{
	return GetValuePtr(tagKey) != nullptr;
}

void FDepotResultTag::RemoveKey(const FDepotName& tagKey)
{
	m_Fields.erase(tagKey);
}

void FDepotResultTag::SetValue(const FDepotName& tagKey, const DepotString& tagValue)
{
	if (tagKey.IsNone() == false)
	{
		m_Fields[tagKey] = tagValue;
	}
}

bool FDepotResultTag::TryGetValue(const FDepotName& tagKey, DepotString& tagValue) const
{
	if (const DepotString* valuePtr = GetValuePtr(tagKey))
	{
		tagValue = *valuePtr;
		return true;
	}
	return false;
}

const DepotString& FDepotResultTag::GetValue(const FDepotName& tagKey) const
{
	const DepotString* valuePtr = GetValuePtr(tagKey);
	return valuePtr ? *valuePtr : StringInfo::EmptyA();
}

const DepotString* FDepotResultTag::GetValuePtr(const FDepotName& tagKey) const
{
	if (tagKey.IsNone() == false)
	{
		DepotResultFields::const_iterator v = m_Fields.find(tagKey);
		if (v != m_Fields.end())
			return &v->second;
	}
	return nullptr;
}

int32_t FDepotResultTag::GetValueInt32(const FDepotName& tagKey, int32_t defaultValue) const
{
	const DepotString* valuePtr = GetValuePtr(tagKey);
	return valuePtr && valuePtr->empty() == false ? atoi(valuePtr->c_str()) : defaultValue;
}

int64_t FDepotResultTag::GetValueInt64(const FDepotName& tagKey, int64_t defaultValue) const
{
	const DepotString* valuePtr = GetValuePtr(tagKey);
	return valuePtr && valuePtr->empty() == false ? _atoi64(valuePtr->c_str()) : defaultValue;
}

FDepotResult::FDepotResult()
{
}

FDepotResult::~FDepotResult()
{
}

DepotResultReply FDepotResult::OnComplete()
{
	return DepotResultReply::Unhandled;
}

DepotResultReply FDepotResult::OnStreamInfo(IDepotClientCommand* cmd, char level, const char* data)
{
	return DepotResultReply::Unhandled;
}

DepotResultReply FDepotResult::OnStreamOutput(IDepotClientCommand* cmd, const char* data, size_t length)
{
	return DepotResultReply::Unhandled;
}

DepotResultReply FDepotResult::OnStreamInput(IDepotClientCommand* cmd, DepotString& data)
{
	return DepotResultReply::Unhandled;
}

DepotResultReply FDepotResult::OnStreamStat(IDepotClientCommand* cmd, const DepotResultTag& tag)
{
	return DepotResultReply::Unhandled;
}

bool FDepotResult::HasError() const
{
	return HasText(DepotResultChannel::StdErr);
}

bool FDepotResult::HasErrorRegex(const char* pattern) const
{
	if (StringInfo::IsNullOrEmpty(pattern) == false)
	{
		for (const DepotResultText& text : m_TextList)
		{
			if (int(text->m_Channel) & int(DepotResultChannel::StdErr))
			{
				std::match_results<const char*> match;
				if (std::regex_search(text->m_Value.c_str(), match, std::regex(pattern, std::regex_constants::ECMAScript|std::regex_constants::icase)))
					return true;
			}
		}
	}
	return false;
}

void FDepotResult::SetError(const char* errorText)
{
	m_TextList.clear();
	m_TextList.push_back(std::make_shared<FDepotResultText>(DepotResultChannel::StdErr, errorText ? errorText : ""));
}

DepotString FDepotResult::GetError() const
{
	return GetText(DepotResultChannel::StdErr);
}

bool FDepotResult::HasText(DepotResultChannel channel) const
{
	for (const DepotResultText& text : m_TextList)
	{
		if (int(text->m_Channel) & int(channel))
			return true;
	}
	return false;
}

DepotString FDepotResult::GetText(DepotResultChannel channel) const
{
	DepotString outText;
	for (const DepotResultText& text : m_TextList)
	{
		if (int(text->m_Channel) & int(channel))
		{
			if (outText.empty() == false)
				outText += "\n";
			outText += text->m_Value;
		}
	}
	return outText;
}

const Array<DepotResultTag>& FDepotResult::TagList() const
{
	return m_TagList;
}

const Array<DepotResultText>& FDepotResult::TextList() const
{
	return m_TextList;
}

const DepotString& FDepotResult::GetTagValue(const DepotString& tagKey) const
{
	if (tagKey.empty() == false)
	{
		for (const DepotResultTag& tag : m_TagList)
		{
			if (const DepotString* tagValuePtr = tag->GetValuePtr(tagKey))
				return *tagValuePtr;
		}
	}
	return StringInfo::EmptyA();
}

}}}
