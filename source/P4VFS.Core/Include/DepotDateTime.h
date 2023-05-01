// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "DepotResult.h"
#pragma managed(push, off)

namespace Microsoft {
namespace P4VFS {
namespace P4 {

	class DepotDateTime
	{
	public:
		DepotDateTime(time_t t = 0);

		time_t Seconds() const;
		time_t Time() const;
		DepotString ToString() const;
		DepotString ToDisplayString() const;

		static DepotDateTime FromString(const DepotString& text);
		static DepotDateTime Now();

	private:
		time_t m_Time;
	};

	class DepotTimer
	{
	public:
		DepotTimer();

		void Reset();
		time_t TotalSeconds() const;

	private:
		DepotDateTime m_Start;
	};

	class DepotStopwatch 
	{
	public:
		struct Init { enum Enum
		{
			Stop,
			Start,
		};};

		DepotStopwatch(Init::Enum state = Init::Stop);

		void Reset(Init::Enum state = Init::Stop);
		void Restart();
		void Start();
		void Stop();

		int64_t TotalSeconds() const { return static_cast<int64_t>(DurationSeconds()); }
		int64_t TotalMilliseconds() const { return static_cast<int64_t>(DurationMilliseconds()); }
		int64_t TotalMicroseconds() const { return static_cast<int64_t>(DurationMicroseconds()); }

		double DurationSeconds() const;
		double DurationMilliseconds() const;
		double DurationMicroseconds() const;

	private:
		LARGE_INTEGER m_Freq;
		LARGE_INTEGER m_Start;
		LARGE_INTEGER m_Span;
		bool m_Running;
	};

}}}

#pragma managed(pop)
