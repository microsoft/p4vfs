// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotResult.h"
#include "DepotConfig.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	struct FDepotResultInfoField
	{
		struct Name
		{
			static constexpr const char* UserName			= "userName";
			static constexpr const char* ClientName			= "clientName";
			static constexpr const char* ClientRoot			= "clientRoot";
			static constexpr const char* ClientLock			= "clientLock";
			static constexpr const char* ClientCwd			= "clientCwd";
			static constexpr const char* ClientHost			= "clientHost";
			static constexpr const char* PeerAddress		= "peerAddress";
			static constexpr const char* ClientAddress		= "clientAddress";
			static constexpr const char* ServerName			= "serverName";
			static constexpr const char* ServerAddress		= "serverAddress";
			static constexpr const char* ServerRoot			= "serverRoot";
			static constexpr const char* ServerDate			= "serverDate";
			static constexpr const char* ServerUptime		= "serverUptime";
			static constexpr const char* ServerVersion		= "serverVersion";
			static constexpr const char* ServerServices		= "serverServices";
			static constexpr const char* ServerLicense		= "serverLicense";
			static constexpr const char* CaseHandling		= "caseHandling";
			static constexpr const char* BrokerAddress		= "brokerAddress";
			static constexpr const char* BrokerVersion		= "brokerVersion";
		};
	};

	struct FDepotResultInfoNode : FDepotResultNode
	{
		const DepotString& UserName()			{ return GetTagValue(FDepotResultInfoField::Name::UserName); }
		const DepotString& ClientName()			{ return GetTagValue(FDepotResultInfoField::Name::ClientName); }
		const DepotString& ClientRoot()			{ return GetTagValue(FDepotResultInfoField::Name::ClientRoot); }
		const DepotString& ClientLock()			{ return GetTagValue(FDepotResultInfoField::Name::ClientLock); }
		const DepotString& ClientCwd()			{ return GetTagValue(FDepotResultInfoField::Name::ClientCwd); }
		const DepotString& ClientHost()			{ return GetTagValue(FDepotResultInfoField::Name::ClientHost); }
		const DepotString& PeerAddress()		{ return GetTagValue(FDepotResultInfoField::Name::PeerAddress); }
		const DepotString& ClientAddress()		{ return GetTagValue(FDepotResultInfoField::Name::ClientAddress); }
		const DepotString& ServerName()			{ return GetTagValue(FDepotResultInfoField::Name::ServerName); }
		const DepotString& ServerAddress()		{ return GetTagValue(FDepotResultInfoField::Name::ServerAddress); }
		const DepotString& ServerRoot()			{ return GetTagValue(FDepotResultInfoField::Name::ServerRoot); }
		const DepotString& ServerDate()			{ return GetTagValue(FDepotResultInfoField::Name::ServerDate); }
		const DepotString& ServerUptime()		{ return GetTagValue(FDepotResultInfoField::Name::ServerUptime); }
		const DepotString& ServerVersion()		{ return GetTagValue(FDepotResultInfoField::Name::ServerVersion); }
		const DepotString& ServerServices()		{ return GetTagValue(FDepotResultInfoField::Name::ServerServices); }
		const DepotString& ServerLicense()		{ return GetTagValue(FDepotResultInfoField::Name::ServerLicense); }
		const DepotString& CaseHandling()		{ return GetTagValue(FDepotResultInfoField::Name::CaseHandling); }
		const DepotString& BrokerAddress()		{ return GetTagValue(FDepotResultInfoField::Name::BrokerAddress); }
		const DepotString& BrokerVersion()		{ return GetTagValue(FDepotResultInfoField::Name::BrokerVersion); }
	};

	typedef FDepotResultNodeProvider<FDepotResultInfoNode> FDepotResultInfo;
	typedef std::shared_ptr<FDepotResultInfo> DepotResultInfo;

}}}

#pragma managed(pop)
