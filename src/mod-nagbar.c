#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "nagbar.h"

int nagbar_init(void)
{
	return 0;
}

int nagbar_nag(void)
{
	int status;
	pid_t pid = fork();

	if (pid == 0) {
		/* child */
		execl("/usr/bin/i3-nagbar", "i3-nagbar", "-t", "error",
		      "-m", "Battery level is critical.", NULL);
		perror("exec");
	} else if (pid == -1) {
		return -1;
	}

	/*
	 * Do not wait for the child to exit. Instead constantly check the
	 * battery status and kill the child if necessary.
	 */
	for (;;) {
		if (!is_critical()) {
			kill(pid, SIGTERM);
			wait(NULL);
			return 0;
		}

		/*
		 * If the child exited itself, close this [parent] function as
		 * well.
		 */
		waitpid(-1, &status, WNOHANG);
		if (kill(pid, 0) == -1) {
			return -1;
		}
		sleep(1);
	}

	return 0;
}

int nagbar_warn(void)
{
	pid_t pid = fork();

	if (pid == 0) {
		/* child */
		execl("/usr/bin/i3-nagbar", "i3-nagbar", "-t", "warning",
		      "-m", "Battery level is low.", NULL);
		perror("exec");
	} else if (pid == -1) {
		perror("fork");
		return -1;
	}

	/* close warning after X seconds */
	sleep(10);
	kill(pid, SIGTERM);
	wait(NULL);

	return 0;
}

void nagbar_cleanup(void)
{
}

struct bn_module nagbar_ops = {
	.name = "nagbar",
	.init = nagbar_init,
	.nag = nagbar_nag,
	.warn = nagbar_warn,
	.cleanup = nagbar_cleanup,
};

BN_MODULE(nagbar_ops)
