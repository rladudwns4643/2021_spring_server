#pragma once
#include "pch.h"
#include "Singleton.h"

class ThreadInterface;
class ThreadHandler {
private:
	std::vector<ThreadInterface*> threads;
	void AddThread(ThreadInterface* myThread);

public:
	ThreadHandler();
	~ThreadHandler();

public:
	void CreateThreads();
	void JoinThreads();
};