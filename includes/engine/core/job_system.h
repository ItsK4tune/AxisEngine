#pragma once

#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>

class JobSystem
{
public:
    static JobSystem& Instance()
    {
        static JobSystem instance;
        return instance;
    }

    // Prevents copying
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    /**
     * Initializes the thread pool. Automatically spawns threads based on hardware concurrency.
     */
    void Initialize();

    /**
     * Shuts down the thread pool and joins all threads. Call this before application exit.
     */
    void Shutdown();

    /**
     * Enqueues a job for asynchronous execution on a worker thread.
     * @param job A callable function to execute.
     */
    void Execute(std::function<void()> job);

    /**
     * Enqueues a job for asynchronous execution and returns a future for the result.
     */
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

    /**
     * Waits until all currently executing and queued jobs are finished.
     */
    void Wait();

    /**
     * Checks if the job system is currently busy executing jobs.
     */
    bool IsBusy();

private:
    JobSystem() = default;
    ~JobSystem() = default;

    void WorkerLoop();

    std::vector<std::thread> m_Workers;
    std::queue<std::function<void()>> m_JobQueue;

    std::mutex m_QueueMutex;
    std::condition_variable m_Condition;
    std::condition_variable m_WaitCondition;

    std::atomic<uint32_t> m_ActiveJobs{0};
    std::atomic<bool> m_IsRunning{false};
};

