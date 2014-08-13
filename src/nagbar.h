#ifndef _NAGBAR_H_
#define _NAGBAR_H_

#include <stdbool.h>
#include <stdint.h>

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

struct bn_module {
	struct bn_module *next;
	const char *name;
	int (*init)(void);
	int (*nag)(void);
	int (*warn)(void);
	void (*cleanup)(void);
};

struct mods {
	const struct bn_module *nag;
	const struct bn_module *warn;
};

uint32_t get_battery_capacity(void);
uint32_t get_battery_status(void);

uint32_t get_nag_threshold(void);
void set_nag_threshold(uint32_t threshold);
uint32_t get_warn_threshold(void);
void set_warn_threshold(uint32_t threshold);

bool is_critical(void);

void bn_register_module(struct bn_module *module);
const struct bn_module *bn_get_module(const char *name);

#define BN_MODULE(MOD)                                          \
static void __init_ ## MOD(void) __attribute__ ((constructor)); \
static void __init_ ## MOD(void)                                \
{                                                               \
	bn_register_module(&MOD);                               \
}
#endif
