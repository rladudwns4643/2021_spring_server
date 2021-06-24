/*
single thread 와 multi thread간의 성능 차이 확인
*/
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>

using namespace std;
using namespace chrono;

constexpr int LOOP_CNT = 500'000'000;
constexpr int THREAD_CNT = 16;
struct NUM {
	volatile alignas(64) int num;
};
namespace SR {
	NUM sum[THREAD_CNT];
}

namespace ATOMIC {
	mutex sum_lock;
}

void worker(int numofthread, int idofthread) {
	volatile int local_sum = 0;
	for (int i = 0; i < LOOP_CNT / numofthread; ++i) {
		SR::sum[idofthread] = SR::sum[idofthread] + 2;
	}
}

int main() {

	for (int num_threads = 1; num_threads <= THREAD_CNT; num_threads *= 2) {
		for (auto& s : SR::sum) s = 0;
		auto start_t = high_resolution_clock::now();
		vector<thread> vt_workers;
		for (int i = 0; i < num_threads; ++i) {
			vt_workers.emplace_back(worker, num_threads, i);
		}
		for (auto& th : vt_workers) {
			th.join();
		}
		int t = 0;
		for (auto& s : SR::sum) t += s;
		auto exec_t = high_resolution_clock::now() - start_t;
		cout << "thread Cnt: " << num_threads << " ";
		cout << duration_cast<milliseconds>(exec_t).count() << "ms ";
		cout << "Result: " << t << endl;
	}
}