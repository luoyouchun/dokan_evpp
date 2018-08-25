/*---------------------------------------------------------------------------------
* Copyright(C),2018
* FileName:  CPluginMirror.h ∂®“Â
* Author:    luoyouchun
* Version:   1.0
* Date:      2018-8-111
* Description:  CPlugin interface
*---------------------------------------------------------------------------------
* Modification Histroy
Date    Author      Modification:
*
*---------------------------------------------------------------------------------*/

#ifndef _CPLUGIN_MIRROR_H_
#define _CPLUGIN_MIRROR_H_

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#ifdef __cplusplus
extern "C"
{
#endif

    unsigned long map(BOOL bReadonly, BOOL bNetwork, const TCHAR * pstrRootDirectory, unsigned long *pMountId);

    unsigned long unmap(unsigned long nMountId);

    void setLogLevel(int nLogLevel);

    unsigned long GetMaps(unsigned int nSize, void * pBuffer);

#ifdef __cplusplus
}
#endif

#endif // !_CPLUGIN_MIRROR_H_
