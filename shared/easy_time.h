#ifdef __APPLE__ 

#elif _WIN32
#include <timeapi.h>
static LARGE_INTEGER GlobalTimeFrequencyDatum;

inline s64 EasyTime_GetTimeCount()
{
    LARGE_INTEGER Time;
    QueryPerformanceCounter(&Time);
    
    return Time.QuadPart;
}

inline float EasyTime_GetSecondsElapsed(s64 CurrentCount, s64 LastCount)
{
    
    s64 Difference = CurrentCount - LastCount;
    float Seconds = (float)Difference / (float)GlobalTimeFrequencyDatum.QuadPart;
    
    return Seconds;
    
}

inline void EasyTime_setupTimeDatums() {
	QueryPerformanceFrequency(&GlobalTimeFrequencyDatum);
	timeBeginPeriod(1);
}
#endif