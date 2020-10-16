#include "SimpleThreadProfiler.h"
static const float STProfiler_ReportIntervalSecs = 5.0f;

float STProfiler::s_fFrequency = 0.f;
INT64 STProfiler::s_iReportInterval = 0;

void STProfiler::Flush(INT64 end,  STLoggable* pLoggable)
{	
	if(pLoggable != nullptr)
	{
		if (STProfiler::s_iReportInterval == 0)
		{
			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);
			s_fFrequency = 1.0f / freq.QuadPart;
			MemoryBarrier();
			STProfiler::s_iReportInterval = (INT64)(freq.QuadPart * STProfiler_ReportIntervalSecs);

		}

		if (m_ThreadInfo->m_iLastReportTime == 0)
		{
			m_ThreadInfo->m_iLastReportTime = m_iStart;
			return;
		}
		float interval = (end - m_ThreadInfo->m_iLastReportTime) * s_fFrequency;
		float measured = m_ThreadInfo->m_iAccumulator * s_fFrequency;
		char buff[512];


		snprintf(buff, sizeof(buff), " TID 0x%x time spent in %s : %.0f/%.0f ms %.1f% % %d times \n",			
			GetCurrentThreadId(),
			m_ThreadInfo->m_name.c_str(),
			measured * 1000,
			interval * 1000,
			100.f * measured / interval,
			m_ThreadInfo->m_iHitCount
		);
		std::string logicalCoreAccumulation;
		logicalCoreAccumulation = "------------------------------------------";
		char coreBuff[64];
		snprintf(coreBuff, sizeof(coreBuff), "\nTID 0x%x core Information", GetCurrentThreadId());
		logicalCoreAccumulation += std::string(coreBuff);
		for (int i = 0; i < m_ThreadInfo->m_iLogicalCoreCount; ++i)
		{
			char coreBuff2[64];
			float measuredCore = m_ThreadInfo->m_vecThreadAccumulator[i] * s_fFrequency * 1000;
			snprintf(coreBuff2, sizeof(coreBuff2), "\n Core %d : Count %d : Time %.0f ms",
				i,
				m_ThreadInfo->m_vecThreadTally[i],
				measuredCore
			);
			logicalCoreAccumulation += std::string(coreBuff2);
		}
		logicalCoreAccumulation += "\n------------------------------------------";
		pLoggable->write(std::string(buff) + logicalCoreAccumulation);		
		
		m_ThreadInfo->m_iLastReportTime = end;
		m_ThreadInfo->m_iAccumulator = 0;
		m_ThreadInfo->m_iHitCount = 0;
		m_ThreadInfo->ResetCoreInfo();
	}

}
