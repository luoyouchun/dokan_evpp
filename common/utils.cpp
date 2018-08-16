

#include "utils.h"
#include <WinSock2.h>
#include <windows.h>

void HandleHolder<SC_HANDLE>::Close()
{
    CloseServiceHandle(m_hHandle);
    m_hHandle = nullptr;
}

void HandleHolder<HANDLE>::Close()
{
    CloseHandle(m_hHandle);
    m_hHandle = nullptr;
}