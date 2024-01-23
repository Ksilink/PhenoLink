#ifndef PERFORMANCES_HPP
#define PERFORMANCES_HPP

#include <utility>



// https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process


#ifdef WIN32

#undef _WIN32_WINNT
#define _WIN32_WINNT  0x0600


#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include <psapi.h>
#include <pdh.h>
#include <sysinfoapi.h>




static struct Perfs
{

    uint64_t totalPhysMem, physMemUsed, procPhysMem;
    double total_cpu_load, proc_cpu_load;


    Perfs()
    {
        SYSTEM_INFO sysInfo;
        FILETIME ftime, fsys, fuser;

        GetSystemInfo(&sysInfo);
        numProcessors = sysInfo.dwNumberOfProcessors;

        GetSystemTimeAsFileTime(&ftime);
        memcpy(&lastCPU, &ftime, sizeof(FILETIME));

        self = GetCurrentProcess();
        GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
        memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
        memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));


        PdhOpenQuery(NULL, NULL, &cpuQuery);
        // You can also use L"\\Processor(*)\\% Processor Time" and get individual CPU values with PdhGetFormattedCounterArray()
        PdhAddEnglishCounterW(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
        PdhCollectQueryData(cpuQuery);
    }


    void refresh()
    {

        refreshCPU();
        refresh_meminfo();
    }


protected:
     PDH_HQUERY cpuQuery;
     PDH_HCOUNTER cpuTotal;

     ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
     int numProcessors;
     HANDLE self;


     void refresh_meminfo()
     {
        MEMORYSTATUSEX memInfo;

        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);



        PROCESS_MEMORY_COUNTERS_EX pmc;
        GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));


        // The total physical mem

        totalPhysMem = memInfo.ullTotalPhys;

        physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;

        procPhysMem  = pmc.WorkingSetSize;

     }


     void refreshCPU()
     {
        PDH_FMT_COUNTERVALUE counterVal;

        PdhCollectQueryData(cpuQuery);
        PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);

        total_cpu_load = counterVal.doubleValue;

        FILETIME ftime, fsys, fuser;
        ULARGE_INTEGER now, sys, user;
        double percent;

        GetSystemTimeAsFileTime(&ftime);
        memcpy(&now, &ftime, sizeof(FILETIME));

        GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
        memcpy(&sys, &fsys, sizeof(FILETIME));
        memcpy(&user, &fuser, sizeof(FILETIME));
        percent = (sys.QuadPart - lastSysCPU.QuadPart) +
                  (user.QuadPart - lastUserCPU.QuadPart);
        percent /= (now.QuadPart - lastCPU.QuadPart);
        percent /= numProcessors;
        lastCPU = now;
        lastUserCPU = user;
        lastSysCPU = sys;



        proc_cpu_load = percent *100;
    }


} perf_instance;


#elif LINUX

// FIXME : Do it for linux also !
static struct Perfs
{

    uint64_t totalPhysMem, physMemUsed, procPhysMem;
    double total_cpu_load, proc_cpu_load;


    Perfs()
    {

    }

} perf_instance;




#endif

#endif // PERFORMANCES_HPP
