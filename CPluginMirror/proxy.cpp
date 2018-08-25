/*---------------------------------------------------------------------------------
* Copyright(C),2018
* FileName:  proxy.cpp
* Author:    luoyouchun
* Version:   1.0
* Date:      2018-8-111
* Description:  Client proxy
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

    /// <summary>
    /// ��ʼ����ӳ��
    /// </summary>
    /// <param name="bReadonly">�Ƿ�ֻ��</param>
    /// <param name="bNetwork">�Ƿ��������ӳ��</param>
    /// <param name="pstrRootDirectory">����ӳ��Ĵ���Ŀ¼</param>
    /// <param name="pMountId">ӳ��ɹ����ID</param>
    /// <returns>ӳ����</returns>
    int CProxyService::Map(bool bReadonly, bool bNetwork, const TCHAR * pstrRootDirectory, unsigned long * pMountId)
    {
        // ʵ�ִ���ӳ���׼��
        if (!hasRegister())
        {
            return STATUS_DLL_INIT_FAILED;
        }

        // todo ����ӳ�䣬���ڹ����໹û��ʵ�֣���ʱ����

        return 0;
    }

    int CProxyService::UnMap(unsigned long nMountId)
    {
        return 0;
    }

    int CProxyService::GetMaps(uint32_t nSize, void * pBuffer)
    {
        return 0;
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

