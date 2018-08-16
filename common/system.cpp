/*---------------------------------------------------------------------------------
* Copyright(C),2018
* FileName:  system.h实现
* Author:    luoyouchun
* Version:   1.0
* Date:      2018-8-111
* Description:  Windows 操作系统类型获取、关闭系统调用重定向、获取命令行输出结果
*---------------------------------------------------------------------------------
* Modification Histroy
Date    Author      Modification:
*
*---------------------------------------------------------------------------------*/


#include <stdio.h>
#include <windows.h>
#include "system.h"
#include <lm.h>
#include <VersionHelpers.h>
#include "utils.h"
#include "dllParser.hpp"


#pragma comment(lib, "netapi32.lib")

namespace lpp
{
    bool split(const std::string & strData, const char * pszSeparator, std::list<std::string>& lstOutput)
    {
        if (strData.length() == 0 || NULL == pszSeparator)
        {
            return false;
        }

        std::string strTmp = strData;
        char *pszTok = NULL;
        char *pNext_token = NULL;
        pszTok = strtok_s((char *)strTmp.c_str(), pszSeparator, &pNext_token);
        while (pszTok)
        {
            lstOutput.push_back(pszTok);
            pszTok = strtok_s(NULL, pszSeparator, &pNext_token);
        }

        return true;
    }

    bool sm_bServer = false;
    bool sm_bRemoteSession = false;
    OSVersionEnum sm_eCurrentOS = WIN_UNKNOWN;
    int sm_nCurrentOSMajor = 0;
    int sm_nCurrentOSMinor = 0;
    int sm_nCurrentOSserverPack = 0;

    bool OS::isOSAtLeast(OSVersionEnum eRequieOS, int nRequireServerPack)
    {
        if (0 == sm_nCurrentOSMajor)
        {
            intitOSVersionInfo();
        }

        int major = 0;
        int minor = 0;

        switch (eRequieOS)
        {
        case WIN_2000:
            major = 5;
            minor = 0;
            break;
        case WIN_XP:
            major = 5;
            minor = 1;
            break;
        case WIN_SERVER_2003:
            major = 5;
            minor = 2;
            break;
        case WIN_VISTA:
        case WIN_SERVER_2008:
            major = 6;
            minor = 0;
            break;
        case WIN_7:
        case WIN_SERVER_2008_R2:
            major = 6;
            minor = 1;
            break;
        case WIN_8:
        case WIN_SERVER_2012:
            major = 6;
            minor = 2;
            break;
        case WIN_8_1:
        case WIN_SERVER_2012_R2:
            major = 6;
            minor = 3;
            break;
        case WIN_10:
        case WIN_SERVER_2016:
            major = 10;
            minor = 0;
            break;
        default:
            break;
        }

        return(((sm_nCurrentOSMajor << 16) | (sm_nCurrentOSMinor << 8) | sm_nCurrentOSserverPack) >=
               ((major << 16) | (minor < 8) | nRequireServerPack));
    }

    void OS::intitOSVersionInfo()
    {
        sm_bRemoteSession = GetSystemMetrics(SM_REMOTESESSION) != 0;

        // 获取操作系统信息
        WKSTA_INFO_100 * pwksInfo = NULL;
        NET_API_STATUS netStatus = NetWkstaGetInfo(NULL, 100, (LPBYTE*)&pwksInfo);
        if (NERR_Success == netStatus)
        {
            sm_nCurrentOSMajor = pwksInfo->wki100_ver_major;
            sm_nCurrentOSMinor = pwksInfo->wki100_ver_minor;

            sm_bServer = IsWindowsServer();

            if (sm_nCurrentOSMajor == 5 && sm_nCurrentOSMinor == 0)
            {
                sm_eCurrentOS = WIN_2000;
            }
            else if (sm_nCurrentOSMajor == 5 && sm_nCurrentOSMinor == 1)
            {
                sm_eCurrentOS = WIN_XP;
            }
            else if (sm_nCurrentOSMajor == 6 && sm_nCurrentOSMinor == 0)
            {
                sm_eCurrentOS = sm_bServer ? WIN_SERVER_2008 : WIN_VISTA;
            }
            else if (sm_nCurrentOSMajor == 6 && sm_nCurrentOSMinor == 1)
            {
                sm_eCurrentOS = sm_bServer ? WIN_SERVER_2008_R2 : WIN_7;
            }
            else if (sm_nCurrentOSMajor == 6 && sm_nCurrentOSMinor == 2)
            {
                sm_eCurrentOS = sm_bServer ? WIN_SERVER_2012 : WIN_8;
            }
            else if (sm_nCurrentOSMajor == 6 && sm_nCurrentOSMinor == 3)
            {
                sm_eCurrentOS = sm_bServer ? WIN_SERVER_2012_R2 : WIN_8_1;
            }
            else if (sm_nCurrentOSMajor == 10 && sm_nCurrentOSMinor == 0)
            {
                sm_eCurrentOS = sm_bServer ? WIN_SERVER_2016 : WIN_10;
            }
            else
            {
                sm_eCurrentOS = WIN_UNKNOWN;
            }

            netStatus = NetApiBufferFree(pwksInfo);
        }
    }

    bool OS::is64BitOS()
    {
        bool bResult = false;
        SYSTEM_INFO si;
        DllParser oDllCaller;
        if (oDllCaller.Load("Kernel32.dll"))
        {
            try
            {
                oDllCaller.ExcecuteFunc<void(SYSTEM_INFO *)>("GetNativeSystemInfo", &si);
            }
            catch (...)
            {
                GetSystemInfo(&si);
            }

            if (PROCESSOR_ARCHITECTURE_AMD64 == si.wProcessorArchitecture ||
                PROCESSOR_ARCHITECTURE_IA64 == si.wProcessorArchitecture)
            {
                bResult = true;
            }
        }

        return bResult;
    }

    bool OS::isWindows2000()
    {
        if (0 == sm_nCurrentOSMajor)
        {
            intitOSVersionInfo();
        }

        return (sm_nCurrentOSMajor == 5 && sm_nCurrentOSMinor == 0);
    }

    bool OS::isWinXp()
    {
        if (0 == sm_nCurrentOSMajor)
        {
            intitOSVersionInfo();
        }

        return (sm_nCurrentOSMajor == 5 && sm_nCurrentOSMinor == 1);
    }

    bool OS::isWindows7()
    {
        if (0 == sm_nCurrentOSMajor)
        {
            intitOSVersionInfo();
        }

        return (sm_nCurrentOSMajor == 6 && sm_nCurrentOSMinor == 1);
    }

    lpp::OSVersionEnum OS::getCurrentOsType()
    {
        if (WIN_UNKNOWN == sm_eCurrentOS)
        {
            intitOSVersionInfo();
        }

        return sm_eCurrentOS;
    }

    EnviromentGuard::EnviromentGuard() : m_bCloseFileRed(FALSE), m_oldValue(nullptr)
    {
        if (OS::is64BitOS())
        {
            DllParser oDllCaller;
            if (oDllCaller.Load("Kernel32.dll"))
            {
                m_bCloseFileRed = oDllCaller.ExcecuteFunc<BOOL (PVOID *)>("Wow64DisableWow64FsRedirection", &m_oldValue);
            }            
        }
    }

    EnviromentGuard::~EnviromentGuard()
    {
        if (m_bCloseFileRed)
        {
            DllParser oDllCaller;
            if (oDllCaller.Load("Kernel32.dll"))
            {
                oDllCaller.ExcecuteFunc<BOOL (PVOID)>("Wow64RevertWow64FsRedirection", m_oldValue);
            }
        }
    }

#ifndef PIPE_INPUT_BUFFER_SIZE
#define PIPE_INPUT_BUFFER_SIZE  4096
#define PIPE_OUTPUT_BUFFER_SIZE 2048
#endif // !PIPE_INPUT_BUFFER_SIZE



    bool CCmdoutput::GetCmdOutput(const std::string & strCommond, std::list<std::string>& lstOutput)
    {
        bool bRtn = false;
        std::string strRead;

        // 创建匿名管道
        SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
        HANDLE hRead  = INVALID_HANDLE_VALUE;
        HANDLE hWrite = INVALID_HANDLE_VALUE;
        HandleHolder<HANDLE> hPipeRead(nullptr);
        //

        if (!CreatePipe(&hRead, &hWrite, &sa, 0))
        {
            // error
             return bRtn;
        }
        else
        {
            hPipeRead.setValue(hRead);
        }  

        // 设置命令行进程启动信息(以隐藏方式启动命令并定位其输出到hWrite
        STARTUPINFOA si = { sizeof(STARTUPINFOA) };
        PROCESS_INFORMATION pi;
        GetStartupInfoA(&si);
        si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        si.wShowWindow = SW_HIDE;
        si.hStdError = hWrite;
        si.hStdOutput = hWrite;

        // 启动命令行        
        if (!CreateProcessA(NULL, (char *)strCommond.c_str(), NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
        {
            return bRtn;
            //return TEXT("Cannot create process");
        }
        else
        {
            // 等待调用完成，释放句柄
            HandleHolder<HANDLE> hPrecess(pi.hProcess);
            HandleHolder<HANDLE> hThread(pi.hThread);
            HandleHolder<HANDLE> hPipeWrite(hWrite);

            // 等待执行完成
            if (WaitForSingleObject(pi.hProcess, 2000) == WAIT_FAILED)
            {
                return false;
            }
        }

        // 获取输出结果
        char szBuffer[PIPE_INPUT_BUFFER_SIZE + 1] = {0};
        DWORD dwBytesRead = 0;
        DWORD dwAllBytesRead = 0;

        while (ReadFile(hRead, szBuffer, PIPE_INPUT_BUFFER_SIZE, &dwBytesRead, NULL))
        {
            if (dwBytesRead > 0)
            {
                strRead += szBuffer;
                dwAllBytesRead += dwBytesRead;
            }
            else
            {
                break;
            }
            
            memset(szBuffer, 0, PIPE_INPUT_BUFFER_SIZE + 1);
        }

        if (dwAllBytesRead)
        {
            split(strRead.c_str(), "\r\n", lstOutput);
        }
  
        bRtn = true;

        return bRtn;
    }

    bool CCmdoutput::GetCmdOutput64(const std::string & strCommond, std::list<std::string>& lstOutput)
    {
        EnviromentGuard oGuard;

        return GetCmdOutput(strCommond, lstOutput);
    }
}