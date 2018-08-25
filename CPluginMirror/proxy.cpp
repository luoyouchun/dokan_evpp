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

    /// <summary>
    /// 开始磁盘映射
    /// </summary>
    /// <param name="bReadonly">是否只读</param>
    /// <param name="bNetwork">是否网络磁盘映射</param>
    /// <param name="pstrRootDirectory">本地映射的磁盘目录</param>
    /// <param name="pMountId">映射成功后的ID</param>
    /// <returns>映射结果</returns>
    int CProxyService::Map(bool bReadonly, bool bNetwork, const TCHAR * pstrRootDirectory, unsigned long * pMountId)
    {
        // 实现磁盘映射的准备
        if (!hasRegister())
        {
            return STATUS_DLL_INIT_FAILED;
        }

        // todo 磁盘映射，由于管理类还没有实现，暂时空着

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
                    // 处理报文错误，应该是不会出现的
                }
            }
        }
    }

}

