/*---------------------------------------------------------------------------------
* Copyright(C),2018
* FileName:  SPluginMirror.h ∂®“Â
* Author:    luoyouchun
* Version:   1.0
* Date:      2018-8-25
* Description:  SPlugin interface
*---------------------------------------------------------------------------------
* Modification Histroy
Date    Author      Modification:
*
*---------------------------------------------------------------------------------*/

#ifndef _SPLUGIN_MIRROR_H_
#define _SPLUGIN_MIRROR_H_

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#ifdef __cplusplus
extern "C"
{
#endif

    void unMountAll();

    void setLogLevel(int nLogLevel);

#ifdef __cplusplus
}
#endif

#endif // !_SPLUGIN_MIRROR_H_
