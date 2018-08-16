#pragma once

#ifndef __SINGLETON_H__
#define __SINGLETON_H__

template <typename T>
class CSingleton
{
public:
    template<typename... Args>
    static T* Instance(Args&&... args)
    {
        if (m_pInstance == nullptr)
        {
            // 完美转发，构造单例
            m_pInstance = new T(std::forward<Args>(args)...);
        }            

        return m_pInstance;
    }

    static T* GetInstance()
    {
        if (m_pInstance == nullptr)
        {
            return nullptr;
        }

        return m_pInstance;
    }

    static void DestroyInstance()
    {
        if (nullptr != m_pInstance)
        {
            delete m_pInstance;
            m_pInstance = nullptr;
        }
    }

private:
    CSingleton(void);
    virtual ~CSingleton(void);
    CSingleton(const CSingleton&);
    CSingleton& operator = (const CSingleton&);
    static T* m_pInstance;
};

template <class T> T*  CSingleton<T>::m_pInstance = nullptr;

#endif
