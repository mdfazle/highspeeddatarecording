#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

class ThreadPool
{
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    template <typename Func, typename... Args>
    auto enqueue(Func &&func, Args &&...args) -> std::future<std::invoke_result_t<Func, Args...>>;

    void wait();

private:
    // keep track of threads so we can join them
    std::vector<std::thread> workers;

    // the task queue
    std::queue<std::function<void()>> tasks;

    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
    size_t active_workers;
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t numThreads) : stop(false), active_workers(0)
{
    for (size_t i = 0; i < numThreads; ++i)
    {
        workers.emplace_back([this]
                             {
            for (;;) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                    if (this->stop && this->tasks.empty()) {
                        return;
                    }
                    ++active_workers;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }

                task();

                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    --active_workers;
                    if (this->stop && this->active_workers == 0) {
                        this->condition.notify_all();
                    }
                }
            } });
    }
}

// add new work item to the pool
template <typename Func, typename... Args>
auto ThreadPool::enqueue(Func &&func, Args &&...args) -> std::future<std::invoke_result_t<Func, Args...>>
{
    using return_type = std::invoke_result_t<Func, Args...>;

    auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if constexpr (std::is_same_v<return_type, void>)
        {
            if (stop)
            {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
        }
        else
        {
            if (stop)
            {
                return std::future<return_type>();
            }
        }

        tasks.emplace([task]()
                      { (*task)(); });
    }
    condition.notify_one();
    return res;
}

// wait for all threads to complete
inline void ThreadPool::wait()
{
    std::unique_lock<std::mutex> lock(queue_mutex);
    condition.wait(lock, [this]
                   { return this->tasks.empty() && this->active_workers == 0; });
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers)
    {
        worker.join();
    }
}
