#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class JobSystem
{
public:
    static JobSystem& Instance()
    {
        static JobSystem instance;
        return instance;
    }

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    void Initialize(int configThreads = -1);
    void Shutdown();

    using JobCounter = std::atomic<uint32_t>;

    void Execute(std::function<void()> job, JobCounter* counter = nullptr);
    
    template <typename F, typename... Args>
    auto ExecuteAsync(F&& f, Args&&... args) -> std::future<typename std::invoke_result_t<F, Args...>>
    {
        using return_type = typename std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        Execute([task]() { (*task)(); });

        return res;
    }

    void Wait();
    void Wait(JobCounter* counter);
    
    bool IsBusy();
    uint32_t GetThreadCount() const { return (uint32_t)m_Workers.size(); }

private:
    JobSystem() = default;
    ~JobSystem() = default;

    struct Job
    {
        std::function<void()> task;
        JobCounter* counter;
    };

    void WorkerLoop();

    std::vector<std::thread> m_Workers;
    std::queue<Job> m_JobQueue;

    std::mutex m_QueueMutex;
    std::condition_variable m_Condition;
    
    // Separate mutex for wait/counter notifications to avoid blocking workers
    std::mutex m_WaitMutex;
    std::condition_variable m_WaitCondition;
    std::condition_variable m_CounterCondition;

    std::atomic<uint32_t> m_ActiveJobs{0};
    std::atomic<bool> m_IsRunning{false};
};