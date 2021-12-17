#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

// thread pool
class ThreadPool {
public:
	template <class F>
	void enqueue(F&& f) {
		std::unique_lock<std::mutex> lock(queue_mutex);
		cv_finished.wait(lock,
		                 [this]() { return tasks.size() <= workers.size() * 2; });
		tasks.emplace(std::forward<F>(f));
		cv_task.notify_one();
	}

	void wait_until_completed();
	void wait_until_empty();

	unsigned int getProcessed() const noexcept {
		return processed;
	}

public:
	ThreadPool(unsigned int n = std::thread::hardware_concurrency());

public:
	~ThreadPool();

private:
	std::vector<std::thread>          workers;
	std::queue<std::function<void()>> tasks;
	std::mutex                        queue_mutex;
	std::condition_variable           cv_task;
	std::condition_variable           cv_finished;
	std::atomic_uint                  processed;
	unsigned int                      busy;
	bool                              stop;

	void thread_proc();
};
