// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotResultInterop.h"
#include "DepotResult.h"
#include "CoreMarshal.h"

using namespace msclr::interop;
using namespace System::Runtime::InteropServices;
using namespace System::Collections::Generic;

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

DepotResultTag::DepotResultTag()
{
	m_Fields = gcnew FieldsType(System::StringComparer::InvariantCultureIgnoreCase);
}

DepotResultTag^ DepotResultTag::FromNative(const P4::DepotResultTag& src)
{
	DepotResultTag^ dst = nullptr;
	if (src.get() != nullptr)
	{
		dst = gcnew DepotResultTag();
		for (P4::FDepotResultTag::FieldsType::const_iterator i = src->m_Fields.begin(); i != src->m_Fields.end(); ++i)
		{
			dst->m_Fields->Add(gcnew System::String(i->first.c_str()), gcnew System::String(i->second.c_str()));
		}
	}
	return dst;
}

bool DepotResultTag::ContainsKey(System::String^ tagKey)
{
	return m_Fields->ContainsKey(tagKey);
}

void DepotResultTag::RemoveKey(System::String^ tagKey)
{
	m_Fields->Remove(tagKey);
}

void DepotResultTag::SetValue(System::String^ tagKey, System::String^ tagValue)
{
	if (System::String::IsNullOrEmpty(tagKey) == false)
	{
		m_Fields[tagKey] = tagValue;
	}
}

bool DepotResultTag::TryGetValue(System::String^ tagKey, [Out] System::String^% value)
{
	return m_Fields->TryGetValue(tagKey, value);
}

System::String^ DepotResultTag::GetValue(System::String^ tagKey)
{
	System::String^ value = nullptr;
	return m_Fields->TryGetValue(tagKey, value) ? value : nullptr;
}

System::Nullable<System::Int32>^ DepotResultTag::GetValueInt32(System::String^ tagKey)
{
	System::Int32 value = 0;
	return System::Int32::TryParse(GetValue(tagKey), value) ? gcnew System::Nullable<System::Int32>(value) : nullptr;
}

System::Nullable<System::Int64>^ DepotResultTag::GetValueInt64(System::String^ tagKey)
{
	System::Int64 value = 0;
	return System::Int64::TryParse(GetValue(tagKey), value) ? gcnew System::Nullable<System::Int64>(value) : nullptr;
}

DepotResultText::DepotResultText()
{
}

DepotResultText::DepotResultText(DepotResultChannel channel, System::String^ value, System::Int32 level)
{
	Channel = channel;
	Value = value;
	Level = level;
}

DepotResultText^ DepotResultText::FromNative(const P4::DepotResultText& src)
{
	DepotResultText^ dst = nullptr;
	if (src.get() != nullptr)
	{
		dst = gcnew DepotResultText();
		dst->Channel = safe_cast<DepotResultChannel>(src->m_Channel);
		dst->Value = gcnew System::String(src->m_Value.c_str());
		dst->Level = src->m_Level;
	}
	return dst;
}

DepotResult::DepotResult()
{
	m_TagList = gcnew TagListType();
	m_TextList = gcnew TextListType();
}

DepotResult^ DepotResult::FromNative(const P4::DepotResult& src)
{
	DepotResult^ dst = nullptr;
	if (src.get() != nullptr)
	{
		dst = gcnew DepotResult();
		for (const P4::DepotResultTag& srcTag : src->TagList())
		{
			dst->m_TagList->Add(DepotResultTag::FromNative(srcTag));
		}

		for (const P4::DepotResultText& srcText : src->TextList())
		{
			dst->TextList->Add(DepotResultText::FromNative(srcText));
		}
	}
	return dst;
}

bool DepotResult::HasError::get()
{
	return HasText(DepotResultChannel::StdErr);
}

void DepotResult::SetError(System::String^ errorText)
{
	m_TextList->Clear();
	m_TextList->Add(gcnew DepotResultText(DepotResultChannel::StdErr, errorText ? errorText : "", 0));
}

System::String^ DepotResult::GetError()
{
	return GetText(DepotResultChannel::StdErr);
}

bool DepotResult::HasText(DepotResultChannel channel)
{
	for each (DepotResultText^ text in m_TextList)
	{
		if (text != nullptr && text->Channel.HasFlag(channel))
		{
			return true;
		}
	}
	return false;
}

System::String^ DepotResult::GetText(DepotResultChannel channel)
{
	System::Text::StringBuilder^ outText = gcnew System::Text::StringBuilder();
	for each (DepotResultText^ text in m_TextList)
	{
		if (text != nullptr && text->Channel.HasFlag(channel))
		{
			if (outText->Length > 0)
			{
				outText->Append("\n");
			}
			outText->Append(text->Value);
		}
	}
	return outText->ToString();
}
	
System::String^ DepotResult::GetTagValue(System::String^ tagKey)
{
	if (System::String::IsNullOrEmpty(tagKey) == false)
	{
		for each (DepotResultTag^ tag in m_TagList)
		{
			System::String^ value = nullptr;
			if (tag != nullptr && tag->TryGetValue(tagKey, value))
			{
				return value;
			}
		}
	}
	return nullptr;
}

}}}

