#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define _VERSION_MAJOR 0
#define _VERSION_MINOR 1

#define BATSTAT "/sys/class/power_supply/BAT0/status"
#define BATCAP "/sys/class/power_supply/BAT0/capacity"

#define DFL_INTERVAL 60
#define DFL_THRESHOLD 2

enum batstat {
	CHARGING = 1,
	DISCHARGING
};

void main_loop(int threshold, int interval);
int get_battery_capacity(void);
int get_battery_status(void);
int nag(void);

void usage(void) {
	fprintf(stderr,
		"Low battery indicator using i3-nagbar.\n\n"
		"Usage:\n"
		"  batnag [options]\n\n"
		"Options:\n"
		"  -d --daemon         Run in daemon mode\n"
		"  -h --help           Show this\n"
		"  -i --interval=<n>   Interval to check battery status\n"
		"  -t --threshold=<n>  Critical battery level\n"
		"  -v --version        Print version\n"
	       );
	exit(0);
}

void version(void) {
	fprintf(stderr,
		"batnag %d.%d\n",
		_VERSION_MAJOR,
		_VERSION_MINOR
	       );
	exit(0);
}

int main(int argc, char *argv[])
{
	int c = 0;
	int daemon_mode = 0;
	int option_index = 0;
	int threshold = DFL_THRESHOLD;
	int interval = DFL_INTERVAL;
	pid_t pid = 0;
	struct option long_options[] = {
		{"daemon", no_argument, 0, 'd'},
		{"help", no_argument, 0, 'h'},
		{"interval", required_argument, 0, 'i'},
		{"threshold", required_argument, 0, 't'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	for (;;) {
		c = getopt_long(argc, argv, "dhi:t:v", long_options,
				&option_index);
		if (c == -1) {
			/* end of arguments */
			break;
		}
		switch (c) {
			case 'd':
				daemon_mode = 1;
				break;
			case 'h':
				usage();
				return EXIT_SUCCESS;
			case 'i':
				interval = atoi(optarg);
				break;
			case 't':
				threshold = atoi(optarg);
				break;
			case 'v':
				version();
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
	main_loop(threshold, interval);
	return EXIT_SUCCESS;
}

void main_loop(int threshold, int interval)
{
	int cap = 0;
	int _interval = interval;

	for (;;) {
		if (get_battery_status() == DISCHARGING) {
			cap = get_battery_capacity();
			if (cap < threshold) {
				/* Skip sleep if nag fails. */
				if (nag() == -1) {
					continue;
				}
			} else if (cap < 10) {
				/* decrease interval */
				_interval = 10;
			}
		} else {
			/* reset interval */
			_interval = interval;
		}
		sleep(_interval);
	}
}

int get_battery_capacity(void)
{
	char buf[4] = {0};
	int cap = 0;
	FILE *f = fopen(BATCAP, "r");

	if (f != NULL) {
		fread(buf, sizeof(buf), 1, f);
		cap = atoi(buf);
		fclose(f);
	}

	return cap;
}

int get_battery_status(void)
{
	char buf[16] = {0};
	int status = 0;
	FILE *f = fopen(BATSTAT, "r");

	if (f != NULL) {
		fread(buf, sizeof(buf), 1, f);
		if (strncmp(buf, "Charging", 8) == 0) {
			status = CHARGING;
		} else if (strncmp(buf, "Discharging", 11) == 0) {
			status = DISCHARGING;
		}
		fclose(f);
	}

	return status;
}

int nag(void)
{
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
		if (get_battery_status() == CHARGING) {
			kill(pid, SIGTERM);
			return 0;
		}
		/*
		 * If the child exited itself, close this [parent] function as
		 * well.
		 */
		if (kill(pid, 0) == -1) {
			return -1;
		}
		sleep(1);
	}
}
