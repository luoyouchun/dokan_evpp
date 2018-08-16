/*---------------------------------------------------------------------------------
* Copyright(C),2018
* FileName:  strcutDef.h定义
* Author:    luoyouchun
* Version:   1.0
* Date:      2018-8-111
* Description:  适配传输通道接口的实现
*---------------------------------------------------------------------------------
* Modification Histroy
Date    Author      Modification:
*
*---------------------------------------------------------------------------------*/



#include "stdafx.h"
#include <chrono>
#include "ChannelAdapter.h"
#include "message.h"
#include "evpp/buffer.h"

namespace lpp
{

    ChannelAdapter::ChannelAdapter(WriteCallback funWriteCallback, uint32_t nChannelType)
        :m_funWriteToChannel(funWriteCallback)
        , m_nChannelTyep(nChannelType)
    {
        m_funMessageResponse = nullptr;
    }

    ChannelAdapter::~ChannelAdapter()
    {
        Stop(0);
    }

    bool ChannelAdapter::ReadFromChannel(uint32_t uSize, void *pData)
    {
        auto pBuffer = std::make_shared<evpp::Buffer>();
        pBuffer->Append(static_cast<const char *>(pData), uSize);
        return m_msgQueue.push(std::move(pBuffer));
    }


    void ChannelAdapter::SetMessageCallback(MessageCallback cb)
    {
        m_funMessageResponse = cb;
    }

    bool ChannelAdapter::Send(CMessage *msg)
    {
        evpp::Buffer oBuffer;
        if (msg)
        {
            msg->writePre(&oBuffer);
            return m_funWriteToChannel(m_nChannelTyep, oBuffer.size(), (void *)oBuffer.data());
        }
    }

    bool ChannelAdapter::Start()
    {
        if (!m_bRunning)
        {
            m_exit_future = std::move(std::async(std::launch::async, [this] {
                m_bRunning = true;
                this->Run();
                m_bRunning = false;
            }));

            return true;
        }

        return false;
    }

    bool ChannelAdapter::Stop(int nWaitMs /*= 0*/)
    {
        if (m_bRunning)
        {
            // set flag to quit the queue
            if (!m_msgQueue.exited())
            {
                m_msgQueue.exit();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            m_bRunning = false;

            if (m_exit_future.valid())
            {
                // 等待线程执行完成
                if (nWaitMs &&
                    m_exit_future.wait_for(std::chrono::microseconds(nWaitMs)) == std::future_status::timeout)
                {
                    return false;
                }
                else
                {
                    m_exit_future.get();
                }
            }

            return true;
        }
    }

    void ChannelAdapter::Run()
    {
        while (m_bRunning && !m_msgQueue.exited())
        {
            std::shared_ptr<evpp::Buffer> pBuffer = std::make_shared<evpp::Buffer>(0, 0);
            if (m_msgQueue.pop_wait(&pBuffer, 1000))
            {
                if (m_bRunning && pBuffer && pBuffer->size() > 0)
                {
                    // 有消息了，可以处理了
                    // 解析成CMessage
                    CMessage oMsg(DISK_DRIVER_MAPPING_TAG);
                    if (oMsg.read(*pBuffer.get()) && m_funMessageResponse)
                    {
                        m_funMessageResponse(&oMsg);
                    }
                }
            }

        }
    }

}