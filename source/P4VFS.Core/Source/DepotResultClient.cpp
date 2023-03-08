// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotResultClient.h"
#include "DepotClient.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

DepotString FDepotResultClientOption::ToString(FDepotResultClientOption::Enum value)
{
	DepotString text;
	text += value & FDepotResultClientOption::AllWrite ? FDepotResultClientOption::Name::AllWrite : FDepotResultClientOption::Name::NoAllWrite;
	text += " ";
	text += value & FDepotResultClientOption::Clobber ? FDepotResultClientOption::Name::Clobber : FDepotResultClientOption::Name::NoClobber;
	text += " ";
	text += value & FDepotResultClientOption::Compress ? FDepotResultClientOption::Name::Compress : FDepotResultClientOption::Name::NoCompress;
	text += " ";
	text += value & FDepotResultClientOption::Locked ? FDepotResultClientOption::Name::Locked : FDepotResultClientOption::Name::UnLocked;
	text += " ";
	text += value & FDepotResultClientOption::ModTime ? FDepotResultClientOption::Name::ModTime : FDepotResultClientOption::Name::NoModTime;
	text += " ";
	text += value & FDepotResultClientOption::RmDir ? FDepotResultClientOption::Name::RmDir : FDepotResultClientOption::Name::NoRmDir;
	return text;
}

FDepotResultClientOption::Enum FDepotResultClientOption::FromString(const DepotString& text)
{
	FDepotResultClientOption::Enum value = FDepotResultClientOption::None;

	if (StringInfo::Contains(text.c_str(), FDepotResultClientOption::Name::NoAllWrite) == false)
		value |= FDepotResultClientOption::AllWrite;
	if (StringInfo::Contains(text.c_str(), FDepotResultClientOption::Name::NoClobber) == false)
		value |= FDepotResultClientOption::Clobber;
	if (StringInfo::Contains(text.c_str(), FDepotResultClientOption::Name::NoCompress) == false)
		value |= FDepotResultClientOption::Compress;
	if (StringInfo::Contains(text.c_str(), FDepotResultClientOption::Name::UnLocked) == false)
		value |= FDepotResultClientOption::Locked;
	if (StringInfo::Contains(text.c_str(), FDepotResultClientOption::Name::NoModTime) == false)
		value |= FDepotResultClientOption::ModTime;
	if (StringInfo::Contains(text.c_str(), FDepotResultClientOption::Name::NoRmDir) == false)
		value |= FDepotResultClientOption::RmDir;

	return value;
}

DepotConfig FDepotResultClient::Config() const
{
	DepotConfig config;
	config.m_Host = Host();
	config.m_Client = Client();
	return config;
}

time_t FDepotResultClient::UpdateTime() const
{
	return DepotInfo::StringToTime(Update());
}

time_t FDepotResultClient::AccessTime() const
{
	return DepotInfo::StringToTime(Access());
}

}}}

