// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "CoreInterop.h"
#include "DepotClient.h"
#include "DepotConfigInterop.h"
#include "DepotResultInterop.h"

namespace Microsoft {
namespace P4VFS {
namespace CoreInterop {

[System::FlagsAttribute]
public enum class DepotCommandFlags : System::Int32
{
	None		= P4::DepotCommand::Flags::None,
	UnTagged	= P4::DepotCommand::Flags::UnTagged,
};

public ref class DepotCommand
{
public:
	DepotCommand(
		);

	P4::DepotCommand 
	ToNative(
		);

	System::String^ Name;
	array<System::String^>^ Args;
	System::String^ Input;
	System::Func<System::String^, System::String^>^ Prompt;
	DepotCommandFlags Flags;
};

public ref class DepotClient : public System::IDisposable
{
public:
	DepotClient(
		);
	
	DepotClient(
		FileContext^ fileContext
		);

	~DepotClient(
		);

	!DepotClient(
		);

	bool 
	Connect(
		DepotConfig^ config
		);

	void 
	Disconnect(
		);

	bool 
	IsConnected(
		);

	bool 
	IsLoginRequired(
		);

	bool 
	Login(
		System::Func<System::String^, System::String^>^ prompt
		);

	bool 
	Login(
		);

	bool 
	IsFaulted(
		);

	bool 
	HasError(
		);

	property bool 
	DisableLogin {
		bool get(); 
		void set(bool); 
		}

	property bool 
	Unattended { 
		bool get(); 
		void set(bool); 
		}

	property bool 
	Unimpersonated { 
		bool get(); 
		void set(bool); 
		}

	bool 
	SetEnv(
		System::String^ name, 
		System::String^ value
		);

	System::String^ 
	GetEnv(
		System::String^ name
		);

	System::String^ 
	GetErrorText(
		);

	DepotResult^ 
	Connection(
		);

	DepotConfig^ 
	Config(
		);

	DepotResult^ 
	Run(
		DepotCommand^ cmd
		);

	P4::DepotClient 
	ToNative(
		);

private:
	void 
	ThrowIfObjectDisposed(
		);

private:
	struct FDepotClientData* m_Data;
};

public ref struct DepotInfo
{
	static System::Int64 
	StringToTime(
		System::String^ text
		);

	static System::String^ 
	TimeToString(
		System::Int64 time
		);

	static bool 
	IsWritableFileType(
		System::String^ fileType
		);

	static bool 
	IsSymlinkFileType(
		System::String^ fileType
		);

	static DepotConfig^ 
	DepotConfigFromPath(
		System::String^ folderPath
		);
};

public ref struct DepotTunable
{
	static System::Int32 
	Get(
		System::String^ name
		);

	static System::Int32 
	Get(
		System::String^ name, 
		System::Int32 defaultValue
		);

	static void	
	Set(
		System::String^ name, 
		System::Int32 value
		);

	static void	
	Unset(
		System::String^ name
		);

	static bool 
	IsSet(
		System::String^ name
		);

	static bool 
	IsKnown(
		System::String^ name
		);
};

}}}

