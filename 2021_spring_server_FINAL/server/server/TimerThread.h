#pragma once
#include "ThreadInterface.h"

class TimerThread final : public ThreadInterface {
public:
	void InitThread() override;

	void ProcThread() override;
	void JoinThread() override;
};