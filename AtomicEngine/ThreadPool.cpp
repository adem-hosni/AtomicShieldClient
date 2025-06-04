#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool
{
public:
    ThreadPool(size_t numThreads) : stop(false)
    {
        for (size_t i = 0; i < numThreads; ++i)
            workers.emplace_back(
                [this]()
                {
                    while (true)
                    {
                        std::function<void()> task;

                        {            // lock scope
                            std::unique_lock<std::mutex> lock(queueMutex);
                            condition.wait(lock, [this]() { return stop || !tasks.empty(); });
                            if (stop && tasks.empty())
                                return;
                            task = std::move(tasks.front());
                            tasks.pop();
                        }

                        task();            // Run the task
                    }
                });
    }

    template <class F>
    void enqueue(F&& f)
    {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }

    void shutdown()
    {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();

        for (std::thread& worker : workers)
            if (worker.joinable())
                worker.join();
    }

    ~ThreadPool() { shutdown(); }

private:
    std::vector<std::thread>          workers;
    std::queue<std::function<void()>> tasks;

    std::mutex              queueMutex;
    std::condition_variable condition;
    std::atomic<bool>       stop;
};
