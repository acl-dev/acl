#include "lib_acl.h"
#include <signal.h>

static void on_sigterm(int signo)
{
	acl_msg_info("get signal sigterm: %d, %d", signo, SIGTERM);
}

static void on_sigkill(int signo)
{
	acl_msg_info("get signal sigkill: %d, %d", signo, SIGKILL);
}

int main(int argc acl_unused, char *argv[] acl_unused)
{
	acl_msg_open("./child.log", "child");
	signal(SIGTERM, on_sigterm);
	signal(SIGKILL, on_sigkill);

	while (1)
		pause();

	return (0);
}
