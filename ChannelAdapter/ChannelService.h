/*---------------------------------------------------------------------------------
* Copyright(C),2018
* FileName:  ChannelService.h定义
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
        /// 将报文处理绑定到通道适配器的收发报文接口，然后通知其开始工作
        /// </summary>
        bool Start();

        /// <summary>
        /// 停止绑定，释放资源
        /// </summary>
        /// <returns></returns>
        bool Stop();

        /// <summary>
        /// 设备处理请求报文的回调函数
        /// </summary>
        /// <param name="cb"></param>
        void SetMessageCallback(funMessageCallback cb);

        /// <summary>
        /// 同步发送报文，发送成功后，等待回应，匹配回应报文
        /// </summary>
        /// <param name="osliceRequest">待发送的请求报文</param>
        /// <param name="pBufResponse">加应报文缓冲</param>
        /// <param name="nTimeout">同步最长等待时长</param>
        /// <returns>发送报文的结果</returns>
        eStatus Send(const evpp::slice &osliceRequest, evpp::Buffer *pBufResponse, uint32_t nTimeout = 0);

        /// <summary>
        /// 同上发送报文，不需要处理回应
        /// </summary>
        /// <param name="osliceRequest"></param>
        /// <returns></returns>
        eStatus Send(const evpp::slice &osliceRequest);
    private:
        /// <summary>
        /// 报文处理统一接口，在些进行报文的分流
        /// 请求报文：交给业务逻辑模块处理
        /// 回应报文：通知异步等待发送的请求处理单无处理回应
        /// </summary>
        /// <param name="poMessage"></param>
        void OnMessage(CMessage * poMessage);

        /// <summary>
        /// 同步发送请求报文，等待回应
        /// </summary>
        /// <param name="pRequest"></param>
        /// <param name="oResponse"></param>
        /// <returns></returns>
        eStatus SendSync(SPtrRequest pRequest, CResponse &oResponse);

        uint32_t GetNextReqId() { return ++m_ReqIndex; }
    private:
        typedef std::promise<std::shared_ptr<CMessage>> ResponsePromise;

        funMessageCallback                            m_funMessageCallback;   // 报文处理回调函数
        ChannelAdapter*                               m_poChannelAdapter;     // 构造时绑定的通道适配实例
        ThreadPool                                    m_oThreadPool;          // 报文处理的线程池
        std::atomic<uint32_t>                         m_ReqIndex;             // 请求报文计数
        std::unordered_map<uint32_t, ResponsePromise> m_mapRequest;           // 同步请求列表
        std::mutex                                    m_mutex;
        volatile std::atomic<bool>                    m_bRunning;             // 标识是否运行
    };

}

#endif