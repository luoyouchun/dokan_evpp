/*---------------------------------------------------------------------------------
* Copyright(C),2018
* FileName:  proxy.h定义
* Author:    luoyouchun
* Version:   1.0
* Date:      2018-8-111
* Description:  Client proxy
*---------------------------------------------------------------------------------
* Modification Histroy
Date    Author      Modification:
*
*---------------------------------------------------------------------------------*/

#pragma once


#ifndef _CLIENT_PROXY_H_
#define _CLIENT_PROXY_H_


#include <memory>
#include "ChannelAdapter.h"
#include "baseproxy.h"


namespace lpp
{
    class CProxyService : public CBaseproxy
    {
    public:
        /// <summary>
        /// 启动插件，插件的传输模块开始工作，插件管理者调用
        /// </summary>
        /// <param name="pWriteToChannel">插件管理者提供的发送数据的接口回调函数</param>
        virtual void Start(void * pWriteToChannel) override;

        /// <summary>
        /// 停止插件工作，释放相关资源
        /// </summary>
        virtual void Stop() override; 

        /// <summary>
        /// 获取插件分配的通通道ID，每个插件都是不一样的
        /// </summary>
        /// <returns>ID</returns>
        virtual uint32_t GetPluginType() override;

        /// <summary>
        /// 获取日志名称
        /// </summary>
        /// <returns>日志名</returns>
        virtual const char * GetLogName() const override;   

        /// <summary>
        /// 开始磁盘映射
        /// </summary>
        /// <param name="bReadonly">是否只读</param>
        /// <param name="bNetwork">是否网络磁盘映射</param>
        /// <param name="pstrRootDirectory">本地映射的磁盘目录</param>
        /// <param name="pMountId">映射成功后的ID</param>
        /// <returns>映射结果</returns>
        int Map(bool bReadonly, bool bNetwork, const TCHAR * pstrRootDirectory, unsigned long* pMountId);

        int UnMap(unsigned long nMountId);

        int GetMaps(uint32_t nSize, void * pBuffer);

        void ReadFormChannel(uint32_t uSize, void *pData);

    private:
        std::unique_ptr<ChannelAdapter> m_pChannelAdapter;
    };
}



#endif  // _CLIENT_PROXY_H_
