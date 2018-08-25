/*---------------------------------------------------------------------------------
* Copyright(C),2018
* FileName:  proxy.cpp
* Author:    luoyouchun
* Version:   1.0
* Date:      2018-8-25
* Description:  Server proxy
*---------------------------------------------------------------------------------
* Modification Histroy
Date    Author      Modification:
*
*---------------------------------------------------------------------------------*/


#include "stdafx.h"
#include "proxy.h"
#include "plugin.h"

namespace lpp
{
    /// <summary>
    /// �������������Ĵ���ģ�鿪ʼ��������������ߵ���
    /// </summary>
    /// <param name="pWriteToChannel">����������ṩ�ķ������ݵĽӿڻص�����</param>
    void CProxyService::Start(void * pWriteToChannel)
    {
        m_pChannelAdapter = std::make_unique<ChannelAdapter>(PFUNCWRITERTOCHANNEL(pWriteToChannel), GetPluginType());

        // todo �����������ʵ�������ڹ����໹û��ʵ�֣���ʱ����

    }

    /// <summary>
    /// ֹͣ����������ͷ������Դ
    /// </summary>
    void CProxyService::Stop()
    {
        // todo ֹͣ�����๤�������ڹ����໹û��ʵ�֣���ʱ����

        auto ptr = m_pChannelAdapter.release();
        if (ptr)
        {
            delete ptr;
        }
    }

    /// <summary>
    /// ��ȡ��������ͨͨ��ID��ÿ��������ǲ�һ����
    /// </summary>
    /// <returns>ID</returns>
    uint32_t CProxyService::GetPluginType()
    {
        return VD_AGENT_VDI_CLIENT_DISK_MAP;
    }

    /// <summary>
    /// ��ȡ��־����
    /// </summary>
    /// <returns>��־��</returns>
    const char * CProxyService::GetLogName() const
    {
        return CDRIVER_MAP_LOG;
    }

    void CProxyService::UnAllMount()
    {
        return;
    }

    void CProxyService::ReadFormChannel(uint32_t uSize, void *pData)
    {
        if (!hasRegister())
        {
            // log error not init.
        }
        else
        {
            if (m_pChannelAdapter)
            {
                if (!m_pChannelAdapter->ReadFromChannel(uSize, pData))
                {
                    // �����Ĵ���Ӧ���ǲ�����ֵ�
                }
            }
        }
    }
}

