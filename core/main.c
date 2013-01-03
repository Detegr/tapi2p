#include "core.h"
#include <signal.h>

void sig(int signal)
{
	core_stop();
}

int main(int argc, char** argv)
{
	signal(SIGINT, sig);
	signal(SIGPIPE, SIG_IGN); // We don't need SIGPIPE when AF_UNIX socket is disconnected.
	return core_start();
}
