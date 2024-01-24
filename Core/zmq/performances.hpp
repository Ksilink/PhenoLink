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


    Perfs(): totalPhysMem(0), physMemUsed(0), procPhysMem(0),
        total_cpu_load(0), proc_cpu_load(0)
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


#include "sys/types.h"
#include "sys/sysinfo.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"


// FIXME : Do it for linux also !
static struct Perfs
{

    uint64_t totalPhysMem, physMemUsed, procPhysMem;
    double total_cpu_load, proc_cpu_load;


    Perfs(): totalPhysMem(0), physMemUsed(0), procPhysMem(0),
        total_cpu_load(0), proc_cpu_load(0)
    {
        FILE* file = fopen("/proc/stat", "r");
        fscanf(file, "cpu %llu %llu %llu %llu", &lastTotalUser, &lastTotalUserLow,
               &lastTotalSys, &lastTotalIdle);
        fclose(file);



        struct tms timeSample;
        char line[128];

        lastCPU = times(&timeSample);
        lastSysCPU = timeSample.tms_stime;
        lastUserCPU = timeSample.tms_utime;

        file = fopen("/proc/cpuinfo", "r");
        numProcessors = 0;
        while(fgets(line, 128, file) != NULL){
            if (strncmp(line, "processor", 9) == 0) numProcessors++;
        }
        fclose(file);


    }


    void refresh()
    {

        refreshCPU();
        refresh_meminfo();
    }


protected:


    unsigned long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;

    clock_t lastCPU, lastSysCPU, lastUserCPU;
    int numProcessors;



    int getMemValue(){ //Note: this value is in KB!
        FILE* file = fopen("/proc/self/status", "r");
        int result = -1;
        char line[128];

        while (fgets(line, 128, file) != NULL){
            if (strncmp(line, "VmRSS:", 6) == 0){
                result = parseLine(line);
                break;
            }
        }
        fclose(file);
        return result;
    }

    void refresh_meminfo()
    {
        struct sysinfo memInfo;

        sysinfo (&memInfo);


        totalPhysMem = memInfo.totalram;
        //Multiply in next statement to avoid int overflow on right hand side...
        totalPhysMem *= memInfo.mem_unit;


        physMemUsed = memInfo.totalram - memInfo.freeram;
        //Multiply in next statement to avoid int overflow on right hand side...
        physMemUsed *= memInfo.mem_unit;

        procPhysMem = getMemValue() * 1024:
    }

    void refreshCPU()
    {
        double percent;
        FILE* file;
        unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;

        file = fopen("/proc/stat", "r");
        fscanf(file, "cpu %llu %llu %llu %llu", &totalUser, &totalUserLow,
               &totalSys, &totalIdle);
        fclose(file);

        if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow ||
            totalSys < lastTotalSys || totalIdle < lastTotalIdle){
            //Overflow detection. Just skip this value.
            percent = -1.0;
        }
        else{
            total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) +
                    (totalSys - lastTotalSys);
            percent = total;
            total += (totalIdle - lastTotalIdle);
            percent /= total;
            percent *= 100;
        }

        lastTotalUser = totalUser;
        lastTotalUserLow = totalUserLow;
        lastTotalSys = totalSys;
        lastTotalIdle = totalIdle;

        total_cpu_load = percent;


        struct tms timeSample;
        clock_t now;

        now = times(&timeSample);

        if (now <= lastCPU || timeSample.tms_stime < lastSysCPU ||
            timeSample.tms_utime < lastUserCPU){
            //Overflow detection. Just skip this value.
            percent = -1.0;
        }
        else{
            percent = (timeSample.tms_stime - lastSysCPU) +
                      (timeSample.tms_utime - lastUserCPU);
            percent /= (now - lastCPU);
            percent /= numProcessors;
            percent *= 100;
        }
        lastCPU = now;
        lastSysCPU = timeSample.tms_stime;
        lastUserCPU = timeSample.tms_utime;


        proc_cpu_load = percent;

    }


} perf_instance;




#endif

#endif // PERFORMANCES_HPP
