#include <iostream>
#include <thread>
using namespace std;

int g_data = 0;
bool g_ready = false;

void receiver() {
	while (false == g_ready) {
		cout << "I Recvived " << g_data << endl;
	}
}

void sender() {
	g_data = 999;
	g_ready = true;
}

int main() {
	thread th_recv{ receiver };
	this_thread::sleep_for(1s);
	thread th_send{ sender };

	th_recv.join();
	th_send.join();
}