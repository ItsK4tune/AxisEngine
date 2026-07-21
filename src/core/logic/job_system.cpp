#include <core/logic/job_system.h>
#include <core/logic/logger.h>
#include <core/logic/runtime_profiler.h>
#include <chrono>
#include <exception>
#include <iostream>

namespace
{
thread_local JobSystem* s_ExecutingJobSystem = nullptr;
thread_local JobSystem::JobCounter* s_ExecutingCounter = nullptr;
thread_local uint32_t s_ExecutionDepth = 0;
}

void JobSystem::Initialize(int configThreads)
{
    {
        std::lock_guard lock(m_QueueMutex);
        if (m_IsRunning)
            return;
        m_IsRunning = true;
    }

    uint32_t numThreads;
    if (configThreads > 0)
    {
        numThreads = static_cast<uint32_t>(configThreads);
    }
    else
    {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads > 1)
            numThreads -= 1;
        else
            numThreads = 1;
    }

    m_Workers.reserve(numThreads);
    for (uint32_t i = 0; i < numThreads; ++i)
    {
        m_Workers.emplace_back(&JobSystem::WorkerLoop, this);
    }

    LOGGER_INFO("JobSystem") << "Initialized thread pool with " << numThreads << " worker threads.";
}

void JobSystem::Shutdown()
{
    if (!m_IsRunning)
        return;

    {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        m_IsRunning = false;
    }

    m_Condition.notify_all();

    for (auto& worker : m_Workers)
    {
        if (worker.joinable())
            worker.join();
    }

    m_Workers.clear();
    LOGGER_INFO("JobSystem") << "Thread pool shut down successfully.";
}

void JobSystem::Execute(std::function<void()> job, JobCounter* counter)
{
    if (!job)
        return;

    bool runInline = false;
    {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        if (!m_IsRunning)
            runInline = true;
        else
        {
            m_ActiveJobs.fetch_add(1, std::memory_order_relaxed);
            if (counter)
                counter->fetch_add(1, std::memory_order_relaxed);
            m_JobQueue.push({std::move(job), counter});
        }
    }

    if (!runInline)
    {
        m_Condition.notify_one();
        return;
    }

    m_ActiveJobs.fetch_add(1, std::memory_order_relaxed);
    if (counter)
        counter->fetch_add(1, std::memory_order_relaxed);
    RunJob({std::move(job), counter});
}

void JobSystem::Wait()
{
    const bool recordWait = s_ExecutingJobSystem != this;
    const auto waitStart = std::chrono::steady_clock::now();
    if (s_ExecutingJobSystem == this)
    {
        while (m_ActiveJobs.load(std::memory_order_acquire) > s_ExecutionDepth)
        {
            if (!TryExecuteOne())
                std::this_thread::yield();
        }
        return;
    }
    std::unique_lock<std::mutex> lock(m_WaitMutex);
    m_WaitCondition.wait(lock, [this]() { return m_ActiveJobs.load(std::memory_order_acquire) == 0; });
    if (recordWait)
        RuntimeProfiler::Instance().AddJobWaitTime(
            std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - waitStart).count());
}

void JobSystem::Wait(JobCounter* counter)
{
    if (!counter)
        return;
    const bool recordWait = s_ExecutingJobSystem != this;
    const auto waitStart = std::chrono::steady_clock::now();
    if (s_ExecutingJobSystem == this)
    {
        const uint32_t target = s_ExecutingCounter == counter ? 1u : 0u;
        while (counter->load(std::memory_order_acquire) > target)
        {
            if (!TryExecuteOne())
                std::this_thread::yield();
        }
        return;
    }
    std::unique_lock<std::mutex> lock(m_WaitMutex);
    m_CounterCondition.wait(lock, [counter]() { return counter->load(std::memory_order_acquire) == 0; });
    if (recordWait)
        RuntimeProfiler::Instance().AddJobWaitTime(
            std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - waitStart).count());
}

bool JobSystem::IsBusy()
{
    return m_ActiveJobs.load(std::memory_order_relaxed) > 0;
}

void JobSystem::WorkerLoop()
{
    while (true)
    {
        Job job;

        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);

            m_Condition.wait(lock, [this]() { return !m_JobQueue.empty() || !m_IsRunning; });

            if (!m_IsRunning && m_JobQueue.empty())
                return;

            job = std::move(m_JobQueue.front());
            m_JobQueue.pop();
        }
        RunJob(std::move(job));
    }
}

bool JobSystem::TryExecuteOne()
{
    Job job;
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        if (m_JobQueue.empty())
            return false;
        job = std::move(m_JobQueue.front());
        m_JobQueue.pop();
    }
    RunJob(std::move(job));
    return true;
}

void JobSystem::RunJob(Job job)
{
    JobSystem* previousSystem = s_ExecutingJobSystem;
    JobCounter* previousCounter = s_ExecutingCounter;
    s_ExecutingJobSystem = this;
    s_ExecutingCounter = job.counter;
    ++s_ExecutionDepth;
    try
    {
        if (job.task)
            job.task();
    }
    catch (const std::exception& e)
    {
        std::cerr << "[JobSystem] CRITICAL: Unhandled job exception: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "[JobSystem] CRITICAL: Unknown job exception" << std::endl;
    }
    --s_ExecutionDepth;
    s_ExecutingCounter = previousCounter;
    s_ExecutingJobSystem = previousSystem;

    const bool activeZero = m_ActiveJobs.fetch_sub(1, std::memory_order_acq_rel) == 1;
    const bool counterZero = job.counter && job.counter->fetch_sub(1, std::memory_order_acq_rel) == 1;
    if (activeZero || counterZero)
    {
        std::lock_guard<std::mutex> lock(m_WaitMutex);
        if (activeZero)
            m_WaitCondition.notify_all();
        if (counterZero)
            m_CounterCondition.notify_all();
    }
}
