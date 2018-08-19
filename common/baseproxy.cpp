
#include "baseproxy.h"

bool CBaseproxy::InitModule(int logLevel, const char * pstrLogName)
{
    if (!m_bInit)
    {
        m_bInit = true;
        m_pPluginInfo = std::make_unique<PluginInfo>();
        memset(m_pPluginInfo.get(), sizeof(PluginInfo), 0);

        return true;
    }
    else
    {
        return false;
    }
}

void* CBaseproxy::Register(void *pWriteToChannel, void * pFunReadFromChannel, HMODULE hModule)
{
    if (!m_bInit)
    {
        InitModule(/*plog::info*/5, GetLogName());
    }

    if (m_bInit)
    {
        m_bRegister = true;
        m_pPluginInfo->uModelType = GetPluginType();
        m_pPluginInfo->pFuncReadFromChannel = (PFUNCREADFROMCHANNEL)pFunReadFromChannel;
        m_pPluginInfo->hModule = hModule;

        Start(pWriteToChannel);

        return (void *)m_pPluginInfo.get();
    }
    else
    {
        return NULL;
    }
}

bool CBaseproxy::UninitModule()
{
    if (m_bInit)
    {
        m_bInit = false;
        m_bRegister = false;

        Stop();

        auto ptr = m_pPluginInfo.release();
        if (ptr)
        {
            delete ptr;
        }
    }

    return true;
}
