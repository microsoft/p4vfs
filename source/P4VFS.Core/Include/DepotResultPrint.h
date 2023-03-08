// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotResult.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	typedef std::shared_ptr<struct FDepotResultPrintFile> DepotResultPrintFile;
	typedef std::shared_ptr<struct FDepotResultPrintHandle> DepotResultPrintHandle;
	typedef std::shared_ptr<struct FDepotResultPrintString> DepotResultPrintString;

	struct FDepotResultPrintCharsetField
	{
		struct Name
		{
			static constexpr const char* Action		= "Action";
			static constexpr const char* Change		= "Change";
			static constexpr const char* DepotFile	= "DepotFile";
			static constexpr const char* FileSize	= "FileSize";
			static constexpr const char* Rev		= "Rev";
			static constexpr const char* Time		= "Time";
			static constexpr const char* Type		= "Type";
		};
	};

	struct FDepotResultPrintCharset : FDepotResult
	{
		const DepotString& Action() const		{ return GetTagValue(FDepotResultPrintCharsetField::Name::Action); }
		const DepotString& Change() const		{ return GetTagValue(FDepotResultPrintCharsetField::Name::Change); }
		const DepotString& DepotFile() const	{ return GetTagValue(FDepotResultPrintCharsetField::Name::DepotFile); }
		const DepotString& FileSize() const		{ return GetTagValue(FDepotResultPrintCharsetField::Name::FileSize); }
		const DepotString& Rev() const			{ return GetTagValue(FDepotResultPrintCharsetField::Name::Rev); }
		const DepotString& Time() const			{ return GetTagValue(FDepotResultPrintCharsetField::Name::Time); }
		const DepotString& Type() const			{ return GetTagValue(FDepotResultPrintCharsetField::Name::Type); }

		virtual DepotResultReply OnStreamStat(IDepotClientCommand* cmd, const DepotResultTag& tag) override;
	};

	struct FDepotResultPrintFile : FDepotResultPrintCharset
	{
		FDepotResultPrintFile(FILE* file);
		virtual DepotResultReply OnStreamOutput(IDepotClientCommand* cmd, const char* data, size_t length) override;

	private:
		FILE* m_File;
	};

	struct FDepotResultPrintHandle : FDepotResultPrintCharset
	{
		FDepotResultPrintHandle(HANDLE hStream);
		virtual DepotResultReply OnStreamOutput(IDepotClientCommand* cmd, const char* data, size_t length) override;

	private:
		HANDLE m_hStream;
	};

	struct FDepotResultPrintString : FDepotResultPrintCharset
	{
		const DepotString& GetString() const;
		virtual DepotResultReply OnStreamOutput(IDepotClientCommand* cmd, const char* data, size_t length) override;

	private:
		DepotString m_Text;
	};

}}}

#pragma managed(pop)
