#include <libnotify/notify.h>

#include "nagbar.h"

int lnotify_init(void)
{
	return !notify_init("batnag");
}

int lnotify_nag(void)
{
	NotifyNotification *nag = notify_notification_new(
			"Low battery", "Battery level is critical.", NULL);
	notify_notification_set_timeout(nag, NOTIFY_EXPIRES_NEVER);
	notify_notification_set_urgency(nag, NOTIFY_URGENCY_CRITICAL);
	notify_notification_show(nag, NULL);
	g_object_unref(G_OBJECT(nag));

	return 0;
}

int lnotify_warn(void)
{
	NotifyNotification *nag = notify_notification_new(
			"Low battery", "Battery level is low.", NULL);
	notify_notification_set_urgency(nag, NOTIFY_URGENCY_NORMAL);
	notify_notification_show(nag, NULL);
	g_object_unref(G_OBJECT(nag));

	return 0;
}

void lnotify_cleanup(void)
{
	notify_uninit();
}

struct bn_module lnotify_ops = {
	.name = "libnotify",
	.init = lnotify_init,
	.nag = lnotify_nag,
	.warn = lnotify_warn,
	.cleanup = lnotify_cleanup,
};

BN_MODULE(lnotify_ops)
