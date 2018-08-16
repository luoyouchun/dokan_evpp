#pragma once

namespace
{
    struct OnApp
    {
        OnApp()
        {
#ifdef WIN32

            WSAData wsaData;
            int nError = WSAStartup(MAKEWORD(2,2), &wsaData);
            if (nError)
            {
                // error
            }
#endif // WIN32
        }

        ~OnApp()
        {
#ifdef WIN32
            WSACleanup();
#endif // WIN32
        }

    };
}
