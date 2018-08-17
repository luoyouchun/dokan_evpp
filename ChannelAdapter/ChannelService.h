/*---------------------------------------------------------------------------------
* Copyright(C),2018
* FileName:  ChannelService.h����
* Author:    luoyouchun
* Version:   1.0
* Date:      2018-8-111
* Description:  �����շ����
*---------------------------------------------------------------------------------
* Modification Histroy
Date    Author      Modification:
*
*---------------------------------------------------------------------------------*/

#pragma once

#ifndef _CHANNELSERVICE_H_
#define _CHANNELSERVICE_H_

#include <atomic>
#include <mutex>
#include <unordered_map>

#include "structDef.h"
#include "response.h"
#include "request.h"
#include "ChannelAdapter.h"
#include "threads.h"

namespace lpp
{
    class ChannelService
    {
    public:
        typedef std::function<CResponse(CMessage *)> funMessageCallback;

        explicit ChannelService(ChannelAdapter * pChannelAdapter);

        ~ChannelService();

        /// <summary>
        /// �����Ĵ���󶨵�ͨ�����������շ����Ľӿڣ�Ȼ��֪ͨ�俪ʼ����
        /// </summary>
        bool Start();

        /// <summary>
        /// ֹͣ�󶨣��ͷ���Դ
        /// </summary>
        /// <returns></returns>
        bool Stop();

        /// <summary>
        /// �豸���������ĵĻص�����
        /// </summary>
        /// <param name="cb"></param>
        void SetMessageCallback(funMessageCallback cb);

        /// <summary>
        /// ͬ�����ͱ��ģ����ͳɹ��󣬵ȴ���Ӧ��ƥ���Ӧ����
        /// </summary>
        /// <param name="osliceRequest">�����͵�������</param>
        /// <param name="pBufResponse">��Ӧ���Ļ���</param>
        /// <param name="nTimeout">ͬ����ȴ�ʱ��</param>
        /// <returns>���ͱ��ĵĽ��</returns>
        eStatus Send(const evpp::slice &osliceRequest, evpp::Buffer *pBufResponse, uint32_t nTimeout = 0);

        /// <summary>
        /// ͬ�Ϸ��ͱ��ģ�����Ҫ�����Ӧ
        /// </summary>
        /// <param name="osliceRequest"></param>
        /// <returns></returns>
        eStatus Send(const evpp::slice &osliceRequest);
    private:
        /// <summary>
        /// ���Ĵ���ͳһ�ӿڣ���Щ���б��ĵķ���
        /// �����ģ�����ҵ���߼�ģ�鴦��
        /// ��Ӧ���ģ�֪ͨ�첽�ȴ����͵��������޴����Ӧ
        /// </summary>
        /// <param name="poMessage"></param>
        void OnMessage(CMessage * poMessage);

        /// <summary>
        /// ͬ�����������ģ��ȴ���Ӧ
        /// </summary>
        /// <param name="pRequest"></param>
        /// <param name="oResponse"></param>
        /// <returns></returns>
        eStatus SendSync(SPtrRequest pRequest, CResponse &oResponse);

        uint32_t GetNextReqId() { return ++m_ReqIndex; }
    private:
        typedef std::promise<std::shared_ptr<CMessage>> ResponsePromise;

        funMessageCallback                            m_funMessageCallback;   // ���Ĵ���ص�����
        ChannelAdapter*                               m_poChannelAdapter;     // ����ʱ�󶨵�ͨ������ʵ��
        ThreadPool                                    m_oThreadPool;          // ���Ĵ�����̳߳�
        std::atomic<uint32_t>                         m_ReqIndex;             // �����ļ���
        std::unordered_map<uint32_t, ResponsePromise> m_mapRequest;           // ͬ�������б�
        std::mutex                                    m_mutex;
        volatile std::atomic<bool>                    m_bRunning;             // ��ʶ�Ƿ�����
    };

}

#endif