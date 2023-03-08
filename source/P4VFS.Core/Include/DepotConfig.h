// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotResult.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	struct DepotConfig
	{
		DepotString m_Host;
		DepotString m_Port;
		DepotString m_Client;
		DepotString m_User;
		DepotString m_Passwd;
		DepotString m_Ignore;
		DepotString m_Directory;

		DepotString PortName() const;
		DepotString PortNumber() const;
	
		void Reset();
		void Apply(const DepotConfig& config);
		
		bool SetHost(const char* src);
		bool SetPort(const char* src);
		bool SetClient(const char* src);
		bool SetUser(const char* src);
		bool SetDirectory(const char* src);

		DepotString ToCommandString() const;
		DepotString ToConnectionString() const;
	
		static bool SetNonEmpty(DepotString& dst, const char* src);
		static bool AppendNonEmpty(DepotString& dst, const char* src);
		static bool AppendNonEmpty(DepotString& dst, const char* src0, const char* src1);
	};

}}}

#pragma managed(pop)
