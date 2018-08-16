#pragma once

#ifndef __REQUEST_H__
#define __REQUEST_H__

#include "message.h"

namespace lpp
{
    class CRequest : public CMessage
    {
    public:
        CRequest(int32_t nTag, int32_t nRequestId, bool bNeedResponse, int nTimeout)
            : CMessage(nTag), m_bNeedResponse(bNeedResponse), m_nTimeOut(nTimeout)
        {
            m_eType = kRequest;
            m_nRequestId = nRequestId;
        }

        int32_t getTimeout() const { return m_nTimeOut;}

        bool getNeedResponse() const {return m_bNeedResponse;}
    private:
        bool m_bNeedResponse;
        int32_t m_nTimeOut;

    };

    typedef std::shared_ptr<CRequest> SPtrRequest;
}

#endif // __RESPONSE_H__
