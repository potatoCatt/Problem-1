#include <windows.h>
#include <iostream>
#include <tchar.h>
#include <psapi.h>
#include <string>
#include <fstream>



static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;


void GetMemoryInfo(HANDLE hProcess, SIZE_T& WorkingSetSize, SIZE_T& PrivateBytes)
{
    
    PROCESS_MEMORY_COUNTERS_EX pmc;


    if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
    {
        WorkingSetSize= pmc.WorkingSetSize;
        PrivateBytes =pmc.PrivateUsage;

    }

}


void init(HANDLE hProcess) {
    SYSTEM_INFO sysInfo;
    FILETIME fcreate, ftime, fsys, fuser;

    GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&lastCPU, &ftime, sizeof(FILETIME));

    GetProcessTimes(hProcess, &fcreate, &ftime, &fsys, &fuser);
    memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
    memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));
}

double getCurrentValue(HANDLE hProcess) {
    FILETIME ftime, fsys, fuser, fcreate;
    ULARGE_INTEGER now, sys, user;
    double percent;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&now, &ftime, sizeof(FILETIME));

    GetProcessTimes(hProcess, &fcreate, &ftime, &fsys, &fuser);
    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));
    percent = (sys.QuadPart - lastSysCPU.QuadPart) +
        (user.QuadPart - lastUserCPU.QuadPart);
    percent /= (now.QuadPart - lastCPU.QuadPart);
    percent /= numProcessors;
    lastCPU = now;
    lastUserCPU = user;
    lastSysCPU = sys;

    return percent * 100;
}


int _tmain(int argc, LPTSTR argv[])
{

   STARTUPINFOW si = { 0 };
   si.cb = sizeof(si);
   PROCESS_INFORMATION pi = { 0 };
   std::cout << "Enter the full path to the process: " << std::endl;
   std::string strPath;
   std::getline(std::cin, strPath);
   std::wstring wideStrPath = std::wstring(strPath.begin(), strPath.end());
   const wchar_t* Path = wideStrPath.c_str();
   SIZE_T TimeInterval;
   std::cout << "Enter the time interval between data collection iterations in seconds: " << std::endl;
   std::cin >> TimeInterval;
   BOOL success = CreateProcessW(
      Path,  
      NULL,                                  
      NULL,                                   
      NULL,                                 
      FALSE,                                 
      0,                                   
      NULL,                                   
      NULL,                                
      &si,                                   
      &pi);                                   

   if (success)
   {
       DWORD HandleCount{0};
       PDWORD pdwHandleCount = &HandleCount;
       double CPU;
       SIZE_T WorkingSet;
       SIZE_T PrivateBytes;
       init(pi.hProcess);
       std::ofstream myfile;
       myfile.open("log.csv");
       myfile << "CPU,Working Set,Private Bytes,Number of Handles\n";
       int counter = 0;
       DWORD exitCode=STILL_ACTIVE;
       while (exitCode== STILL_ACTIVE) {
           GetExitCodeProcess(pi.hProcess, &exitCode);
           BOOL successCount = GetProcessHandleCount(pi.hProcess, pdwHandleCount);
           GetMemoryInfo(pi.hProcess, WorkingSet, PrivateBytes);
           CPU = getCurrentValue(pi.hProcess);
           myfile <<CPU<<","<<WorkingSet<<","<<PrivateBytes<<","<< HandleCount<< "\n";
           Sleep(TimeInterval*1000);
           counter++;
       }
       myfile.close();
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
   }
   else
   {
       std::cout << "Error " <<GetLastError()<< std::endl;
   }
   return 0;
}