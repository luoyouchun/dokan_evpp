#ifndef __MESSAGETYPE_H__
#define __MESSAGETYPE_H__

#include <stdint.h>
#include "evpp/buffer.h"

namespace lpp
{
    /// <summary>
    /// 消息类型包含请求和回应
    /// </summary>
    enum MessageType : unsigned char
    {
        kRequest = 0,
        kResponse,
        kMessageMac,
    };

    class CMessage
    {
    public:
        CMessage(int32_t nTag) : m_nTag(nTag), m_eType(kRequest), m_nSessionId(0), m_nRequestId(0){};

        CMessage(const CMessage& msg)
        {
            *this = msg;
        }

        CMessage(CMessage &&msg)
        { 
            *this = std::move(msg);
        }

        CMessage &operator=(const CMessage &msg)
        {
            m_nTag = msg.m_nTag;
            m_eType = msg.m_eType;
            m_nSessionId = msg.m_nSessionId;
            m_nRequestId = msg.m_nRequestId;
            
            setBuffer(msg.m_oBuffer);

            return *this;
        }

        CMessage &operator=(const CMessage &&msg)
        {
            m_nTag = msg.m_nTag;
            m_eType = msg.m_eType;
            m_nSessionId = msg.m_nSessionId;
            m_nRequestId = msg.m_nRequestId;
            setBuffer(std::move(msg.m_oBuffer));

            return *this;
        }

        // 反序列化
        bool read(const evpp::slice& oslice)
        {
            evpp::Buffer oBuf;
            oBuf.Append(oslice.data(), oslice.size());
            return read(oBuf);
        }

        bool read(evpp::Buffer& buf)
        {
            if (buf.size() < sm_nHeaderSize)
            {
                return false;
            }

            if (buf.PeekInt32() != m_nTag)
            {
                // TAG不对
                return false;
            }
            buf.Skip(sizeof(m_nTag));

            m_eType = static_cast<MessageType>(buf.PeekInt8());
            buf.Skip(sizeof(m_eType));

            m_nSessionId = buf.PeekInt32();
            buf.Skip(sizeof(m_nSessionId));

            m_nRequestId = buf.PeekInt32();
            buf.Skip(sizeof(m_nRequestId));

            // 读完后，直接将后面的报文交换到缓冲
            m_oBuffer.Swap(buf);

            return true;
        }

        // 前序序列化，减少内存COPY
        void writePre(evpp::Buffer *poBuffer)
        { 
            // 报文交换到poBuffer
            m_oBuffer.Swap(*poBuffer);
            poBuffer->PrependInt32(m_nRequestId);
            poBuffer->PrependInt32(m_nSessionId);
            poBuffer->PrependInt8(static_cast<int8_t>(m_eType));
            poBuffer->PrependInt32(m_nTag);
        }

        // 直接序列化，有一次内存COPY
        void write(evpp::Buffer &oBuffer) const
        { 
            oBuffer.AppendInt32(m_nTag);
            oBuffer.AppendInt8(static_cast<int8_t>(m_eType));
            oBuffer.AppendInt32(m_nSessionId);
            oBuffer.AppendInt32(m_nRequestId);
            oBuffer.Append(m_oBuffer.data(), m_oBuffer.size());
        }

        // 解析完成后，可以直接使用内部的方法
        MessageType getType() const {return m_eType;}
        void setType(MessageType eType) {m_eType = eType;}
        int32_t getSession() const {return m_nSessionId;}
        void setSession(int32_t nSession) {m_nSessionId = nSession;}
        int32_t getReqidx() const { return m_nRequestId; }
        void setReqidx(int32_t nReqidx) { m_nRequestId = nReqidx; }
        const evpp::slice getBuffer() const
        { 
            return evpp::slice(m_oBuffer.data(), m_oBuffer.size());
        }

        // 直接交换走当前buffer
        void getBuffer(evpp::Buffer *poBuffer)
        {
            m_oBuffer.Swap(*poBuffer);
        }


        void setBuffer(const evpp::Buffer &oBuffer)
        {
            m_oBuffer.Reset();
            m_oBuffer.Append(oBuffer.data(), oBuffer.size());
        }

        void setBuffer(const evpp::slice & oSlice)
        {
            m_oBuffer.Reset();
            m_oBuffer.Append(oSlice.data(), oSlice.size());
        }

        void setBuffer(evpp::Buffer&& oBuffer)
        {
            m_oBuffer.Reset();
            m_oBuffer.Swap(oBuffer);
        }

        void appendBuffer(const evpp::Buffer& oBuffer)
        {
            m_oBuffer.Append(oBuffer.data(), oBuffer.size());
        }
     protected:
        int32_t                 m_nTag;   //消息类型
        MessageType             m_eType;
        int32_t                 m_nSessionId; 
        int32_t                 m_nRequestId;   
        evpp::Buffer            m_oBuffer;   // 具体的消息内容

        const static int32_t sm_nHeaderSize = sizeof(int32_t) +
                                              sizeof(MessageType) +
                                              sizeof(int32_t) + sizeof(int32_t);
    };
}

#endif