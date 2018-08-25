/*---------------------------------------------------------------------------------
* Copyright(C),2018
* FileName:  proxy.h����
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
        /// �������������Ĵ���ģ�鿪ʼ��������������ߵ���
        /// </summary>
        /// <param name="pWriteToChannel">����������ṩ�ķ������ݵĽӿڻص�����</param>
        virtual void Start(void * pWriteToChannel) override;

        /// <summary>
        /// ֹͣ����������ͷ������Դ
        /// </summary>
        virtual void Stop() override; 

        /// <summary>
        /// ��ȡ��������ͨͨ��ID��ÿ��������ǲ�һ����
        /// </summary>
        /// <returns>ID</returns>
        virtual uint32_t GetPluginType() override;

        /// <summary>
        /// ��ȡ��־����
        /// </summary>
        /// <returns>��־��</returns>
        virtual const char * GetLogName() const override;   

        /// <summary>
        /// ��ʼ����ӳ��
        /// </summary>
        /// <param name="bReadonly">�Ƿ�ֻ��</param>
        /// <param name="bNetwork">�Ƿ��������ӳ��</param>
        /// <param name="pstrRootDirectory">����ӳ��Ĵ���Ŀ¼</param>
        /// <param name="pMountId">ӳ��ɹ����ID</param>
        /// <returns>ӳ����</returns>
        int Map(bool bReadonly, bool bNetwork, const TCHAR * pstrRootDirectory, unsigned long* pMountId);

        int UnMap(unsigned long nMountId);

        int GetMaps(uint32_t nSize, void * pBuffer);

        void ReadFormChannel(uint32_t uSize, void *pData);

    private:
        std::unique_ptr<ChannelAdapter> m_pChannelAdapter;
    };
}



#endif  // _CLIENT_PROXY_H_
