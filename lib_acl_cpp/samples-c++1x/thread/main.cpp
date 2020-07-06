#include "stdafx.h"
#include <vector>
#include <thread>

static void thread_main(void)
{
	logger("thread-%lu: hello, error=%s",
		acl::thread::self(), acl::last_serror());
}

int main(void)
{
	// ≥ı ºªØ acl ø‚
	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	std::vector<std::thread*> threads;
	for (int i = 0; i < 10; i++) {
		std::thread* thread = new std::thread(thread_main);
		threads.push_back(thread);
	}

	for (std::vector<std::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {
		(*it)->join();
		delete *it;
	}

	return 0;
}
