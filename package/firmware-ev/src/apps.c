
#include "ev.h"
#include "apps.h"
#include <ev.h>
#include <apps/init_config.h>
#include <apps/init_boot.h>

#include <stdlib.h>

EV_APP(init_config, init_config, NULL, NULL, NULL, NULL);
EV_APP(init_boot, init_boot_boot, NULL, NULL, NULL, NULL);

const struct ev_app *ev_apps[] = {
	&app_init_boot,
	&app_init_config,
	NULL,
};
