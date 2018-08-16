/*---------------------------------------------------------------------------------
* Copyright(C),2018
* FileName:  strcutDef.h定义
* Author:    luoyouchun
* Version:   1.0
* Date:      2018-8-111
* Description:  报文收发结果
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
        /// 供插件调用接口ReadFromChannel调用，将收到的报文放到对列中
        /// </summary>
        /// <param name="uSize">需要处理的数据的大小</param>
        /// <param name="pData">需要处理的数据</param>
        /// <returns></returns>
        bool ReadFromChannel(uint32_t uSize, void *pData);

        /// <summary>
        /// 设备消息处理回调函数，用于处理消息
        /// </summary>
        /// <param name="cb">消息处理回调函数</param>
        void SetMessageCallback(MessageCallback cb);

        /// <summary>
        /// 处理完了，通知通道，发送回应报文
        /// </summary>
        /// <param name="msg">待发送的消息</param>
        /// <returns>发送的结果</returns>
        bool Send(CMessage *msg);

        /// <summary>
        /// 启动消息异步处理函数，创建一个异步运行函数，调用ChannelAdapter::Run()处理
        /// </summary>
        /// <returns></returns>
        bool Start();

        /// <summary>
        /// 信息异步消息处理
        /// </summary>
        /// <param name="nWaitMs">超时时间</param>
        /// <returns>异步处理线程停止的结果</returns>
        bool Stop(int nWaitMs = 0);

    private:

        /// <summary>
        /// 消息处理函数，只能被异步运行函数调用 - Start 
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
