#pragma once

#include <evpp/tcp_conn.h>
#include <evpp/buffer.h>

class LengthHeadercodec
{
public:
    typedef std::function<void(const evpp::TCPConnPtr &,
                               const std::string& message)> StringMessageCallback;

    explicit LengthHeadercodec(const StringMessageCallback& cb)
        :m_messageCallback(cb)
    {}

    void onMessage(const evpp::TCPConnPtr & conn, evpp::Buffer* buf)
    {
        while (buf->size() >= m_nHeaderLen)
        {
            size_t stSize = buf->size();
            const int32_t nLength = buf->PeekInt32();
            if (nLength > INT_MAX || nLength <= 0)
            {
                conn->Close();
                break;
            }

            if (buf->size() >= nLength + m_nHeaderLen)
            {
                buf->Skip(m_nHeaderLen);
                std::string  strMessage(buf->NextString(nLength));
                m_messageCallback(conn, strMessage);
            }
            else
            {
                break;
            }
        }
    }

    void Send(const evpp::TCPConnPtr & conn, const evpp::slice message)
    {
        evpp::Buffer buf;
        buf.Append(message.data(), message.size());
        buf.PrependInt32(message.size());
        conn->Send(&buf);
    }


private:
    StringMessageCallback m_messageCallback;
    const static size_t m_nHeaderLen = sizeof(int32_t);
};
