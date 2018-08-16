#pragma once

#ifndef __RESPONSE_H__
#define __RESPONSE_H__

#include "message.h"

namespace lpp
{
    class CResponse : public CMessage
    {
    public:
        CResponse(int32_t nTag) : CMessage(nTag)
        {
            m_eType = kResponse;
        }

        CResponse(CMessage&& msg) : CMessage(std::move(msg))
        {
            m_eType = kResponse;
        }
    };
}

#endif // __RESPONSE_H__
