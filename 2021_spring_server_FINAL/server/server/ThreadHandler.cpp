#include "pch.h"
#include "server_define.h"
#include "ThreadHandler.h"
#include "ThreadInterface.h"
#include "WorkerThread.h"
#include "TimerThread.h"

ThreadHandler::ThreadHandler() {}

ThreadHandler::~ThreadHandler() {
	JoinThreads();
}

void ThreadHandler::AddThread(ThreadInterface* m) {
	m->InitThread();
	threads.emplace_back(m);
}

void ThreadHandler::CreateThreads() {
	//Worker Thread
#ifdef LOG_ON
	std::cout << "Create Thread " << MAX_WORKERTHREAD << endl;
#endif
	for (int i = 0; i < NETWORK_UTIL::WORKER_THREAD_NUM; ++i) AddThread(new WorkerThread);
#ifdef LOG_ON
	std::cout << "Worker Thread Create Complate\n";
#endif

	//Timer Therad
#ifdef LOG_ON
	std::cout << "Create Timer Thread " << endl;
#endif
	AddThread(new TimerThread);
#ifdef LOG_ON
	std::cout << "Timer Thread Create Complate\n";
#endif
}

void ThreadHandler::JoinThreads() {
	for (auto& t : threads) t->JoinThread();
}