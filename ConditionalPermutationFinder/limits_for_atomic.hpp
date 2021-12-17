#pragma once

#include <atomic>
#include <limits>

template <typename T>
class std::numeric_limits<std::atomic<T>>: public std::numeric_limits<T> {};
