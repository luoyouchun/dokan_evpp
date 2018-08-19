#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <Windows.h>

//定义磁盘映射的消息类型
#define VD_AGENT_VDI_CLIENT_DISK_MAP 120

// 定义插件的日志名称
const char *const SDRIVER_MAP_LOG = "SDokan";
const char *const CDRIVER_MAP_LOG = "CDokan";

// 通道接口读写声明
typedef bool (*PFUNCWRITERTOCHANNEL)(UINT uModelType, UINT uSize, void *pData);
typedef bool (*PFUNCREADFROMCHANNEL)(UINT uSize, void *pData);

typedef struct _PluginInfo
{
    UINT                 uModelType;
    PFUNCREADFROMCHANNEL pFuncReadFromChannel;
    HMODULE              hModule;
}PluginInfo,*PPluginInfo;

#ifdef __cplusplus
extern "C"
{
#endif

/// <summary>
/// 
/// </summary>
void *Register(void* pWriteToChannel);

BOOL InitModule(int logLevel);

BOOL UnInitModule();

#ifdef __cplusplus
}
#endif

#endif
