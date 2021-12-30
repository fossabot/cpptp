#pragma once

#include <mutex>
#include <queue>
#include <atomic>
#include <future>
#include <memory>
#include <thread>
#include <functional>
#include <type_traits>
#include <condition_variable>

namespace cpptp
{
    class Worker
    {
    public:
        using task_type = std::function<void()>;

        Worker();

        Worker(const Worker&) = delete;
        Worker& operator= (const Worker&) = delete;

        ~Worker();

    public:
        void stop() noexcept;
        bool stopped() const noexcept;

        template<class F, class ... Args>
        std::future<std::result_of_t<F(Args...)>> submit(F&& f, Args&& ... args)
        {
            auto task = std::make_shared<std::packaged_task<std::result_of_t<F(Args...)>()>>([=] {
                return f(args...);
            });

            {
                std::unique_lock<std::mutex> l(m_Mutex);

                m_Tasks.emplace([=] {
                    (*task)();
                });
            }

            m_ConditionVariable.notify_one();
            return task->get_future();
        }

    private:
        std::mutex m_Mutex;
        std::condition_variable m_ConditionVariable;
        std::queue<task_type> m_Tasks;
        std::thread m_WorkerThread;
        std::atomic<bool> m_Stopped;
    };
} // namespace cpptp