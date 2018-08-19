#ifndef ___BASEPROXY_H__
#define ___BASEPROXY_H__

#include "nocopyable.h"
#include <memory>
#include <atomic>
#include "plugin.h"

/// <summary>
/// 插件基础类
/// </summary>
class CBaseproxy:lpp::noncopyable
{
public:
    CBaseproxy(): m_bInit(false), m_bRegister(false) {};
    ~CBaseproxy() {};

    /// <summary>
    /// 插件模块初始化，模块的一些初始化工作可以在这里完成
    /// </summary>
    bool InitModule(int logLevel, const char * pstrLogName);

    /// <summary>
    /// 插件注册导出接口
    /// </summary>
    void* Register(void *pWriteToChannel, void * pFunReadFromChannel, HMODULE hModule);

    /// <summary>
    /// 取消初始化
    /// </summary>
    bool UninitModule();

    /// <summary>
    /// 启动插件
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