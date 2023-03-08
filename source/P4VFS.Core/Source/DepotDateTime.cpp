// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "Pch.h"
#include "DepotDateTime.h"
#include "DepotClient.h"

namespace Microsoft {
namespace P4VFS {
namespace P4 {

DepotDateTime::DepotDateTime(time_t t) :
	m_Time(t)
{ 
}

time_t DepotDateTime::Seconds() const
{
	return m_Time;
}

time_t DepotDateTime::Time() const
{
	return m_Time;
}

DepotString DepotDateTime::ToString() const
{
	return DepotInfo::TimeToString(m_Time);
}

DepotString DepotDateTime::ToDisplayString() const
{
	return StringInfo::FormatLocalTime(m_Time, "%m/%d/%Y-%H:%M:%S");
}

DepotDateTime DepotDateTime::FromString(const DepotString& text)
{
	return DepotDateTime(DepotInfo::StringToTime(text));
}

DepotDateTime DepotDateTime::Now()
{
	return DepotDateTime(TimeInfo::GetTime());
}

DepotTimer::DepotTimer()
{
	Reset();
}

void DepotTimer::Reset() 
{ 
	m_Start = DepotDateTime::Now();
}

time_t DepotTimer::TotalSeconds() const
{
	return DepotDateTime::Now().Seconds() - m_Start.Seconds();
}

DepotStopwatch::DepotStopwatch(Init::Enum state)
{ 
	Reset(state); 
}

void DepotStopwatch::Reset(Init::Enum state)
{
	m_Span.QuadPart = 0;
	m_Start.QuadPart = 0;
	QueryPerformanceFrequency(&m_Freq);
	m_Running = false; 
	if (state == Init::Start)
	{
		Start();
	}
}

void DepotStopwatch::Restart()
{
	Reset(Init::Start);
}

void DepotStopwatch::Start()
{
	QueryPerformanceCounter(&m_Start);
	m_Running = true;
}

void DepotStopwatch::Stop()
{
	if (m_Running)
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		m_Span.QuadPart += now.QuadPart - m_Start.QuadPart;
		m_Running = false;
	}
}

int64_t DepotStopwatch::TotalSeconds() const
{
	return TotalMicroseconds() / 1000000;
}

int64_t DepotStopwatch::TotalMilliseconds() const
{
	return TotalMicroseconds() / 1000;
}

int64_t DepotStopwatch::TotalMicroseconds() const
{
	LARGE_INTEGER span = m_Span;
	if (m_Running) 
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		span.QuadPart += now.QuadPart - m_Start.QuadPart;
	}
	double totalSeconds = double(span.QuadPart) / double(m_Freq.QuadPart);
	return int64_t(totalSeconds * 1000000.0);
}

}}}


