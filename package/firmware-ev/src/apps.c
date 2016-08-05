
#include "ev.h"
#include "apps.h"
#include <ev.h>
#include <apps/init_config.h>
#include <apps/init_boot.h>
#include <apps/init_vpn.h>

#include <stdlib.h>

EV_APP(init_config, init_config, NULL, NULL, NULL, NULL);
EV_APP(init_boot, init_boot_boot, NULL, NULL, NULL, NULL);
EV_APP(init_vpn, init_vpn_check, NULL, init_vpn_up, init_vpn_down, NULL);

#if defined(EV_DEBUG)
//EV_APP(new_config, new_config_wrapper, new_config_wrapper, NULL);
#endif

const struct ev_app *ev_apps[] = {
	&app_init_boot,
	&app_init_config,
	&app_init_vpn,
#if defined(EV_DEBUG)
#endif
	NULL,
};
