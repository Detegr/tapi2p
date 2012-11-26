#include "core.h"
#include <signal.h>

void sig(int signal)
{
	Core::Stop();
}

int main(int argc, char** argv)
{
	signal(SIGINT, sig);
	signal(SIGPIPE, SIG_IGN); // We don't need SIGPIPE when AF_UNIX socket is disconnected.
	return Core::Start(argc, argv);
}
