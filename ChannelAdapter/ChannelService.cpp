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

    }

    lpp::eStatus ChannelService::SendSync(SPtrRequest pReauest, CResponse &oResponse)
    {

    }

}