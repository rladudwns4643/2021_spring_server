#include <iostream>
#include <thread>
using namespace std;

atomic<int> sum = 0;
atomic<int> v = 0;
atomic<bool> flags[2] = { false, false };
constexpr int MAX = 25'000'000;
void p_lock(int t_id) {
	int other = 1 - t_id;
	flags[t_id] = true;
	v = t_id;
	while ((flags[other] == true) && (v == t_id));
}

void p_unlock(int t_id) {
	flags[t_id] = false;
}

void worker(int t_id) {
	for (int i = 0; i < MAX; ++i) {
		sum += 2;
	}
}

int main() {
	thread t1{ worker, 0 };
	thread t2{ worker, 1 };

	t1.join();
	t2.join();

	cout << sum << endl;
}