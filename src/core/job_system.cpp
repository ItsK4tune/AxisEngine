#include <core/job_system.h>
#include <utils/logger.h>

void JobSystem::Initialize()
{
    if (m_IsRunning)
        return;

    m_IsRunning = true;

    // Use available threads - 1 (leave one for main thread), but guarantee at least 1 worker
    uint32_t numThreads = std::thread::hardware_concurrency();
    if (numThreads > 1)
        numThreads -= 1;
    else
        numThreads = 1;

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

    // Wake up all threads so they can exit the loop
    m_Condition.notify_all();

    for (std::thread& worker : m_Workers)
    {
        if (worker.joinable())
            worker.join();
    }

    m_Workers.clear();
    LOGGER_INFO("JobSystem") << "Thread pool shut down successfully.";
}

void JobSystem::Execute(std::function<void()> job)
{
    m_ActiveJobs.fetch_add(1, std::memory_order_relaxed);

    {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        m_JobQueue.push(std::move(job));
    }

    m_Condition.notify_one();
}

void JobSystem::Wait()
{
    std::unique_lock<std::mutex> lock(m_QueueMutex);
    m_WaitCondition.wait(lock, [this]() {
        return m_JobQueue.empty() && (m_ActiveJobs.load(std::memory_order_relaxed) == 0);
    });
}

bool JobSystem::IsBusy()
{
    std::unique_lock<std::mutex> lock(m_QueueMutex);
    return !m_JobQueue.empty() || m_ActiveJobs.load(std::memory_order_relaxed) > 0;
}

void JobSystem::WorkerLoop()
{
    while (true)
    {
        std::function<void()> job;

        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);

            // Wait until a job is available or the system is shutting down
            m_Condition.wait(lock, [this]() {
                return !m_JobQueue.empty() || !m_IsRunning;
            });

            if (!m_IsRunning && m_JobQueue.empty())
                return;

            job = std::move(m_JobQueue.front());
            m_JobQueue.pop();
        }

        // Execute the job outside of the mutex lock
        if (job)
        {
            job();
        }

        // Decrement active jobs and notify anyone waiting for jobs to finish
        m_ActiveJobs.fetch_sub(1, std::memory_order_release);
        m_WaitCondition.notify_all();
    }
}

