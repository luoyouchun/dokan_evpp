#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <Windows.h>

//�������ӳ�����Ϣ����
#define VD_AGENT_VDI_CLIENT_DISK_MAP 120

// ����������־����
const char *const SDRIVER_MAP_LOG = "SDokan";
const char *const CDRIVER_MAP_LOG = "CDokan";

// ͨ���ӿڶ�д����
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
