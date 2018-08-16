#pragma once

#include <chrono>

namespace lpp
{
    class Timer
    {
    public:
        Timer() : m_begin(std::chrono::high_resolution_clock::now()) {}
        void reset()
        {
            m_begin = std::chrono::high_resolution_clock::now();
        }

        template<typename Duration = std::chrono::milliseconds>
        int64_t elapsed() const
        {
            return std::chrono::duration_cast<Duration>(std::chrono::high_resolution_clock::now() - m_begin).count();
        }
    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_begin;
    };

}
