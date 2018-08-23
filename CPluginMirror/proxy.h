/*---------------------------------------------------------------------------------
* Copyright(C),2018
* FileName:  proxy.h∂®“Â
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
        /// 
        /// </summary>
        virtual void Start(void * pWriteToChannel) override;

        virtual void Stop() override; 

        virtual uint32_t GetPluginType() override;

        virtual const char * GetLogName() const override;   

        int Map(bool bReadonly, bool bNetwork, const TCHAR * pstrRootDirectory, uint32_t *pMountId);

        int UnMap(uint32_t MountId);

        int GetMaps(uint32_t nSize, void * pBuffer);

        void ReadFormChannel(uint32_t uSize, void *pData);

    private:
        std::unique_ptr<ChannelAdapter> m_pChannelAdapter;
    };
}



#endif  // _CLIENT_PROXY_H_
