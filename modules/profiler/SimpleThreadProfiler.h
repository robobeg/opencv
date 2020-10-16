#pragma once
#ifndef _CPP_STProfiler_H_
#define _CPP_STProfiler_H_
#include <Windows.h>
#include <vector>
#include <thread>
#include <string>
interface STLoggable
{
public:
	virtual void write(const std::string& log) = 0;
};

class DebugLogger: public STLoggable
{
	virtual void write(const std::string& log)
    {
#ifdef UNICODE
        std::wstring temp;
        temp.assign(log.begin(), log.end());
        OutputDebugString(temp.c_str());
#else //UNICODE
        OutputDebugString(log.c_str());
#endif //UNICODE
	}
};

class STProfiler
{
public:
	struct ThreadInfo
	{
		INT64 m_iLastReportTime;
		INT64 m_iAccumulator;
		INT64 m_iHitCount;
		std::string m_name;
		int m_iLogicalCoreCount;

		std::vector<int> m_vecThreadTally;
		std::vector<INT64> m_vecThreadAccumulator;

		ThreadInfo():
			m_iLastReportTime(0),
			m_iAccumulator(0),
			m_iHitCount(0),
			m_name("invalid"),
			m_iLogicalCoreCount(std::thread::hardware_concurrency()),
			m_vecThreadTally(std::thread::hardware_concurrency(), 0),
			m_vecThreadAccumulator(std::thread::hardware_concurrency(),0)

		{
			
		}

		ThreadInfo(INT64 iLastReportTime, INT64 iAccumulator, INT64 iHitCount, std::string name):
			m_iLastReportTime(iLastReportTime),
			m_iAccumulator(iAccumulator),
			m_iHitCount(iHitCount),
			m_name(name),
			m_iLogicalCoreCount(std::thread::hardware_concurrency()),
			m_vecThreadTally(std::thread::hardware_concurrency(), 0),
			m_vecThreadAccumulator(std::thread::hardware_concurrency(), 0)
		{

		}


		void AccumulateCoreInfo(DWORD threadNumber, INT64& iAccumulateBy)
		{
			int iThreadNumber = static_cast<int>(threadNumber);
			if( iThreadNumber < m_vecThreadTally.size() && iThreadNumber>= 0)
			{
				m_vecThreadTally[iThreadNumber] += 1;
			}
			if (iThreadNumber < m_vecThreadAccumulator.size() && iThreadNumber >= 0)
			{
				m_vecThreadAccumulator[iThreadNumber] += iAccumulateBy;
			}
		}

		void ResetCoreInfo()
		{
			for (int i = 0; i < m_vecThreadTally.size(); ++i)
			{
				m_vecThreadTally[i] = 0;
			}

			for (int i = 0; i < m_vecThreadAccumulator.size(); ++i)
			{
				m_vecThreadAccumulator[i] = 0;
			}
		}

	};
	INT64 m_iStart;
	ThreadInfo* m_ThreadInfo;
	
	static float s_fFrequency;
	static INT64 s_iReportInterval;
	void Flush(INT64 end, STLoggable* pLoggable);
	STLoggable* m_pLoggable;
public:
	__forceinline STProfiler(ThreadInfo* threadInfo, STLoggable* pLoggable) : m_pLoggable(pLoggable)
	{
		LARGE_INTEGER start;
		QueryPerformanceCounter(&start);
		m_iStart = start.QuadPart;
		m_ThreadInfo = threadInfo;
	}

	__forceinline ~STProfiler()
	{
		LARGE_INTEGER end;
		QueryPerformanceCounter(&end);
		INT64 iAccumulateBy = end.QuadPart - m_iStart;
		m_ThreadInfo->m_iAccumulator += iAccumulateBy;
		m_ThreadInfo->AccumulateCoreInfo(GetCurrentProcessorNumber(), iAccumulateBy);
		m_ThreadInfo->m_iHitCount++;
		if (end.QuadPart - m_ThreadInfo->m_iLastReportTime > STProfiler::s_iReportInterval)
		{
			Flush(end.QuadPart, m_pLoggable);
		}
	}
};

#define DECLARE_ST_PROFILER(name) \
	__declspec(thread) STProfiler::ThreadInfo __STProfiler_##name(0,0,0, #name);

#define DEFINE_ST_PROFILER(name) \
	__declspec(thread) STProfiler::ThreadInfo __STProfiler_##name(0,0,0, #name);

#define TOKENPASTE2(x, y) x ## y 
#define TOKENPASTE(x,y) TOKENPASTE2(x,y)
#define ST_PROFILER(name, pLoggable) \
	STProfiler TOKENPASTE(__STProfiler_##name, __LINE__)(&__STProfiler_##name, pLoggable)

#endif //_CPP_STProfiler_H_
