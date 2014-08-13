#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "config.h"
#include "nagbar.h"

static uint32_t nag_threshold = DFL_THRESHOLD;
static uint32_t warn_threshold = DFL_WARN_THRESHOLD;
static struct bn_module *base = NULL;

uint32_t get_battery_capacity(void)
{
	char buf[4] = { 0 };
	uint32_t cap = 0;
	int unused __attribute__ ((unused));
	FILE *f = fopen(BATCAP, "r");

	if (f != NULL) {
		unused = fread(buf, 1, sizeof(buf), f);
		cap = (uint32_t) atoi(buf);
		fclose(f);
	}

	return cap;
}

uint32_t get_battery_status(void)
{
	char buf[16] = { 0 };
	uint32_t status = 0;
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

uint32_t get_nag_threshold(void)
{
	return nag_threshold;
}

void set_nag_threshold(uint32_t threshold)
{
	nag_threshold = threshold;
}

uint32_t get_warn_threshold(void)
{
	return warn_threshold;
}

void set_warn_threshold(uint32_t threshold)
{
	warn_threshold = threshold;
}

bool is_critical(void)
{
	return (get_battery_status() == DISCHARGING &&
		get_battery_capacity() <= nag_threshold);
}

void bn_register_module(struct bn_module *ops)
{
	ops->next = base;
	base = ops;
}

const struct bn_module *bn_get_module(const char *name)
{
	const struct bn_module *ops;

	if (!name) {
		return NULL;
	}

	for (ops = base; ops; ops = ops->next) {
		if (!strcasecmp(name, ops->name)) {
			return ops;
		}
	}

	return NULL;
}
