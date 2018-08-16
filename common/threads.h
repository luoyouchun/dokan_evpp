#pragma once

#ifndef __THREADS_H__
#define __THREADS_H__

#include <functional>
#include <limits>
#include <condition_variable>
#include <mutex>
#include "syncQueue.h"
#include "nocopyable.h"
#include <thread>
#include <utility>

namespace lpp
{
    // 线程池支持的调用方式
    typedef std::function<void()> Task;

    struct ThreadPool : private noncopyable
    {
        // 
        ThreadPool(int nThreads, int nTaskCapacity = 0, bool bStart = true) : m_sqTasks(nTaskCapacity)
        {
            if (bStart)
            {
                start(nThreads);
            }
        }

        ~ThreadPool()
        {
            stop();
        }

        /// <summary>
        /// 结束线程池
        /// </summary>
        void stop()
        {
            std::call_once(m_bFlag, [this] {stopThreadGroup();});
        }

        void addTask(Task&& task)
        {
            m_sqTasks.push(std::forward<Task>(task));
        }

        void addTask(Task& task)
        {
            m_sqTasks.push(task);
        }
    
    private:

        void start(int nThreads)
        {
            m_bRunning = true;

            for (int i = 0; i < nThreads; i++)
            {
                m_thrGroup.push_back(std::make_shared<std::thread>([this] {

                    while (this->m_bRunning)
                    {
                        Task task;
                        if (m_sqTasks.pop_wait(&task, 1000))
                        {
                            task();
                        }
                    }

                }));
            }
        }

        void stopThreadGroup()
        {
            if (!m_sqTasks.exited())
            {
                m_sqTasks.exit();
            }

            m_bRunning = false;

            // 等待所有线程结束
            for (auto thr : m_thrGroup)
            {
                if (thr && thr->joinable())
                {
                    thr->join();
                }
            }

            m_thrGroup.clear();
        }

        CSyncQueue<Task> m_sqTasks;              // 待执行的方法
        std::list<std::shared_ptr<std::thread>> m_thrGroup;
        std::atomic_bool                        m_bRunning;
        std::once_flag                          m_bFlag;
    };

}
#endif
