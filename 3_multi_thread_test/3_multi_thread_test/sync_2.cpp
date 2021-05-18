#include <iostream>
#include <thread>
using namespace std;

volatile int* bounce;
volatile bool done{ false };
int error{ 0 };

void changer() {
	for (int i = 0; i < 25'000'000; ++i) {
		*bounce = -(1 + *bounce);
	}
	done = true;
}

void checker() {
	while (done == false) {
		int v = *bounce;
		if ((v != 0) && (v != -1)) {
			printf("%X, ", v);
			error++;
		}
	}
}

int main() {
	int arr[17];
	long long addr = reinterpret_cast<long long>(&(arr[16]));
	int offset = addr % 64;
	addr = addr - offset;
	addr = addr - 2;
	bounce = reinterpret_cast<int*>(addr);
	*bounce = 0;
	thread t1{ changer };
	thread t2{ checker };
	t1.join();
	t2.join();
	cout << error << endl;
}