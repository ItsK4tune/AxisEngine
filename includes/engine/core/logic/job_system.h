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

    /**
     * Initializes the thread pool. Automatically spawns threads based on hardware concurrency.
     */
    void Initialize();

    /**
     * Shuts down the thread pool and joins all threads. Call this before application exit.
     */
    void Shutdown();

    /**
     * A counter used to synchronize multiple jobs.
     */
    using JobCounter = std::atomic<uint32_t>;

    /**
     * Enqueues a job for asynchronous execution on a worker thread.
     * @param job A callable function to execute.
     * @param counter Optional counter to decrement when the job is finished.
     */
    void Execute(std::function<void()> job, JobCounter* counter = nullptr);

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
     * Waits until the specified counter reaches zero.
     */
    void Wait(JobCounter* counter);

    /**
     * Checks if the job system is currently busy executing jobs.
     */
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
    std::condition_variable m_WaitCondition;
    std::condition_variable m_CounterCondition;

    std::atomic<uint32_t> m_ActiveJobs{0};
    std::atomic<bool> m_IsRunning{false};
};