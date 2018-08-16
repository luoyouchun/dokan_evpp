/*---------------------------------------------------------------------------------
* Copyright(C),2018
* FileName:  system.h定义
* Author:    luoyouchun
* Version:   1.0
* Date:      2018-8-111
* Description:  Windows 操作系统类型获取、关闭系统调用重定向、获取命令行输出结果
*---------------------------------------------------------------------------------
* Modification Histroy 
Date    Author      Modification:
*
*---------------------------------------------------------------------------------*/


#pragma once


#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <string>
#include <list>


namespace lpp
{
    // 操作系统类型
    typedef enum
    {
        // IMPORTANT: If you add a new item here, update IsOSVersionAtLeast().

        WIN_UNKNOWN = 0,
        WIN_31,
        WIN_95,
        WIN_98,
        WIN_ME,
        WIN_NT3,
        WIN_NT4,
        WIN_2000,
        WIN_XP,
        WIN_XP64,
        WIN_SERVER_2003,
        WIN_VISTA,
        WIN_SERVER_2008,
        WIN_7,
        WIN_SERVER_2008_R2,
        WIN_8,
        WIN_SERVER_2012,
        WIN_8_1,
        WIN_SERVER_2012_R2,
        WIN_10,
        WIN_SERVER_2016
    } OSVersionEnum;

    /// <summary>
    /// Use oS to check the operation system
    /// </summary>
    class OS
    {
    public:
        static bool isOSAtLeast(OSVersionEnum eRequieOS, int nRequireServerPack);

        static void intitOSVersionInfo();

        static bool is64BitOS();

        static bool isWindows2000();

        static bool isWinXp();

        static bool isWindows7();

        static OSVersionEnum getCurrentOsType();

        static bool sm_bServer;
        static bool sm_bRemoteSession;
        static OSVersionEnum sm_eCurrentOS;
        static int sm_nCurrentOSMajor;
        static int sm_nCurrentOSMinor;
        static int sm_nCurrentOSserverPack;
    };

    class EnviromentGuard
    {
    public:
        EnviromentGuard();
        ~EnviromentGuard();

    private:
        BOOL m_bCloseFileRed;
        PVOID m_oldValue;
    };

    class CCmdoutput
    {
    public :
        static bool GetCmdOutput(const std::string & strCommond, std::list<std::string>& lstOutput);
        static bool GetCmdOutput64(const std::string & strCommond, std::list<std::string>& lstOutput);
    };

    bool split(const std::string & strData, const char * pszSeparator, std::list<std::string>& lstOutput);
}


#endif