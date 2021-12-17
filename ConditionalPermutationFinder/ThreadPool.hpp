#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

// thread pool
class ThreadPool {
public:
	void enqueue(std::function<void()> task);
	void wait_until_completed();
	void wait_until_empty();

public:
	ThreadPool(unsigned int num_of_threads = std::thread::hardware_concurrency());

public:
	~ThreadPool();

private:
	std::vector<std::thread>           m_workers;
	std::vector<std::function<void()>> m_tasks;
	std::uint_fast16_t                 m_task_index;

	std::mutex              queue_mutex;
	std::condition_variable cv_task;
	std::condition_variable cv_finished;

	std::atomic_uint busy;
	bool             stop;

	void thread_proc();
};
