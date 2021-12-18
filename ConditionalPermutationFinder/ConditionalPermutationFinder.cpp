/*
 * [k進法]
 * k 進法において、1～k-1を並び替えた数で、以下の実例を満たすような数を求める。
 * 初等的な計算によって、k = 偶数の場合においてのみ存在することがすぐに分かる。
 * また、奇数番目の数は奇数で、偶数番目の数は偶数で、中央の数は k/2
 * であり、k-1の倍数の場合を確かめる必要はない（その場合は、必ずk-1の倍数になる）ことが分かる。
 * （これは除外しなくても速度は殆ど変わらないので除外しないでおく。）
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
 *
 * 答え
 * [2進法] 1
 * [4進法] 123, 321
 * [6進法] 14325, 54321
 * [8進法] 3254167, 5234761, 5674321
 * [10進法] 381654729
 * [12進法] 解なし
 * [14進法] 9C3A5476B812D
 * [16進法] 解なし
 */
/* 処理速度メモ
 * 12進法の場合、
 *   queue + mutex で100秒
 *   vector + index で30秒
 *   偶数/奇数による最適化で一瞬
 * 16進法の場合、
 *   偶数/奇数による最適化で25秒
 */

#include "ThreadPool.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <execution>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <vector>

using digit_t = uint_fast8_t;
std::vector<std::vector<digit_t>> get_conditional_permutations(
    const unsigned int                               N,
    std::function<void(const std::vector<digit_t>&)> callback_when_found =
        nullptr);

int main() {
	unsigned int N;
	std::cout << "何進法で計算しますか(数値を入力(2～16)): ", std::cin >> N;
	// N=16までは動作が保証される。それより上は、正しく動作するかどうかまだ検証していない。
	if (std::cin.fail() || N < 2 || N > 16) {
		std::cerr << "数値入力エラーのため終了します。" << std::endl;
		return EXIT_FAILURE;
	}

	auto results = get_conditional_permutations(N, [](const auto& result) {
		for (unsigned int digit : result) {
			if (digit < 10) {
				std::cout << digit;
			} else {
				assert(digit >= 10);
				// Assuming ascii-compatible character encoding
				std::cout << (char)('A' + (digit - 10));
			}
		}
		std::cout << '\n';
	});

	std::cout << "正常に計算終了しました。\n"
	          << results.size() << "件見つかりました。" << std::endl;

	return EXIT_SUCCESS;
}

static bool satisfy_condition(const std::vector<digit_t>& permutation);

std::vector<std::vector<digit_t>> get_conditional_permutations(
    const unsigned int                               N,
    std::function<void(const std::vector<digit_t>&)> callback_when_found) {
	std::vector<std::vector<digit_t>> results;
	// 奇数の場合は、空
	if (N % 2 == 1) {
		return results;
	}

	assert(N % 2 == 0);
	std::vector<digit_t> odd_part;
	odd_part.reserve(N / 2);
	std::vector<digit_t> even_part;
	even_part.reserve(odd_part.capacity() - 1);

	const unsigned int center_i = N / 2;
	for (unsigned int i = 1; i <= N - 1; ++i) {
		if (center_i == i) {
			continue;
		}

		if (i % 2 == 1) {
			odd_part.push_back(i);
		} else {
			even_part.push_back(i);
		}
	}

	const digit_t center_digit = center_i;
	{
		ThreadPool condition_check_threads;
		std::mutex results_push_mtx;
		do {
			do {
				std::vector<digit_t> permutation;
				permutation.reserve(N - 1);

				{
					auto odd_part_src_it  = odd_part.cbegin();
					auto even_part_src_it = even_part.cbegin();
					for (unsigned int i = 1; i <= N - 1; ++i) {
						if (center_i == i) {
							permutation.push_back(center_digit);
							continue;
						}

						if (i % 2 == 1) {
							assert(odd_part_src_it != odd_part.end());
							permutation.push_back(*odd_part_src_it);
							++odd_part_src_it;
						} else {
							assert(even_part_src_it != even_part.end());
							permutation.push_back(*even_part_src_it);
							++even_part_src_it;
						}
					}
				}

				condition_check_threads.enqueue(
				    [permutation, &results, &results_push_mtx, &callback_when_found]() {
					    if (satisfy_condition(permutation)) {
						    std::lock_guard lk(results_push_mtx);
						    results.push_back(permutation);
						    callback_when_found(results.back());
					    }
				    });
			} while (std::next_permutation(even_part.begin(), even_part.end()));
		} while (std::next_permutation(odd_part.begin(), odd_part.end()));
	} // Wait for completetion

	return results;
}

static bool satisfy_condition(const std::vector<digit_t>& permutation) {
	const unsigned int N = static_cast<int>(permutation.size() + 1);

	std::vector<int> mod_list;
	mod_list.reserve(permutation.size());
	for (unsigned int multiplier = 1; multiplier <= permutation.size();
	     ++multiplier) {
		mod_list.push_back(
		    std::accumulate(permutation.cbegin(), permutation.cbegin() + multiplier,
		                    0u, [N, multiplier](unsigned int lhs, digit_t rhs) {
			                    return ((N % multiplier) * lhs + rhs) % multiplier;
		                    }));
	}

	return std::all_of(std::execution::par_unseq, mod_list.cbegin(),
	                   mod_list.cend(),
	                   [](const auto& mod_ith) { return mod_ith == 0; });
}
