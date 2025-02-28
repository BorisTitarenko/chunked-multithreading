#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <future>
#include <functional>
#include <mutex>
#include <condition_variable>


class ThreadPool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop_flag = false;

public:
    explicit ThreadPool(size_t num_threads)
    {
        Start(num_threads);
    }

    ~ThreadPool()
    {
        Stop();
    }

    // Submit a task to the thread pool
    template <typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        
        using ReturnType = typename std::invoke_result<F, Args...>::type;

        // Wrap the task in a shared pointer to a packaged_task
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> future = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace([task]() { (*task)(); });
        }

        condition.notify_one(); // Wake up a thread
        return future;
    }

private:

    void Start(size_t num_threads)
    {
        for (size_t i = 0; i < num_threads; ++i)
        {
            workers.emplace_back([this]
                {
                while (true)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { return stop_flag || !tasks.empty(); });

                        if (stop_flag && tasks.empty())
                            return;

                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task(); // Execute the task
                }
            });
        }
    };

    void Stop()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop_flag = true;
        }

        condition.notify_all();
        for (std::thread &worker : workers)
            worker.join();
    }
};
