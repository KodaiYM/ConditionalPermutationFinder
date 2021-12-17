/*
 * [k進法]
 * k 進法において、1～k-1を並び替えた数で、以下の実例を満たすような数を求める。
 * 初等的な計算によって、k = 偶数の場合においてのみ存在することがすぐに分かる。
 * 興味深いことに、10進法においては一通りしか存在しない。
 *
 * 実例
 * [14進法]
 * ABCDEFGHIJKLM: 1～13の並び替え
 * A: 1の倍数
 * AB: 2の倍数
 * ABC: 3の倍数
 * ABCD: 4の倍数
 * ABCDE: 5の倍数
 * ABCDEF: 6の倍数
 * …
 * ABCDEFGHIJKLM: 13の倍数
 */

#include "ThreadPool.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <execution>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <vector>

std::vector<std::vector<int>> get_conditional_permutations(int N);

int main() {
	int N;
	std::cout << "何進法で計算しますか(数値を入力(2～16)): ", std::cin >> N;
	if (std::cin.fail() || N < 2 || N > 16) {
		std::cerr << "数値入力エラーのため終了します。" << std::endl;
		return EXIT_FAILURE;
	}

	auto results = get_conditional_permutations(N);
	for (const auto& result : results) {
		for (int digit : result) {
			if (0 <= digit && digit < 10) {
				std::cout << digit;
			} else {
				assert(digit >= 10);
				// Assuming ascii-compatible character encoding
				std::cout << ('a' + (digit - 10));
			}
		}
		std::cout << '\n';
	}

	std::cout << "正常に計算終了しました。" << std::endl;

	return EXIT_SUCCESS;
}

static bool satisfy_condition(const std::vector<int>& permutation);

std::vector<std::vector<int>> get_conditional_permutations(int N) {
	std::vector<std::vector<int>> results;

	std::vector<int> permutation(N - 1);
	// initialize
	std::iota(permutation.begin(), permutation.end(), 1);

	{
		ThreadPool condition_check_threads;
		std::mutex results_push_mtx;
		do {
			condition_check_threads.enqueue(
			    [permutation, &results, &results_push_mtx]() {
				    if (satisfy_condition(permutation)) {
					    std::lock_guard lk(results_push_mtx);
					    results.push_back(permutation);
				    }
			    });
		} while (std::next_permutation(permutation.begin(), permutation.end()));
	} // Wait for completetion

	return results;
}

static bool satisfy_condition(const std::vector<int>& permutation) {
	const int N = static_cast<int>(permutation.size() + 1);

	std::vector<int> mod_list;
	mod_list.reserve(permutation.size());
	for (int multiplier = 1; multiplier <= static_cast<int>(permutation.size());
	     ++multiplier) {
		mod_list.push_back(
		    std::accumulate(permutation.cbegin(), permutation.cbegin() + multiplier,
		                    0, [N, multiplier](int lhs, int rhs) {
			                    return ((N % multiplier) * lhs + rhs) % multiplier;
		                    }));
	}

	return std::all_of(std::execution::par_unseq, mod_list.cbegin(),
	                   mod_list.cend(),
	                   [](const auto& mod_ith) { return mod_ith == 0; });
}
