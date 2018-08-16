#pragma once

#ifndef __DLL_PARSER_H__
#define __DLL_PARSER_H__

#include <Windows.h>
#include <string>
#include <map>
#include <functional>


namespace lpp
{
    class DllParser
    {
    public:

        DllParser() : m_hMod(nullptr)
        {
        }

        ~DllParser()
        {
            UnLoad();
        }

        bool Load(const std::string& dllPath)
        {
            m_hMod = LoadLibraryA(dllPath.data());
            if (nullptr == m_hMod)
            {
                return false;
            }

            return true;
        }

        bool UnLoad()
        {
            bool bResult = false;
            if (nullptr == m_hMod)
            {
                bResult = true;
            }
            else
            {
                FreeLibrary(m_hMod);
                bResult = true;
                m_hMod = nullptr;
            }
            
            return bResult;
        }

        template <typename T>
        std::function<T> GetFunction(const std::string& funcName)
        {
            auto it = m_map.find(funcName);
            if (it != m_map.end())
            {
                auto addr = GetProcAddress(m_hMod, funcName.c_str());
                if (!addr)
                {
                    return nullptr;
                }

                m_map.insert(std::make_pair(funcName, addr));
                it = m_map.find(funcName);
            }

            return std::function<T>((T*)(it->second));
        }

        template <typename T, typename... Args>
        typename std::result_of<std::function<T>(Args...)>::type ExcecuteFunc(const std::string& funcName, Args&&... args)
        {
            auto fun = GetFunction<T>(funcName);
            if (fun == nullptr)
            {
                std::string s = "can not find this function " + funcName;
                throw std::exception(s.c_str());
            }

            return fun(std::forward<Args>(args)...);
        }


    private:
        HMODULE m_hMod;
        std::map<std::string, FARPROC> m_map;
    };
}



#endif