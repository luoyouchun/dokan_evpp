

#pragma once

#ifndef __THREADSAFEQUEUE_H__
#define __THREADSAFEQUEUE_H__

#include <list>
#include <mutex>
#include <atomic>

#include "nocopyable.h"


namespace lpp
{
    /// <summary>
    /// ͬ�����У���������ʱ�򣬻�û��ʵ�ּ�����еȴ���Ŀǰֻ�Ǽ򵥵ķ���ʧ��
    /// �����ߵȴ�PUSH
    /// </summary>
    template<typename T>
    class CSyncQueue : private noncopyable
    {
#ifdef max
#undef max
#endif

        static const int wait_infinite = std::numeric_limits<int>::max();   // ��ȴ�ʱ��
    public:
        CSyncQueue(size_t stCapacity = 0)
            : m_stCapacity(stCapacity), m_bExit(false)
        {

        }

        ~CSyncQueue() {}

      /// <summary>
      /// ����У���ֵ
      /// </summary>
      /// <param name="v"></param>
      /// <returns></returns>
        bool push(T&& v)
        {
            std::lock_guard<std::mutex> lk(m_mutex);

            if (m_bExit || (m_stCapacity && m_lstItems.size() >= m_stCapacity))
            {
                return false;
            }

            m_lstItems.push_back(std::forward<T>(v));

            m_cvNotEmpty.notify_one();

            return true;
        }

        bool push(T &v)
        {
            std::lock_guard<std::mutex> lk(m_mutex);

            if (m_bExit || (m_stCapacity && m_lstItems.size() >= m_stCapacity))
            {
              return false;
            }

            m_lstItems.push_back(v);

            m_cvNotEmpty.notify_one();

            return true;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="waitMs"></param>
        /// <returns></returns>
        T pop_wait(int32_t waitMs = wait_infinite)
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            wait_ready(lk, waitMs);

            if (m_lstItems.empty())
            {
                return T();
            }

            T result = std::move(m_lstItems.front());
            m_lstItems.pop_front();

            return result;
        }

        bool pop_wait(T * v, int32_t waitMs = wait_infinite)
        {
            bool bReslult = false;
            std::unique_lock<std::mutex> lk(m_mutex);

            wait_ready(lk, waitMs);

            if (!m_lstItems.empty())
            {
                bReslult  = true;
                *v = std::move(m_lstItems.front());
                m_lstItems.pop_front();
            }

            return bReslult;
        }

        size_t size()
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            return m_lstItems.size();
        }

        void exit()
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_bExit = true;

            m_cvNotEmpty.notify_one();
        }

        bool exited()
        {
            return m_bExit;
        }

    private:

        /// <summary>
        /// 
        /// </summary>
        /// <param name="lk"></param>
        /// <param name="waitMs"></param>
        void wait_ready(std::unique_lock<std::mutex>& lk, int32_t waitMs)
        {
            // ���в����ˣ����������ݣ�����Ҫ�ȴ�
            if (m_bExit || !m_lstItems.empty())
            {
                return;
            }

            // �ȴ�����
            if (wait_infinite == waitMs)
            {
                m_cvNotEmpty.wait(lk, [this] {return m_bExit || !m_lstItems.empty(); });
            }
            else if (waitMs > 0)
            {
                auto tp = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(waitMs);
                while (m_cvNotEmpty.wait_until(lk, tp) != std::cv_status::timeout &&
                       m_lstItems.size())
                {
                }
            }
        }

        std::list<T> m_lstItems;     // list
        std::condition_variable m_cvNotEmpty;
        std::condition_variable m_cvNotFull;
        size_t                  m_stCapacity;    // ���е���󳤶�
        mutable std::mutex      m_mutex;
        std::atomic_bool        m_bExit;         // ��ʶ�����Ƿ��˳�����

    };
}

#endif
