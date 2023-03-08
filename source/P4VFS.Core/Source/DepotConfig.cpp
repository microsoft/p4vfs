// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotConfig.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

DepotString DepotConfig::PortName() const
{
	DepotStringArray tokens = StringInfo::Split(m_Port.c_str(), ":", StringInfo::SplitFlags::RemoveEmptyEntries);
	return tokens.size() > 0 ? tokens[0] : "";
}

DepotString DepotConfig::PortNumber() const
{
	DepotStringArray tokens = StringInfo::Split(m_Port.c_str(), ":", StringInfo::SplitFlags::RemoveEmptyEntries);
	return tokens.size() > 1 ? tokens[1] : "";
}

void DepotConfig::Apply(const DepotConfig& config)
{
	SetNonEmpty(m_Host, config.m_Host.c_str());
	SetNonEmpty(m_Port, config.m_Port.c_str());
	SetNonEmpty(m_Client, config.m_Client.c_str());
	SetNonEmpty(m_User, config.m_User.c_str());
	SetNonEmpty(m_Passwd, config.m_Passwd.c_str());
	SetNonEmpty(m_Ignore, config.m_Ignore.c_str());
	SetNonEmpty(m_Directory, config.m_Directory.c_str());
}

bool DepotConfig::SetHost(const char* src)
{
	return SetNonEmpty(m_Host, src);
}

bool DepotConfig::SetPort(const char* src)
{
	return SetNonEmpty(m_Port, src);
}

bool DepotConfig::SetClient(const char* src)
{
	return SetNonEmpty(m_Client, src);
}

bool DepotConfig::SetUser(const char* src)
{
	return SetNonEmpty(m_User, src);
}

bool DepotConfig::SetDirectory(const char* src)
{
	return SetNonEmpty(m_Directory, src);
}

DepotString DepotConfig::ToCommandString() const
{
	DepotString result;
	AppendNonEmpty(result, "-H", m_Host.c_str());
	AppendNonEmpty(result, "-p", m_Port.c_str());
	AppendNonEmpty(result, "-c", m_Client.c_str());
	AppendNonEmpty(result, "-u", m_User.c_str());
	AppendNonEmpty(result, "-P", m_Passwd.c_str());
	return result;
}

DepotString DepotConfig::ToConnectionString() const
{
	DepotString result;
	AppendNonEmpty(result, m_Port.c_str());
	AppendNonEmpty(result, m_Client.c_str());
	AppendNonEmpty(result, m_User.c_str());
	return result;
}

void DepotConfig::Reset()
{
	m_Host.clear();
	m_Port.clear();
	m_Client.clear();
	m_User.clear();
	m_Passwd.clear();
	m_Ignore.clear();
	m_Directory.clear();
}

bool DepotConfig::SetNonEmpty(DepotString& dst, const char* src)
{
	if (StringInfo::IsNullOrEmpty(src) == false)
	{
		dst = src;
		return true;
	}
	return false;
}

bool DepotConfig::AppendNonEmpty(DepotString& dst, const char* src)
{
	if (StringInfo::IsNullOrEmpty(src) == false)
	{
		if (dst.empty() == false)
			dst += " ";
		dst += src;
		return true;
	}
	return false;
}

bool DepotConfig::AppendNonEmpty(DepotString& dst, const char* src0, const char* src1)
{
	if (StringInfo::IsNullOrEmpty(src0) == false && StringInfo::IsNullOrEmpty(src1) == false)
	{
		if (dst.empty() == false)
			dst += " ";
		dst += src0;
		dst += " ";
		dst += src1;
		return true;
	}
	return false;
}

}}}

