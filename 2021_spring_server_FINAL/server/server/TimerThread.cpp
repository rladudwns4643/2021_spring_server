#include "pch.h"
#include "TimerThread.h"
#include "extern.h"

void TimerThread::InitThread() {
	mythread = std::thread([&]() {TimerThread::ProcThread(); });
}


void TimerThread::ProcThread() {
	while (true) {
		ATOMIC::g_timer_lock.lock();
		while (SR::g_timer_queue.empty()) {
			ATOMIC::g_timer_lock.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			ATOMIC::g_timer_lock.lock();
		}

		const EVENT& ev = SR::g_timer_queue.top(); //���� ���� ��ٸ� event
		if (std::chrono::high_resolution_clock::now() < ev.event_time) { //�� �ð����� eventTime�� �ڶ��(���� ó���ϸ� �ȵǴ� event���)
			ATOMIC::g_timer_lock.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		EVENT proc_ev = ev;
		SR::g_timer_queue.pop();
		ATOMIC::g_timer_lock.unlock();

		EX_OVER* ex_over = new EX_OVER;
		//if (SR::g_world->is_start == true) {
			switch (proc_ev.et) {
			case EV_UPDATE: {
				ex_over->ev_type = EV_UPDATE;
				break;
			}
			case EV_FLUSH_MSG: {
				ex_over->ev_type = EV_FLUSH_MSG;
				break;
			}
			case EV_RANDOM_MOVE: {
				ex_over->ev_type = EV_RANDOM_MOVE;
				break;
			}
			case EV_PLAYER_MOVE_NOTIFY: {
				ex_over->ev_type = EV_PLAYER_MOVE_NOTIFY;
				ex_over->obj_id = proc_ev.tmp;//������ player_id
				break;
			}
			default: { while (true); break; }
			}
			//GetQueuedCompletionStatus�� event ���� ����
			bool retval = PostQueuedCompletionStatus(SR::g_iocp, 1, proc_ev.id, &ex_over->over);
		}
	//}
}

void TimerThread::JoinThread() {
	mythread.join();
}