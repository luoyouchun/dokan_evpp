#pragma once

template <typename HandleT>
class HandleHolder
{
public:
    HandleHolder(HandleT hHandle) : m_hHandle(hHandle){}
    ~HandleHolder()
    {
        if (m_hHandle != nullptr)
        {
            Close();
        }
    }

    // ���þ��
    void setValue(HandleT hHandle)
    {
        if (m_hHandle != nullptr)
        {
            Close();
        }

        m_hHandle = hHandle;
    }
private:
    void Close();
    HandleT m_hHandle;
};

