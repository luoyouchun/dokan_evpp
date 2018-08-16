/*---------------------------------------------------------------------------------
* Copyright(C),2018
* FileName:  strcutDef.h����
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

#ifndef _CHANNELADAPTER_H_
#define _CHANNELADAPTER_H_


#include <functional>
#include <thread>
#include <future>

#include "syncQueue.h"
#include "nocopyable.h"
#include "message.h"


const int32_t   DISK_DRIVER_MAPPING_TAG   =  0xBC12FD70;

namespace lpp
{
    class ChannelAdapter : private noncopyable
    {
    public: 
        typedef std::function<bool(uint32_t, uint32_t, void *)> WriteCallback;
        typedef std::function<void(CMessage *)> MessageCallback;

    public:
        ChannelAdapter(WriteCallback funWriteCallback, uint32_t nChannelType);
        ~ChannelAdapter();

        /// <summary>
        /// ��������ýӿ�ReadFromChannel���ã����յ��ı��ķŵ�������
        /// </summary>
        /// <param name="uSize">��Ҫ��������ݵĴ�С</param>
        /// <param name="pData">��Ҫ���������</param>
        /// <returns></returns>
        bool ReadFromChannel(uint32_t uSize, void *pData);

        /// <summary>
        /// �豸��Ϣ����ص����������ڴ�����Ϣ
        /// </summary>
        /// <param name="cb">��Ϣ����ص�����</param>
        void SetMessageCallback(MessageCallback cb);

        /// <summary>
        /// �������ˣ�֪ͨͨ�������ͻ�Ӧ����
        /// </summary>
        /// <param name="msg">�����͵���Ϣ</param>
        /// <returns>���͵Ľ��</returns>
        bool Send(CMessage *msg);

        /// <summary>
        /// ������Ϣ�첽������������һ���첽���к���������ChannelAdapter::Run()����
        /// </summary>
        /// <returns></returns>
        bool Start();

        /// <summary>
        /// ��Ϣ�첽��Ϣ����
        /// </summary>
        /// <param name="nWaitMs">��ʱʱ��</param>
        /// <returns>�첽�����߳�ֹͣ�Ľ��</returns>
        bool Stop(int nWaitMs = 0);

    private:

        /// <summary>
        /// ��Ϣ��������ֻ�ܱ��첽���к������� - Start 
        /// </summary>
        void Run();

        WriteCallback                             m_funWriteToChannel;
        MessageCallback                           m_funMessageResponse;
        CSyncQueue<std::shared_ptr<evpp::Buffer>> m_msgQueue;
        std::future<void>                         m_exit_future;
        std::atomic<bool>                         m_bRunning = false;
        const uint32_t                            m_nChannelTyep;
    };
}

#endif // _CHANNELADAPTER_H_
