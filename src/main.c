#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config.h"

#ifndef BATSTAT
#define BATSTAT "/sys/class/power_supply/BAT0/status"
#endif

#ifndef BATCAP
#define BATCAP "/sys/class/power_supply/BAT0/capacity"
#endif

#define DFL_INTERVAL 60
#define DFL_THRESHOLD 2
#define DFL_WARN_THRESHOLD 5

enum batstat {
	CHARGING = 1,
	DISCHARGING
};

enum typenotify {
	WARN = 1,
	NAG
};

void main_loop(int threshold, int warn_threshold, int interval,
	       int notify_terminals);
int get_battery_capacity(void);
int get_battery_status(void);
int nag(int notify_terminals, int threshold);
int warn(int notify_terminals);
void wall(int type);

inline void usage(void)
{
	fprintf(stderr,
		"Low battery indicator using i3-nagbar.\n\n"
		"Usage:\n"
		"  batnag [options]\n\n"
		"Options:\n"
		"  -d --daemon         Run in daemon mode\n"
		"  -h --help           Show this\n"
		"  -i --interval=<n>   Interval to check battery status\n"
		"  -n --no-wall        Do not notify TTYs\n"
		"  -t --threshold=<n>  Critical battery level\n"
		"  -v --version        Print version\n"
		"  -w --warn=[<n>]     Send additional warning\n");
	exit(0);
}

inline void version(void)
{
	fprintf(stderr, "%s\n", PACKAGE_STRING);
	exit(0);
}

int main(int argc, char *argv[])
{
	int c = 0;
	bool daemon_mode = false;
	bool notify_terminals = true;
	int option_index = 0;
	int threshold = DFL_THRESHOLD;
	int interval = DFL_INTERVAL;
	int warn_threshold = 0;
	pid_t pid = 0;
	struct option long_options[] = {
		{"daemon", no_argument, 0, 'd'},
		{"help", no_argument, 0, 'h'},
		{"interval", required_argument, 0, 'i'},
		{"no-wall", no_argument, 0, 'n'},
		{"threshold", required_argument, 0, 't'},
		{"version", no_argument, 0, 'v'},
		{"warn", optional_argument, 0, 'w'},
		{0, 0, 0, 0}
	};

	for (;;) {
		c = getopt_long(argc, argv, "dhi:nt:vw::", long_options,
				&option_index);
		if (c == -1) {
			/* end of arguments */
			break;
		}
		switch (c) {
		case 'd':
			daemon_mode = true;
			break;
		case 'h':
			usage();
			return EXIT_SUCCESS;
		case 'i':
			interval = atoi(optarg);
			break;
		case 'n':
			notify_terminals = false;
			break;
		case 't':
			threshold = atoi(optarg);
			break;
		case 'v':
			version();
		case 'w':
			if (optarg == NULL) {
				warn_threshold = DFL_WARN_THRESHOLD;
			} else {
				warn_threshold = atoi(optarg);
			}
			break;
		default:
			usage();
		}
	}

	if (daemon_mode == 1) {
		pid = fork();
		if (pid == -1) {
			perror("fork");
			return EXIT_FAILURE;
		}
		if (pid > 0) {
			return EXIT_SUCCESS;
		}
	}

	if (warn_threshold > 0 && threshold >= warn_threshold) {
		fprintf(stderr,
			"warning: "
			"Additional warning will not be displayed if\n"
			"         threshold is greater than warning_threshold "
			"(%d >= %d).\n", threshold, warn_threshold);
	}

	main_loop(threshold, warn_threshold, interval, notify_terminals);

	return EXIT_SUCCESS;
}

void main_loop(int threshold, int warn_threshold, int interval,
	       int notify_terminals)
{
	int cap = 0;
	int _interval = interval;
	bool warned = false;

	for (;;) {
		if (get_battery_status() != DISCHARGING) {
			/* reset interval */
			_interval = interval;
			/* reset warned status */
			warned = false;
			sleep(_interval);
			continue;
		}
		cap = get_battery_capacity();
		if (cap <= threshold) {
			/* Skip sleep if nag fails. */
			if (nag(notify_terminals, threshold) == -1) {
				continue;
			}
		} else if (!warned && cap <= warn_threshold) {
			if (warn(notify_terminals) == 0) {
				/* do not warn again */
				warned = true;
			}
		}
		if (cap < 10) {
			/* decrease interval */
			_interval = 10;
		}
		sleep(_interval);
	}
}

int get_battery_capacity(void)
{
	char buf[4] = { 0 };
	int cap = 0;
	int unused __attribute__ ((unused));
	FILE *f = fopen(BATCAP, "r");

	if (f != NULL) {
		unused = fread(buf, 1, sizeof(buf), f);
		cap = atoi(buf);
		fclose(f);
	}

	return cap;
}

int get_battery_status(void)
{
	char buf[16] = { 0 };
	int status = 0;
	int unused __attribute__ ((unused));
	FILE *f = fopen(BATSTAT, "r");

	if (f != NULL) {
		unused = fread(buf, 1, sizeof(buf), f);
		if (strncmp(buf, "Charging", 8) == 0) {
			status = CHARGING;
		} else if (strncmp(buf, "Discharging", 11) == 0) {
			status = DISCHARGING;
		}
		fclose(f);
	}

	return status;
}

int nag(int notify_terminals, int threshold)
{
	int status = 0;
	pid_t pid = 0;

	if (notify_terminals) {
		wall(NAG);
	}

	pid = fork();

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
		if (get_battery_status() == CHARGING ||
		    get_battery_capacity() > threshold) {
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
}

int warn(int notify_terminals)
{
	pid_t pid = 0;

	if (notify_terminals) {
		wall(WARN);
	}

	pid = fork();

	if (pid == 0) {
		/* child */
		execl("/usr/bin/i3-nagbar", "i3-nagbar", "-t", "warning",
		      "-m", "Battery level is low.", NULL);
		perror("exec");
	} else if (pid == -1) {
		return -1;
	}

	/* close warning after X seconds */
	sleep(10);
	kill(pid, SIGTERM);
	wait(NULL);

	return 0;
}

void wall(int type)
{
	char *template = "wall -t 10 'Battery level is %s.'";
	char msg[43] = { 0 };
	int unused __attribute__ ((unused));

	if (type == WARN) {
		sprintf(msg, template, "low");
	} else if (type == NAG) {
		sprintf(msg, template, "critical");
	}

	unused = system(msg);
}
