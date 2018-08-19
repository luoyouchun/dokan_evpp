#ifndef ___BASEPROXY_H__
#define ___BASEPROXY_H__

#include "nocopyable.h"
#include <memory>
#include <atomic>
#include "plugin.h"

/// <summary>
/// ���������
/// </summary>
class CBaseproxy:lpp::noncopyable
{
public:
    CBaseproxy(): m_bInit(false), m_bRegister(false) {};
    ~CBaseproxy() {};

    /// <summary>
    /// ���ģ���ʼ����ģ���һЩ��ʼ�������������������
    /// </summary>
    bool InitModule(int logLevel, const char * pstrLogName);

    /// <summary>
    /// ���ע�ᵼ���ӿ�
    /// </summary>
    void* Register(void *pWriteToChannel, void * pFunReadFromChannel, HMODULE hModule);

    /// <summary>
    /// ȡ����ʼ��
    /// </summary>
    bool UninitModule();

    /// <summary>
    /// �������
    /// </summary>
    virtual void Start(void * pWriteToChannel) = 0;

    virtual void Stop() = 0;

    virtual uint32_t GetPluginType() = 0;

    virtual const char * GetLogName() const = 0;

    bool hasRegister() { return m_bRegister; }

public:
    std::unique_ptr<PluginInfo>  m_pPluginInfo;
    std::atomic<bool>            m_bInit;
    std::atomic<bool>            m_bRegister;
};

#endif // ___BASEPROXY_H__