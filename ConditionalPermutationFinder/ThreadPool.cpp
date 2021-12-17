#include "ThreadPool.hpp"
#include "limits_for_atomic.hpp"

#include <limits>

ThreadPool::ThreadPool(unsigned int num_of_threads)
    : m_task_index(), busy(), stop() {
	if (std::numeric_limits<decltype(busy)>::max() < num_of_threads) {
		throw std::invalid_argument("too many threads");
	}

	for (unsigned int i = 0; i < num_of_threads; ++i) {
		m_workers.emplace_back(std::bind(&ThreadPool::thread_proc, this));
	}

	// m_tasks is up to 40MB
	constexpr auto tasks_capacity =
	    size_t(40) * size_t(1024) * size_t(1024) / sizeof(std::function<void()>);
	m_tasks.reserve(tasks_capacity);
	static_assert(std::numeric_limits<decltype(m_task_index)>::max() >=
	              tasks_capacity);
}

ThreadPool::~ThreadPool() {
	// set stop-condition
	std::unique_lock<std::mutex> latch(queue_mutex);
	stop = true;
	cv_task.notify_all();
	latch.unlock();

	// all threads terminate, then we're done.
	for (auto& t : m_workers)
		t.join();
}

void ThreadPool::enqueue(std::function<void()> task) {
	if (m_tasks.size() == m_tasks.capacity()) {
		wait_until_empty();
		m_tasks.clear();
		m_task_index = 0;
	}
	m_tasks.emplace_back(std::move(task));
	cv_task.notify_one();
}
void ThreadPool::thread_proc() {
	while (true) {
		std::unique_lock<std::mutex> lock(queue_mutex);
		// wait until enqueue task
		cv_task.wait(lock,
		             [this] { return stop || m_task_index < m_tasks.size(); });

		if (m_task_index < m_tasks.size()) {
			// got work. set busy.
			++busy;

			// pull from queue
			auto fn = std::move(m_tasks.at(m_task_index++));

			lock.unlock(); // abort!

			// run function outside context
			fn();

			--busy;
			cv_finished.notify_one();
		} else if (stop) {
			break;
		}
	}
}
// waits until all m_workers are free
void ThreadPool::wait_until_completed() {
	std::unique_lock<std::mutex> lock(queue_mutex);
	cv_finished.wait(
	    lock, [this]() { return m_tasks.size() == m_task_index && (busy == 0); });
}

// waits until the m_tasks is empty
void ThreadPool::wait_until_empty() {
	std::unique_lock<std::mutex> lock(queue_mutex);
	cv_finished.wait(lock, [this]() { return m_tasks.size() == m_task_index; });
}
