#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config.h"
#include "nagbar.h"

void main_loop(int interval, bool notify_terminals,
	       const struct mods *mods);
int nag(bool notify_terminals, const struct bn_module *mod);
int warn(bool notify_terminals, const struct bn_module *mod);
void wall(int type);
void handle_exit(int sig);

static inline void usage(void)
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
		"  --mods              Show available modules\n"
		"  --nag=<mod>         Use <mod> as nagger (defaults to"
		" `nagbar' if applicable)\n"
		"  --tn=<n>            Critical battery level\n"
		"  --tw=<n>            Warning battery level\n"
		"  -v --version        Print version\n"
		"  --warn=<mod>        use <mod> as warner\n");
	exit(0);
}

static inline void version(void)
{
	fprintf(stderr, "%s\n", PROJECT_VERSION);
	exit(0);
}

static inline void handle_long_option(struct option *options, int opt_index,
				      struct mods *mods)
{
	if (strncmp("mods", options[opt_index].name, 4) == 0) {
		// TODO: list available modules
	} else if (strncmp("nag", options[opt_index].name, 3) == 0) {
		mods->nag = bn_get_module(optarg);
	} else if (strncmp("tn", options[opt_index].name, 2) == 0) {
		set_nag_threshold((uint8_t) atoi(optarg));
	} else if (strncmp("tw", options[opt_index].name, 2) == 0) {
		set_warn_threshold((uint8_t) atoi(optarg));
	} else if (strncmp("warn", options[opt_index].name, 4) == 0) {
		mods->warn = bn_get_module(optarg);
	}
}

static int pid_file;

int main(int argc, char *argv[])
{
	int c = 0;
	bool daemon_mode = false;
	bool notify_terminals = true;
	int option_index = 0;
	int interval = DFL_INTERVAL;
	pid_t pid = 0;
	struct mods mods = {
		.nag = bn_get_module("nagbar"),
		.warn = NULL,
	};
	struct option long_options[] = {
		{"daemon", no_argument, 0, 'd'},
		{"help", no_argument, 0, 'h'},
		{"interval", required_argument, 0, 'i'},
		{"nag", required_argument, 0, 0},
		{"no-wall", no_argument, 0, 'n'},
		{"tn", required_argument, 0, 0},
		{"tw", required_argument, 0, 0},
		{"version", no_argument, 0, 'v'},
		{"warn", required_argument, 0, 0},
		{0, 0, 0, 0}
	};

	// allow one instance only
	if ((pid_file = open(PID_FILE, O_CREAT | O_RDWR, 0666)) < 0) {
		perror("open");
		exit(1);
	}

	if (flock(pid_file, LOCK_EX | LOCK_NB) && EWOULDBLOCK == errno) {
		perror("flock");
		exit(1);
	}

	for (;;) {
		c = getopt_long(argc, argv, "dhi:nt:vw::", long_options,
				&option_index);
		if (c == -1) {
			/* end of arguments */
			break;
		}
		switch (c) {
		case 0:
			handle_long_option(long_options, option_index,
					   &mods);
			break;
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
		case 'v':
			version();
		default:
			usage();
		}
	}

	/*
	 * We need either mods.nag or mods.warn, otherwise this program
	 * is totally useless.
	 */
	if (!(mods.nag || mods.warn))
	{
		fputs("No module selected. Please make sure the modules are"
		      " enabled and/or you have\nselected one.\n\n",
		      stderr);
		usage();
	}

	if (daemon_mode) {
		pid = fork();
		if (pid == -1) {
			perror("fork");
			return EXIT_FAILURE;
		}
		if (pid > 0) {
			return EXIT_SUCCESS;
		}
	}

	uint32_t tn = get_nag_threshold();
	uint32_t tw = get_warn_threshold();

	if (tw > 0 && tn >= tw) {
		fprintf(stderr,
			"warning: "
			"Additional warning will not be displayed if\n"
			"         threshold is greater than warning_threshold "
			"(%d >= %d).\n", tn, tw);
	}

	if (mods.nag) {
		if (mods.nag->init()) {
			fputs("Failed to initialize module\n", stderr);
		}

		if (atexit(mods.nag->cleanup) != 0) {
			fputs("Failed to register cleanup function\n", stderr);
		}
	}

	if (mods.warn) {
		if (mods.warn->init()) {
			fputs("Failed to initialize module\n", stderr);
		}

		if (atexit(mods.warn->cleanup) != 0) {
			fputs("Failed to register cleanup function\n", stderr);
		}
	}

	signal(SIGTERM, handle_exit);
	signal(SIGINT, handle_exit);
	main_loop(interval, notify_terminals, &mods);

	return EXIT_SUCCESS;
}

void main_loop(int interval, bool notify_terminals,
	       const struct mods *mods)
{
	uint32_t cap = 0;
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
		/*
		 * The threshold should be obtained by calling
		 * get_nag_threshold() directly and not by a function
		 * argument, since mods->nag() could change this value.
		 * The same applies to get_warn_threshold().
		 */
		if (cap <= get_nag_threshold()) {
			/* Skip sleep if nag fails. */
			if (nag(notify_terminals, mods->nag) == -1) {
				continue;
			}
		} else if (mods->warn != NULL && !warned &&
			   cap <= get_warn_threshold()) {
			if (warn(notify_terminals, mods->warn) == 0) {
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

int nag(bool notify_terminals, const struct bn_module *mod)
{
	if (notify_terminals) {
		wall(NAG);
	}

	return mod->nag();
}

int warn(bool notify_terminals, const struct bn_module *mod)
{
	if (notify_terminals) {
		wall(WARN);
	}

	return mod->warn();
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

void handle_exit(int sig __attribute__((unused)))
{
	if (pid_file > 0) {
		flock(pid_file, LOCK_UN);
		remove(PID_FILE);
	}

	exit(0);
}
