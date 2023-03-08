// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "CoreInterop.h"
#include "DepotResult.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

[System::FlagsAttribute]
public enum class DepotResultChannel : System::Int32
{
	None		= P4::DepotResultChannel::None,
	StdOut		= P4::DepotResultChannel::StdOut,
	StdErr		= P4::DepotResultChannel::StdErr,
};

public ref class DepotResultTag
{
public:
	DepotResultTag();
	static DepotResultTag^ FromNative(const P4::DepotResultTag& src);

	bool ContainsKey(System::String^ tagKey);
	void RemoveKey(System::String^ tagKey);
	void SetValue(System::String^ tagKey, System::String^ tagValue);

	bool TryGetValue(System::String^ tagKey, [System::Runtime::InteropServices::Out] System::String^% value);
	System::String^ GetValue(System::String^ tagKey);

	System::Nullable<System::Int32>^ GetValueInt32(System::String^ tagKey);
	System::Nullable<System::Int64>^ GetValueInt64(System::String^ tagKey);

	using FieldsType = System::Collections::Generic::SortedDictionary<System::String^, System::String^>;
	property FieldsType^ Fields { FieldsType^ get() { return m_Fields; } }

private:
	FieldsType^ m_Fields;
};

public ref class DepotResultText
{
public:
	DepotResultText();
	DepotResultText(DepotResultChannel channel, System::String^ value, System::Int32 level);
	static DepotResultText^ FromNative(const P4::DepotResultText& src);

	property DepotResultChannel Channel;
	property System::String^ Value;
	property System::Int32 Level;
};

public ref class DepotResult
{
public:
	DepotResult();
	static DepotResult^ FromNative(const P4::DepotResult& src);

	property bool HasError { bool get(); }
	void SetError(System::String^ errorText);
	System::String^ GetError();

	bool HasText(DepotResultChannel channel);
	System::String^ GetText(DepotResultChannel channel);
	
	System::String^ GetTagValue(System::String^ tagKey);

	using TagListType = System::Collections::Generic::List<DepotResultTag^>;
	property TagListType^ TagList { TagListType^ get() { return m_TagList; } }
	
	using TextListType = System::Collections::Generic::List<DepotResultText^>;
	property TextListType^ TextList { TextListType^ get() { return m_TextList; } }

protected:
	TagListType^ m_TagList;
	TextListType^ m_TextList;
};

}}}

