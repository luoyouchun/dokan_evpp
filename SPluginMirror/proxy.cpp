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
    /// 启动插件，插件的传输模块开始工作，插件管理者调用
    /// </summary>
    /// <param name="pWriteToChannel">插件管理者提供的发送数据的接口回调函数</param>
    void CProxyService::Start(void * pWriteToChannel)
    {
        m_pChannelAdapter = std::make_unique<ChannelAdapter>(PFUNCWRITERTOCHANNEL(pWriteToChannel), GetPluginType());

        // todo 创建管理类的实例，由于管理类还没有实现，暂时空着

    }

    /// <summary>
    /// 停止插件工作，释放相关资源
    /// </summary>
    void CProxyService::Stop()
    {
        // todo 停止管理类工作，由于管理类还没有实现，暂时空着

        auto ptr = m_pChannelAdapter.release();
        if (ptr)
        {
            delete ptr;
        }
    }

    /// <summary>
    /// 获取插件分配的通通道ID，每个插件都是不一样的
    /// </summary>
    /// <returns>ID</returns>
    uint32_t CProxyService::GetPluginType()
    {
        return VD_AGENT_VDI_CLIENT_DISK_MAP;
    }

    /// <summary>
    /// 获取日志名称
    /// </summary>
    /// <returns>日志名</returns>
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
                    // 处理报文错误，应该是不会出现的
                }
            }
        }
    }
}

