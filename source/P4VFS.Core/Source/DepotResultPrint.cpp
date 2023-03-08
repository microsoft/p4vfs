// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotResultPrint.h"
#include "DepotClient.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

DepotResultReply FDepotResultPrintCharset::OnStreamStat(IDepotClientCommand* cmd, const DepotResultTag& tag)
{
	cmd->SetOutputEncoding(Type());
	return DepotResultReply::Handled;
}

FDepotResultPrintFile::FDepotResultPrintFile(FILE* file) :
	m_File(file)
{
}

DepotResultReply FDepotResultPrintFile::OnStreamOutput(IDepotClientCommand* cmd, const char* data, size_t length)
{
	if (data != nullptr && length > 0 && m_File != nullptr)
	{
		if (fwrite(data, 1, length, m_File) != length)
		{
			m_TextList.push_back(std::make_shared<FDepotResultText>(FDepotResultText{ DepotResultChannel::StdErr, "Failed to write data to stream" }));
			m_File = nullptr;
		}
	}
	return DepotResultReply::Handled;
}

FDepotResultPrintHandle::FDepotResultPrintHandle(HANDLE hStream) :
	m_hStream(hStream)
{
}

DepotResultReply FDepotResultPrintHandle::OnStreamOutput(IDepotClientCommand* cmd, const char* data, size_t length)
{
	if (data != nullptr && length > 0 && m_hStream != INVALID_HANDLE_VALUE)
	{
		DWORD dwWritten = 0;
		if (WriteFile(m_hStream, data, DWORD(length), &dwWritten, NULL) == FALSE || dwWritten != DWORD(length))
		{
			m_TextList.push_back(std::make_shared<FDepotResultText>(FDepotResultText{ DepotResultChannel::StdErr, "Failed to write data to stream" }));
			m_hStream = INVALID_HANDLE_VALUE;
		}
	}
	return DepotResultReply::Handled;
}

const DepotString& FDepotResultPrintString::GetString() const
{
	return m_Text;
}

DepotResultReply FDepotResultPrintString::OnStreamOutput(IDepotClientCommand* cmd, const char* data, size_t length)
{
	if (data != nullptr && length > 0)
		m_Text.append(data, length);

	return DepotResultReply::Handled;
}

}}}

