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


#include "stdafx.h"
#include "ChannelService.h"


namespace lpp
{

    ChannelService::ChannelService(ChannelAdapter * pChannelAdapter)
        : m_poChannelAdapter(pChannelAdapter)
        , m_ReqIndex(0)
        , m_oThreadPool(std::thread::hardware_concurrency(), 0, true)
        , m_bRunning(false)
    {
    }

    ChannelService::~ChannelService()
    {
        Stop();
    }

    bool ChannelService::Start()
    {
        m_poChannelAdapter->SetMessageCallback(std::bind(&ChannelService::OnMessage, this, std::placeholders::_1));

        m_bRunning = true;

        return m_poChannelAdapter->Start();
    }

    bool ChannelService::Stop()
    {
        if (m_bRunning)
        {
            m_poChannelAdapter->SetMessageCallback(std::move(ChannelAdapter::MessageCallback()));
            m_bRunning = false;

            // stop thread pool block until the thread pool stoped.
            m_oThreadPool.stop();

            return m_poChannelAdapter->Stop(1000);

        }

        return true;
    }

    void ChannelService::SetMessageCallback(funMessageCallback cb)
    {
        m_funMessageCallback = cb;
    }

    eStatus ChannelService::Send(const evpp::slice &osliceRequest, evpp::Buffer *pBufResponse, uint32_t nTimeout /*= 0*/)
    {
        SPtrRequest poRequest(new CRequest(DISK_DRIVER_MAPPING_TAG, GetNextReqId(), true, nTimeout));
        CResponse oResponse(DISK_DRIVER_MAPPING_TAG);

        poRequest->setBuffer(osliceRequest);
        auto sResult = SendSync(poRequest, oResponse);
        if (eStatus::sSucceed == sResult)
        {
            oResponse.getBuffer(pBufResponse);
        }

        return sResult;
    }

    eStatus ChannelService::Send(const evpp::slice &osliceRequest)
    {
        SPtrRequest poRequest(new CRequest(DISK_DRIVER_MAPPING_TAG, GetNextReqId(), false, 0));
        CResponse oResponse(DISK_DRIVER_MAPPING_TAG);
        poRequest->setBuffer(osliceRequest);

        auto sResult = SendSync(poRequest, oResponse);
        return sResult;
    }

    void ChannelService::OnMessage(CMessage * poMessage)
    {
        switch (poMessage->getType())
        {
        // 请求报文，由线程池处理
        case kRequest:
        {
            m_oThreadPool.addTask([this, poMessage = std::move(*poMessage)]()mutable
            {
                auto result = m_funMessageCallback(&poMessage);
                m_poChannelAdapter->Send(&result);
            });
            break;
        }
        // 回应报文 
        case kResponse:
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto pos = m_mapRequest.find(poMessage->getReqidx());
            if (pos != m_mapRequest.end())
            {
                pos->second.set_value(std::make_shared<CMessage>(std::move(*poMessage)));
            }
            else
            {
                // Response timeout
            }
            break;
        }            
        default:
            break;
        }

    }

    lpp::eStatus ChannelService::SendSync(SPtrRequest pRequest, CResponse &oResponse)
    {
        std::shared_ptr<ResponsePromise> spResponsePromise = std::make_shared<ResponsePromise>();
        auto ret = spResponsePromise->get_future();

        if (m_poChannelAdapter->Send(pRequest.get()) && pRequest->getNeedResponse())
        {
            // send success then push to map
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_mapRequest.insert(std::make_pair(pRequest->getReqidx(), std::move(*spResponsePromise)));
            }

            // wait for response
            int nTimeout = 0;
            if (nTimeout == pRequest->getTimeout())
            {
                auto wait_result = ret.wait_for(std::chrono::milliseconds(nTimeout));
                if (std::future_status::timeout == wait_result)
                {
                    // timeout
                    int nIndex = pRequest->getReqidx();
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        auto pos = m_mapRequest.find(nIndex);
                        if (pos != m_mapRequest.end())
                        {
                            m_mapRequest.erase(pos);
                        }
                    }

                    return eStatus::sTimeout;
                }
            }

            // .deal the response
            oResponse = std::move(*(ret.get().get()));
            return eStatus::sSucceed;
        }

        return eStatus::sFailed;
    }

}