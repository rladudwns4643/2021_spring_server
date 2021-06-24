#include <iostream>
#include <thread>
#include <vector>

constexpr unsigned char MAX_THREAD = 8;

void do_thread(int id) {
	std::cout << id <<" thread\n";
	for (;;);
}

int main() {
	std::vector<std::thread> my_thread;
	for (int i = 0; i < MAX_THREAD; ++i) {
		my_thread.emplace_back(do_thread, i);
	}
	for (auto& t : my_thread) {
		t.join();
	}
}